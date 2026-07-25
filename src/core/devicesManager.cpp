#include "devicesManager.h"
// ============================================================================
// Implementation (devicemanager.cpp)
// ============================================================================

#include <algorithm>
#include <iostream>

DeviceManager::DeviceManager(prometheus::Registry *registry,
                             std::chrono::milliseconds defaultPollInterval)
    : running_(false), stopRequested_(false), registry_(registry),
      defaultPollInterval_(defaultPollInterval), maxErrorsBeforeDisable_(5),
      errorCallback_(nullptr) {}

DeviceManager::~DeviceManager() { stop(); }

bool DeviceManager::addDevice(std::shared_ptr<labdevices> device,
                              std::chrono::milliseconds pollInterval) {
  if (!device) {
    std::cerr << "Cannot add null device" << std::endl;
    return false;
  }

  std::lock_guard<std::mutex> lock(devicesMutex_);

  // Check if device already exists
  if (findDevice(device->device_name) != devices_.end()) {
    std::cerr << "Device '" << device->device_name << "' already exists"
              << std::endl;
    return false;
  }

  // Use default interval if not specified
  auto interval =
      (pollInterval.count() > 0) ? pollInterval : defaultPollInterval_;

  // Set registry for the device
  device->setRegistry(registry_);

  // Create managed device wrapper
  auto managedDevice =
      std::make_shared<ManagedDevice>(device, device->device_name, interval);
  devices_.push_back(managedDevice);

  std::cout << "Added device: " << device->device_name
            << " (poll interval: " << interval.count() << "ms)" << std::endl;

  return true;
}
bool DeviceManager::deviceOnline(std::string deviceName) {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it == devices_.end()) {
    return false;
  }

  return (*it)->device->isOnline();
}
bool DeviceManager::removeDevice(const std::string &deviceName) {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it == devices_.end()) {
    return false;
  }

  // Disable before removing
  (*it)->enabled = false;

  devices_.erase(it);
  std::cout << "Removed device: " << deviceName << std::endl;

  return true;
}

int DeviceManager::getDevicePollingTime(const std::string &deviceName) {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it == devices_.end()) {
    return -1;
  }

  return (*it)->pollInterval.count();
}

bool DeviceManager::enableDevice(const std::string &deviceName) {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it == devices_.end()) {
    return false;
  }

  (*it)->enabled = true;
  (*it)->errorCount = 0; // Reset error count on enable
  std::cout << "Enabled device: " << deviceName << std::endl;

  return true;
}

bool DeviceManager::disableDevice(const std::string &deviceName) {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it == devices_.end()) {
    return false;
  }

  (*it)->enabled = false;
  std::cout << "Disabled device: " << deviceName << std::endl;

  return true;
}

bool DeviceManager::isDeviceEnabled(const std::string &deviceName) const {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it == devices_.end()) {
    return false;
  }

  return (*it)->enabled.load();
}

void DeviceManager::start() {
  if (running_.load()) {
    std::cout << "DeviceManager already running" << std::endl;
    return;
  }

  stopRequested_ = false;
  running_ = true;
  updateThread_ = std::thread(&DeviceManager::run, this);

  std::cout << "DeviceManager started" << std::endl;
}

void DeviceManager::stop() {
  if (!running_.load()) {
    return;
  }

  std::cout << "Stopping DeviceManager..." << std::endl;

  stopRequested_ = true;

  if (updateThread_.joinable()) {
    updateThread_.join();
  }

  running_ = false;
  std::cout << "DeviceManager stopped" << std::endl;
}

void DeviceManager::run() {
  while (!stopRequested_.load()) {
    auto loopStart = std::chrono::steady_clock::now();

    // Create a local copy of devices to minimize lock time
    std::vector<std::shared_ptr<ManagedDevice>> devicesCopy;
    {
      std::lock_guard<std::mutex> lock(devicesMutex_);
      devicesCopy = devices_;
    }

    // Update each device
    for (auto &managedDev : devicesCopy) {
      if (!managedDev->enabled.load()) {
        continue;
      }

      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          now - managedDev->lastUpdate);

      // Check if it's time to update this device
      if (elapsed >= managedDev->pollInterval) {
        try {
          managedDev->device->update();
          managedDev->lastUpdate = now;
          managedDev->errorCount = 0; // Reset error count on success

        } catch (const std::exception &e) {
          int errorCount = ++managedDev->errorCount;
          std::cerr << "Error updating device '" << managedDev->name
                    << "': " << e.what() << " (error count: " << errorCount
                    << ")" << std::endl;

          if (errorCallback_) {
            errorCallback_(managedDev->name, e.what());
          }

          // Disable device if too many errors
          if (maxErrorsBeforeDisable_ > 0 &&
              errorCount >= maxErrorsBeforeDisable_) {
            managedDev->enabled = false;
            std::cerr << "Device '" << managedDev->name << "' disabled after "
                      << errorCount << " consecutive errors" << std::endl;
          }

        } catch (...) {
          int errorCount = ++managedDev->errorCount;
          std::cerr << "Unknown error updating device '" << managedDev->name
                    << "'" << std::endl;

          if (errorCallback_) {
            errorCallback_(managedDev->name, "Unknown error");
          }

          if (maxErrorsBeforeDisable_ > 0 &&
              errorCount >= maxErrorsBeforeDisable_) {
            managedDev->enabled = false;
          }
        }
      }
    }

    // Sleep to avoid busy waiting (use minimum poll interval as sleep time)
    auto minInterval = defaultPollInterval_;
    for (const auto &dev : devicesCopy) {
      if (dev->enabled.load() && dev->pollInterval < minInterval) {
        minInterval = dev->pollInterval;
      }
    }

    // Sleep for a fraction of minimum interval to ensure responsive polling
    auto sleepTime = minInterval / 10;
    if (sleepTime < std::chrono::milliseconds(10)) {
      sleepTime = std::chrono::milliseconds(10);
    }

    std::this_thread::sleep_for(sleepTime);
  }
}

void DeviceManager::setDefaultPollInterval(std::chrono::milliseconds interval) {
  defaultPollInterval_ = interval;
}

void DeviceManager::setDevicePollInterval(const std::string &deviceName,
                                          std::chrono::milliseconds interval) {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it != devices_.end()) {
    (*it)->pollInterval = interval;
    std::cout << "Set poll interval for device '" << deviceName << "' to "
              << interval.count() << "ms" << std::endl;
  }
}

int DeviceManager::getDeviceCount() const {
  std::lock_guard<std::mutex> lock(devicesMutex_);
  return devices_.size();
}

int DeviceManager::getEnabledDeviceCount() const {
  std::lock_guard<std::mutex> lock(devicesMutex_);
  return std::count_if(devices_.begin(), devices_.end(),
                       [](const auto &dev) { return dev->enabled.load(); });
}

int DeviceManager::getDeviceErrorCount(const std::string &deviceName) const {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  auto it = findDevice(deviceName);
  if (it != devices_.end()) {
    return (*it)->errorCount.load();
  }
  return -1;
}

std::vector<std::string> DeviceManager::getDeviceNames() const {
  std::lock_guard<std::mutex> lock(devicesMutex_);

  std::vector<std::string> names;
  names.reserve(devices_.size());
  for (const auto &dev : devices_) {
    names.push_back(dev->name);
  }
  return names;
}

std::vector<std::shared_ptr<ManagedDevice>>::iterator
DeviceManager::findDevice(const std::string &deviceName) {
  return std::find_if(
      devices_.begin(), devices_.end(),
      [&deviceName](const auto &dev) { return dev->name == deviceName; });
}

std::vector<std::shared_ptr<ManagedDevice>>::const_iterator
DeviceManager::findDevice(const std::string &deviceName) const {
  return std::find_if(
      devices_.begin(), devices_.end(),
      [&deviceName](const auto &dev) { return dev->name == deviceName; });
}

bool DeviceManager::deviceExist(std::string deviceName) {

  std::vector<std::string> namelist = getDeviceNames();
  for (auto name : namelist) {
    if (deviceName.compare(name) == 0) {
      return true;
    }
  }
  return false;
}

#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "labdev_log.h"
#include "labdevices.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <prometheus/registry.h>
#include <thread>
#include <vector>

// Device wrapper to track individual device state
struct ManagedDevice {
  std::shared_ptr<labdevices> device;
  std::string name;
  std::atomic<bool> enabled;
  std::chrono::milliseconds pollInterval;
  std::chrono::steady_clock::time_point lastUpdate;
  std::atomic<int> errorCount;

  //:::   Logging state, so the polling loop can report a fault once    :::
  //:::   instead of once per cycle. lastOnline is tri-state: -1 means  :::
  //:::   isOnline() has not been sampled yet.                          :::
  std::atomic<int> lastOnline;
  labdev::log_latch updateFault;

  ManagedDevice(std::shared_ptr<labdevices> dev, const std::string &n,
                std::chrono::milliseconds interval)
      : device(dev), name(n), enabled(true), pollInterval(interval),
        lastUpdate(std::chrono::steady_clock::now()), errorCount(0),
        lastOnline(-1) {}
};

class DeviceManager {
public:
  // Constructor
  explicit DeviceManager(prometheus::Registry *registry,
                         std::chrono::milliseconds defaultPollInterval =
                             std::chrono::milliseconds(1000));

  // Destructor
  ~DeviceManager();

  // Prevent copying
  DeviceManager(const DeviceManager &) = delete;
  DeviceManager &operator=(const DeviceManager &) = delete;

  // Add a device with optional custom poll interval
  bool addDevice(
      std::shared_ptr<labdevices> device,
      std::chrono::milliseconds pollInterval = std::chrono::milliseconds(0));

  // Remove a device by name
  bool removeDevice(const std::string &deviceName);

  // Control individual devices
  bool enableDevice(const std::string &deviceName);
  bool disableDevice(const std::string &deviceName);
  bool isDeviceEnabled(const std::string &deviceName) const;

  // Global control
  void start();
  void stop();
  bool isRunning() const { return running_.load(); }

  // Configuration
  void setDefaultPollInterval(std::chrono::milliseconds interval);
  void setDevicePollInterval(const std::string &deviceName,
                             std::chrono::milliseconds interval);
  void setMaxErrorsBeforeDisable(int maxErrors) {
    maxErrorsBeforeDisable_ = maxErrors;
  }

  // Status and monitoring
  int getDeviceCount() const;
  int getEnabledDeviceCount() const;
  int getDeviceErrorCount(const std::string &deviceName) const;
  std::vector<std::string> getDeviceNames() const;

  // Callback for error handling (optional)
  using ErrorCallback = std::function<void(const std::string &deviceName,
                                           const std::string &error)>;
  void setErrorCallback(ErrorCallback callback) { errorCallback_ = callback; }

  bool deviceExist(std::string deviceName);

  int getDevicePollingTime(const std::string &deviceName);
  bool deviceOnline(std::string deviceName);

  // Last value read by the device on its previous update(). Returns false when
  // the device or the gauge is unknown.
  bool getDeviceValue(const std::string &deviceName,
                      const std::string &gaugeName, double &value) const;
  std::vector<std::string> getDeviceGaugeNames(const std::string &deviceName)
      const;

  //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  //:::   Typed access to a managed device, so device specific methods  :::
  //:::   can be called (e.g. getDevice<lakeshore336>("ls")->setp_1()). :::
  //:::   Returns nullptr when the name is unknown or when T is not the :::
  //:::   actual device type. Beware: methods reached this way run on   :::
  //:::   the CALLER's thread while update() may be running on the      :::
  //:::   polling thread, and the transports are not mutex protected.   :::
  //:::   Disable the device first when the call does any I/O.          :::
  //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  template <typename T>
  std::shared_ptr<T> getDevice(const std::string &deviceName) {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    auto it = findDevice(deviceName);
    if (it == devices_.end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<T>((*it)->device);
  }

private:
  // Main update loop
  void run();

  // Logs online <-> offline changes only, see the definition.
  void logOnlineTransition(ManagedDevice &dev);

  // Helper to find device by name
  std::vector<std::shared_ptr<ManagedDevice>>::iterator
  findDevice(const std::string &deviceName);

  std::vector<std::shared_ptr<ManagedDevice>>::const_iterator
  findDevice(const std::string &deviceName) const;

  // Thread management
  std::thread updateThread_;
  std::atomic<bool> running_;
  std::atomic<bool> stopRequested_;

  // Device storage
  std::vector<std::shared_ptr<ManagedDevice>> devices_;
  mutable std::mutex devicesMutex_;

  // Configuration
  prometheus::Registry *registry_;
  std::chrono::milliseconds defaultPollInterval_;
  int maxErrorsBeforeDisable_;

  // Error handling
  ErrorCallback errorCallback_;
};

#endif // DEVICEMANAGER_H

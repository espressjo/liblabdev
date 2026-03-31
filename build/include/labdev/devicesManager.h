#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

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

  ManagedDevice(std::shared_ptr<labdevices> dev, const std::string &n,
                std::chrono::milliseconds interval)
      : device(dev), name(n), enabled(true), pollInterval(interval),
        lastUpdate(std::chrono::steady_clock::now()), errorCount(0) {}
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

private:
  // Main update loop
  void run();

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

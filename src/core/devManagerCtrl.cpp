#include "devManagerCtrl.h"
#include <chrono>

void dev_ctrl_status(instHandle *handle, cmd *cc) {
  //:::::::::::::::::::::::::::::::::::::
  //:::   list the state of devices   :::
  //:::::::::::::::::::::::::::::::::::::
  if ((*cc)["-print"].compare("") != 0) {
    std::vector<std::string> list = handle->devManager->getDeviceNames();
    std::string msg = "\n";
    int _polling = 0;
    for (auto devName : list) {
      _polling = handle->devManager->getDevicePollingTime(devName);
      if (handle->devManager->isDeviceEnabled(devName)) {
        msg += devName + " " + std::to_string(_polling) + " ms" +
               " [\033[32m enabled\033[0m ]\n";
      } else {
        msg += devName + " " + std::to_string(_polling) + " ms" +
               " [\033[31m disabled\033[0m ]\n";
      }
    } // for loop
    cc->respond(msg);
    return;
  }

  //:::::::::::::::::::::::::::
  //:::   Trigger devices   :::
  //:::::::::::::::::::::::::::

  // first get the device name
  std::string dev = "";
  int polling_ms = 0;
  if ((*cc)["dev"].compare("") != 0) {
    dev = (*cc)["dev"];

    if (!handle->devManager->deviceExist(dev)) {
      cc->respond("given device does not exist", uicsCMD_ERR_PARAM_FORMAT);
      return;
    }
  } else {
    cc->respond("Expect a device to continue", uicsCMD_ERR_PARAM_FORMAT);
    return;
  }

  if ((*cc)["-disable"].compare("") != 0) {
    if (!handle->devManager->disableDevice(dev)) {
      cc->respond("Fail to disable device", uicsCMD_ERR_PARAM_UNKNOWN);
      return;
    }
    cc->respond();
    return;
  }
  if ((*cc)["-enable"].compare("") != 0) {
    if (!handle->devManager->enableDevice(dev)) {
      cc->respond("Fail to enable device", uicsCMD_ERR_PARAM_UNKNOWN);
      return;
    }
    cc->respond();
    return;
  }

  if ((*cc)["polling"].compare("") != 0) {
    polling_ms = std::stod((*cc)["polling"]);
    auto ms = std::chrono::milliseconds(polling_ms);
    handle->devManager->setDevicePollInterval(dev, ms);
    cc->respond();
    return;
  }

  cc->respond();
  return;
}

#include "labjack.h"
#include <iostream>
#include <random>
#include <uics/uics_config.h>

labjack::labjack(std::string configfile, std::string devicename = "",
                 prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry)

{
  IP = cfg_get_string(configfile, "IP", "ANY");
  std::string LJACKTYPE = cfg_get_string(configfile, "LJACKTYPE", "T4");
  std::string CONNECTIONTYPE =
      cfg_get_string(configfile, "CONNECTIONTYPE", "ETHERNET");
  lj_handle = 0;
  error = 0;
  online = false;

  if (LJM_OpenS(LJACKTYPE.c_str(), CONNECTIONTYPE.c_str(), IP.c_str(),
                &lj_handle) != 0) {
    online = false;
  } else {
    online = true;
  }
  this->devicename = devicename;
  //:::::::::::::::::::::::::::::::::::
  //:::   Add all your gauge here   :::
  //:::::::::::::::::::::::::::::::::::
  addGauge(devicename + "AIN1", "Analogue AIN1");
  addGauge(devicename + "AIN0", "Analogue AIN0");
}
int labjack::readA00(double *value) {
  if (online) {
    error = LJM_eReadName(lj_handle, "AIN0", value);
    if (error != 0) {
      std::cout << "[debug] error???" << std::endl;
    }
    return OK;
  } else {
    return NOK;
  }
  return NOK;
}

int labjack::readA01(double *value) {
  if (online) {
    error = LJM_eReadName(lj_handle, "AIN1", value);
    if (error != 0) {
      std::cout << "[debug] error???" << std::endl;
    }
    return OK;
  } else {
    return NOK;
  }
  return NOK;
}
void labjack::update() {

  // 1. Create a random device (seed)

  // std::random_device rd;

  // 2. Initialize a Mersenne Twister engine with the seed
  // std::mt19937 gen(rd());

  // 3. Define the distribution (range [min, max))
  // std::uniform_real_distribution<double> dis(0.0,
  std::cout << "[debug] Labjack" << std::endl;
  double A01 = 0.0;
  double A00 = 0.0;
  
if (readA01(&A01) == OK) {
    online = true;
	readA00(&A00);
    setGauge(devicename + "AIN1", A01);
    setGauge(devicename + "AIN0", A00);
    std::cout << "[labjack] OK" << std::endl;
  } else {

    std::cout << "[labjack] NOK" << std::endl;
    online = false;
  }
}

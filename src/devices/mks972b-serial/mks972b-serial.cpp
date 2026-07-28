#include "mks972b-serial.h"
#include <iostream>
#include <random>
#include <uics/uics_config.h>

gauge972::gauge972(std::string configfile, std::string devicename = "",
                   prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry)

{
  timeout = -1;
  int port = cfg_get_int(configfile, "PORT", -1);
  int baudrate = cfg_get_int(configfile, "BAUDRATE", 9600);

  status = ouvrirport(port, baudrate);

  if (status != OK) {
    online = false;
  }

  addGauge("Pressure", "Pressure of the MKS972b");
}

gauge972::gauge972(int port, int baudrate, std::string devicename = "",
                   prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry)

{
  timeout = -1;
  status = ouvrirport(port, baudrate);
  if (status != OK) {
    online = false;
  }

  addGauge("Pressure", "Pressure of the MKS972b");
}
void gauge972::set_timeout(int to_s) { timeout = to_s; }
int gauge972::read_pressure(double *pressure) {
  std::string buff = "";
  // std::cout << "[debug] ecrire" << std::endl;
  ecrireport("@253PR4?;FF\f");
  if (timeout > 0) {

    // std::cout << "[debug] lire" << std::endl;
    lireport(&buff, 128, timeout);
  } else {

    lireport(&buff, 128);
  }
  std::string pressure_str = "";
  strip_ack(buff, &pressure_str);
  if (pressure_str.length() > 0) {
    *pressure = std::stod(pressure_str.c_str());
    return OK;
  } else {
    *pressure = 0.0;
    return NOK;
  }
}
int gauge972::read_pirani(double *pressure) {
  std::string buff = "";
  ecrireport("@253PR1?;FF\f");
  lireport(&buff, 128);
  std::string pressure_str = "";
  strip_ack(buff, &pressure_str);
  if (pressure_str.length() > 0) {
    *pressure = std::stod(pressure_str.c_str());
    return OK;
  } else {
    *pressure = 0.0;
    return NOK;
  }
}
int gauge972::read_cold_cathod(double *pressure) {
  std::string buff = "";
  ecrireport("@253PR5?;FF\f");
  lireport(&buff, 128);
  std::string pressure_str = "";
  strip_ack(buff, &pressure_str);
  if (pressure_str.length() > 0) {
    *pressure = std::stod(pressure_str.c_str());
    return OK;
  } else {
    *pressure = 0.0;
    return NOK;
  }
}
int gauge972::strip_ack(std::string ack, std::string *val)
// return the address of the gauge
{
  int adr = 0, start = 0;
  std::string tmp = "";
  if (ack.substr(0, 1).compare("@") != 0) {
    *val = "";
    return adr;
  }
  if (ack.substr(4, 3).compare("ACK") == 0) {
    adr = std::atoi(ack.substr(1, 3).c_str());
    start = 7;
  } else if (ack.substr(3, 3).compare("ACK") == 0) {
    adr = std::atoi(ack.substr(1, 2).c_str());
    start = 6;
  } else if (ack.substr(2, 3).compare("ACK") == 0) {
    adr = std::atoi(ack.substr(1, 1).c_str());
    start = 5;
  }
  if (adr == 0) {
    *val = "";
    return adr;
  }

  ack = ack.substr(start, ack.length() - 1); // strip the @XXXack
  for (auto &c : ack) {
    if (c == ';') {
      break;
    }
    tmp += c;
  }
  *val = tmp;
  return adr;
}
void gauge972::update() {

  // 1. Create a random device (seed)

  // std::random_device rd;

  // 2. Initialize a Mersenne Twister engine with the seed
  // std::mt19937 gen(rd());

  // 3. Define the distribution (range [min, max))
  // std::uniform_real_distribution<double> dis(0.0,
  std::cout << "ok" << std::endl;
  double pressure = 0.0;
  if (read_pressure(&pressure) == OK) {
    online = true;
    setGauge("Pressure", pressure);
  } else {
    online = false;
  }
}

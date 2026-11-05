#include "mks972b-serial.h"
#include <uics/parse_number.h>
#include <iostream>
#include <random>
#include <uics/uics_config.h>

gauge972::gauge972(std::string configfile, std::string devicename = "",
                   prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry)

{
  init_gauge();
  int port = cfg_get_int(configfile, "PORT", -1);
  int baudrate = cfg_get_int(configfile, "BAUDRATE", 9600);

  status = ouvrirport(port, baudrate);

  if (status != OK) {
    online = false;
    labdev::log_error(device_name, "cannot open serial port " +
                                       std::to_string(port) + " from " +
                                       configfile + ", device offline");
  }
}

// Shared by both constructors. Mirrors gauge972_e: pressure starts at 0,
// the device starts offline and the gauge is named "<devicename>_Pressure".
void gauge972::init_gauge() {
  timeout = -1;
  pressure = 0.0;
  online = false;
  p_gauge = device_name + "_Pressure";
  addGauge(p_gauge, "Pressure of the MKS972b");
}

gauge972::gauge972(int port, int baudrate, std::string devicename = "",
                   prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry)

{
  init_gauge();
  status = ouvrirport(port, baudrate);
  if (status != OK) {
    online = false;
    labdev::log_error(device_name, "cannot open serial port " +
                                       std::to_string(port) +
                                       ", device offline");
  }
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
  if (pressure_str.length() > 0 && parse_double(pressure_str, pressure)) {
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
  if (pressure_str.length() > 0 && parse_double(pressure_str, pressure)) {
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
  if (pressure_str.length() > 0 && parse_double(pressure_str, pressure)) {
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

  double p = 0.0;
  if (read_pressure(&p) == OK) {
    pressure = p;
    online = true;
    setGauge(p_gauge, pressure);
    if (reading_fault.failed()) {
      reading_fault.clear(device_name + ": reading valid again");
    }
  } else {
    online = false;
    // Latched, so a dead gauge costs two lines per outage, not one per poll.
    reading_fault.fail(device_name + ": no valid reply to PR4? on the serial "
                                     "link, check cabling and power",
                       labdev::LOG_WARN);
  }
}

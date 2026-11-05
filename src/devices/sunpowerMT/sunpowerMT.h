#ifndef SUNPOWER_H
#define SUNPOWER_H
#include "labdev_log.h"
#include "labdevices.h"
#include "serial.h"
#include "sunpowerMT_t.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <prometheus/registry.h>
#include <sstream>
#include <string>
#define DELAIMAX 5000 /* delai maximum pour l'attente en ms*/

#define NOK -1 /* valeur de retour avec erreur */
#define OK 0

class sunpowerMT : public labdevices, public serial {
public:
  // Config file keys, same as sunpowerDS30: DEVICE (default /dev/ttyACM0) and
  // BAUDRATE (default 9600).
  sunpowerMT(std::string configfile, std::string devicename,
             prometheus::Registry *registry);
  sunpowerMT(std::string port, int baudrate, std::string devicename,
             prometheus::Registry *registry);
  bool cold_finger_temperature(double *temperature);

  bool get_status(sunpowerMT_t &s_t);
  //:::   Same names and use as sunpowerDS30. These do serial I/O on the  :::
  //:::   caller's thread, so when the device is managed by a             :::
  //:::   DeviceManager, disableDevice() it first and enableDevice()      :::
  //:::   afterwards, as cpapir2 does. Unlike the DS30, success means the :::
  //:::   controller's reply confirms the new state (ON / OFF).           :::
  bool setCoolerOn();
  bool setCoolerOff();
  // Old names, kept so existing callers still build.
  bool set_cooler_on() { return setCoolerOn(); }
  bool set_cooler_off() { return setCoolerOff(); }
  void printStatus();
  bool AVC(bool status);

  bool basic_cmd(std::string cmd, std::string &ret);
  void update();
  void set_timeout(int to_s);

private:
  void init_gauges();
  int timeout;
  std::string port;
  int baudrate;

  sunpowerMT_t parseStatus(const std::string &input);

  std::string trim(const std::string &str);
};

#endif

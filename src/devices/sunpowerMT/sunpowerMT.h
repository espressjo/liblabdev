#ifndef SUNPOWER_H
#define SUNPOWER_H
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
  sunpowerMT(std::string port, int baudrate, std::string devicename,
           prometheus::Registry *registry);
  bool cold_finger_temperature(double *temperature);

  bool get_status(sunpowerMT_t &s_t);
  bool set_cooler_on();
  bool set_cooler_off();
  void printStatus();
  bool AVC(bool status);

  bool basic_cmd(std::string cmd, std::string &ret);
  void update();
  void set_timeout(int to_s);

private:
  int timeout;
  std::string port;
  int baudrate;

  sunpowerMT_t parseStatus(const std::string &input);

  std::string trim(const std::string &str);
};

#endif

#ifndef SUNPOWER_H
#define SUNPOWER_H
#include "labdev.h"
#include "labdev_log.h"
#include "labdevices.h"
#include "sunpowerDS30_t.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <prometheus/registry.h>
#include <sstream>
#include <string>
#define DELAIMAX 5000 /* delai maximum pour l'attente en ms*/

#define NOK -1 /* valeur de retour avec erreur */
#define OK 0

class sunpowerDS30 : public labdevices, public serial {
public:
  sunpowerDS30(std::string configfile, std::string devicename,
               prometheus::Registry *registry);
  sunpowerDS30(std::string port, int baudrate, std::string devicename,
               prometheus::Registry *registry);
  bool cold_finger_temperature(double *temperature);

  bool get_status(sunpower_t &s_t);

  void printStatus();
  bool AVC(bool status);
  bool get_vibration(std::string &buff);

  bool basic_cmd(std::string cmd, std::string &ret);

  //:::   Decodes a "C ERROR" / "NC ERROR" bit field ("0000 0000 0000  :::
  //:::   0001") into the numeric value of the mask, LSB on the right, :::
  //:::   so that every combination of simultaneous errors maps to one :::
  //:::   distinct value. The field width and the grouping are not     :::
  //:::   assumed: any number of bits is accepted, grouped any way, or :::
  //:::   not grouped at all, with an optional "0b" prefix. Spaces,    :::
  //:::   tabs, '_', ',', '-' and '|' are ignored as separators;       :::
  //:::   anything else makes the string invalid.                      :::
  //:::                                                                :::
  //:::   Returns true and writes the mask to value on success.        :::
  //:::   Returns false and leaves value at -1 when the string is      :::
  //:::   empty, holds a character that is not a bit, or carries more  :::
  //:::   than 53 significant bits (past what a double holds exactly). :::
  static bool parse_error_bits(const std::string &input, double &value);

  void update();
  void set_timeout(int to_s);
  bool setCoolerOn();
  bool setCoolerOff();

private:
  int timeout;
  std::string port;
  int baudrate;

  // Unreadable error field: logged on the edges only, see update().
  labdev::log_latch error_fault;

  sunpower_t parseStatus(const std::string &input);

  std::string trim(const std::string &str);
};

#endif

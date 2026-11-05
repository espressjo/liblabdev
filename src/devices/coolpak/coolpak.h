#ifndef COOLPAK_H
#define COOLPAK_H
#include "labdev.h"
#include "labdev_log.h"
#include "labdevices.h"
#include <mutex>
#include <prometheus/registry.h>
#include <string>
#include <vector>

#define NOK -1 /* valeur de retour avec erreur */
#define OK 0

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:::   Leybold COOLPAK 6000 helium compressor, RS-232 (4800 8N1).     :::
//:::                                                                  :::
//:::   Every message is  STX <3 or 4 ASCII letters/digits> CR.        :::
//:::     "DAT"  -> request data (14 fields separated by '/')          :::
//:::     "SYS1" -> switch the system ON,  reply "SYS" y               :::
//:::     "SYS0" -> switch the system OFF, reply "SYS" y               :::
//:::                (y: 0 = OFF, 1 = ON, 2 = SYSTEM ERROR)            :::
//:::     "ERR"  -> request the logged errors                          :::
//:::   Reference: Leybold COOLPAK 6000 manual, section 3.5.           :::
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// Decoded "DAT" reply.
struct coolpak_t {
  double version = 0.0;    // software version
  double hours = 0.0;      // hours counter
  double delay = 0.0;      // switch-ON timer [s], 0 = can start now
  int command_status = 0;  // 0 = OFF, 1 = ON, 2 = SYSTEM ERROR
  bool running = false;    // actual compressor status
  int active_errors = 0;   // number of errors still active
  unsigned error_mask = 0; // bit (n-1) set <=> error code n is active
  int logged_errors = 0;   // number of recorded errors
};

// One entry of the "ERR" reply.
struct coolpak_error {
  int code;   // 1..16, see coolpak::error_name()
  long hours; // hours counter when it was logged
};

class coolpak : public labdevices, public serial {
public:
  // Config file keys: DEVICE (default /dev/ttyUSB0), BAUDRATE (default 4800).
  coolpak(std::string configfile, std::string devicename,
          prometheus::Registry *registry);
  coolpak(std::string port, int baudrate, std::string devicename,
          prometheus::Registry *registry);

  void update();
  void set_timeout(int to_s);

  //:::   Switch the compressor system ON / OFF. True when the compressor :::
  //:::   answers with the requested state. A "SYSTEM ERROR" reply or an  :::
  //:::   unreadable one gives false.                                     :::
  //:::                                                                   :::
  //:::   Safe to call while a DeviceManager is polling this device: every :::
  //:::   serial exchange is serialised by an internal mutex, so the      :::
  //:::   command and update() never interleave on the wire. Beware that  :::
  //:::   the command can still wait up to one reply timeout behind a    :::
  //:::   running update().                                               :::
  bool setCoolerOn();
  bool setCoolerOff();

  bool get_status(coolpak_t &s);
  bool get_logged_errors(std::vector<coolpak_error> &errors);
  void printStatus();

  // Sends one command, adds STX / CR when missing, strips the STX of the
  // reply and checks that the reply carries the same 3 letter keyword.
  bool basic_cmd(std::string cmd, std::string &ret);

  // Decodes a "DAT" reply (STX already stripped). False when the field
  // count or any field is wrong, s is then left untouched.
  static bool parse_status(const std::string &input, coolpak_t &s);
  static std::string error_name(int code);

private:
  int timeout;
  std::string port;
  int baudrate;
  void init_gauges();
  bool set_system(bool on);

  std::mutex io_mutex; // one request / reply on the wire at a time

  // Edge-triggered logging, update() runs at the poll rate.
  labdev::log_latch comm_fault;
  labdev::log_latch alarm_fault;
};

#endif

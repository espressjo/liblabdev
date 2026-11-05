#include "coolpak.h"
#include "labdevices.h"
#include "serial.h"
#include <iostream>
#include <prometheus/registry.h>
#include <uics/parse_number.h>
#include <uics/uics_config.h>

static const char STX = '\x02';
static const int DEFAULT_REPLY_TIMEOUT_MS = 5000;

coolpak::coolpak(std::string configfile, std::string devicename,
                 prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry) {
  coolpak::port = cfg_get_string(configfile, "DEVICE", "/dev/ttyS0");
  coolpak::baudrate = cfg_get_int(configfile, "BAUDRATE", 9600);
  // The protocol ends every reply with <CR>, lirec() cannot wait for it
  // while the port discards <CR> (IGNCR).
  ignore_cariage_return = false;
  init_gauges();
  status = ouvrirport(coolpak::port, coolpak::baudrate);
  if (status != OK) {
    labdev::log_error(device_name, "cannot open " + coolpak::port + " at " +
                                       std::to_string(coolpak::baudrate) +
                                       " baud (from " + configfile + ")");
  }
}

coolpak::coolpak(std::string port, int baudrate, std::string devicename,
                 prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry) {
  coolpak::port = port;
  coolpak::baudrate = baudrate;
  ignore_cariage_return = false;
  init_gauges();
  status = ouvrirport(port, baudrate);
  if (status != OK) {
    labdev::log_error(device_name, "cannot open " + port + " at " +
                                       std::to_string(baudrate) + " baud");
  }
}

void coolpak::init_gauges() {
  timeout = -1;
  addGauge("Coolpak_Version", "COOLPAK software version");
  addGauge("Coolpak_Hours", "COOLPAK hours counter");
  addGauge("Coolpak_Delay", "COOLPAK switch-ON timer (s), 0 = can start now");
  addGauge("Coolpak_Command_Status",
           "COOLPAK command status (0 = OFF, 1 = ON, 2 = SYSTEM ERROR)");
  addGauge("Coolpak_Running",
           "COOLPAK actual compressor status (1 = ON, 0 = OFF)");
  addGauge("Coolpak_Active_Errors", "COOLPAK number of errors still active");
  addGauge("Coolpak_Error_Mask",
           "COOLPAK active errors as a bit mask, error code n is bit n-1 "
           "(0 = no error, 1 = SYSTEM ERROR, 4 = Locked rotor, ...)");
  addGauge("Coolpak_Logged_Errors", "COOLPAK number of recorded errors");
}

void coolpak::set_timeout(int to_s) { timeout = to_s; }

std::string coolpak::error_name(int code) {
  static const char *const names[] = {
      "SYSTEM ERROR",      "Compressor fail",  "Locked rotor",
      "OVERLOAD",          "Phase/fuse ERROR", "Pressure alarm",
      "Helium temp. fail", "Oil circuit fail", "RAM ERROR",
      "ROM ERROR",         "EEPROM ERROR",     "DC Voltage error",
      "MAINS LEVEL !!!!"};
  if (code >= 1 && code <= 13)
    return names[code - 1];
  return "Unknown error " + std::to_string(code);
}

//:::   One request, one reply, nothing else on the wire in between.    :::
bool coolpak::basic_cmd(std::string cmd, std::string &ret) {
  ret.clear();
  if (cmd.empty() || cmd[0] != STX)
    cmd.insert(0, 1, STX);
  if (cmd.back() != '\r')
    cmd += '\r';
  // keyword the reply must start with: "DAT", "SYS", "ERR"
  const std::string keyword = cmd.substr(1, 3);

  const int timeout_ms = timeout > 0 ? timeout * 1000 : DEFAULT_REPLY_TIMEOUT_MS;
  std::string buff;
  {
    std::lock_guard<std::mutex> lock(io_mutex);
    if (ecrireport(cmd) != OK)
      return false;
    if (lirec(&buff, '\r', timeout_ms) < 0)
      return false;
  }

  if (!buff.empty() && buff[0] == STX)
    buff.erase(0, 1);
  // A late reply to an earlier, timed out request would not match.
  if (buff.compare(0, keyword.size(), keyword) != 0)
    return false;
  ret = buff;
  return true;
}

//:::   "DAT2.08/004307/14175/23.5/01.7/06.9/003/1/1/1/0/02/              :::
//:::    1010000000000000/4"                                            :::
//:::   0 version, 1 internal, 2 hours, 3-5 internal, 6 switch-ON timer,  :::
//:::   7 command status, 8 compressor status, 9-10 not used,           :::
//:::   11 active errors, 12 error flags (char n = error n), 13 logged.  :::
bool coolpak::parse_status(const std::string &input, coolpak_t &s) {
  std::vector<std::string> f;
  size_t start = 0;
  for (;;) {
    size_t pos = input.find('/', start);
    if (pos == std::string::npos) {
      f.push_back(input.substr(start));
      break;
    }
    f.push_back(input.substr(start, pos - start));
    start = pos + 1;
  }
  if (f.size() != 14 || f[0].compare(0, 3, "DAT") != 0)
    return false;

  coolpak_t t;
  double v = 0.0;
  if (!parse_double(f[0].substr(3), &t.version))
    return false;
  if (!parse_double(f[2], &t.hours))
    return false;
  if (!parse_double(f[6], &t.delay))
    return false;
  if (!parse_double(f[7], &v))
    return false;
  t.command_status = static_cast<int>(v);
  if (!parse_double(f[8], &v))
    return false;
  t.running = (static_cast<int>(v) == 1);
  if (!parse_double(f[11], &v))
    return false;
  t.active_errors = static_cast<int>(v);

  const std::string &flags = f[12];
  if (flags.empty() || flags.size() > 32)
    return false;
  for (size_t i = 0; i < flags.size(); ++i) {
    if (flags[i] == '1')
      t.error_mask |= (1u << i);
    else if (flags[i] != '0')
      return false;
  }

  if (!parse_double(f[13], &v))
    return false;
  t.logged_errors = static_cast<int>(v);

  s = t;
  return true;
}

bool coolpak::get_status(coolpak_t &s) {
  std::string reply;
  if (!basic_cmd("DAT", reply))
    return false;
  return parse_status(reply, s);
}

void coolpak::update() {
  coolpak_t s;
  if (!get_status(s)) {
    online = false;
    if (!comm_fault.failed())
      comm_fault.fail(device_name + ": no valid DAT reply from the compressor "
                                    "on " + port + ", check cabling and power");
    return;
  }
  online = true;
  if (comm_fault.failed())
    comm_fault.clear(device_name + ": compressor answers again");

  setGauge("Coolpak_Version", s.version);
  setGauge("Coolpak_Hours", s.hours);
  setGauge("Coolpak_Delay", s.delay);
  setGauge("Coolpak_Command_Status", s.command_status);
  setGauge("Coolpak_Running", s.running ? 1.0 : 0.0);
  setGauge("Coolpak_Active_Errors", s.active_errors);
  setGauge("Coolpak_Error_Mask", static_cast<double>(s.error_mask));
  setGauge("Coolpak_Logged_Errors", s.logged_errors);

  if (s.active_errors > 0 || s.error_mask != 0) {
    if (!alarm_fault.failed()) {
      std::string names;
      for (int code = 1; code <= 16; ++code)
        if (s.error_mask & (1u << (code - 1)))
          names += (names.empty() ? "" : ", ") + error_name(code);
      alarm_fault.fail(device_name + ": compressor reports " +
                           std::to_string(s.active_errors) +
                           " active error(s): " + names,
                       labdev::LOG_WARN);
    }
  } else if (alarm_fault.failed()) {
    alarm_fault.clear(device_name + ": no active compressor error");
  }
}

//:::   SYS1 / SYS0. The reply is "SYS" y, y = 0 OFF, 1 ON, 2 ERROR.    :::
bool coolpak::set_system(bool on) {
  const char *what = on ? "SYS1" : "SYS0";
  std::string reply;
  if (!basic_cmd(what, reply)) {
    online = false;
    labdev::log_warn(device_name, std::string("no valid reply to ") + what);
    return false;
  }
  if (reply.size() < 4) {
    labdev::log_warn(device_name,
                     std::string("short reply to ") + what + ": " + reply);
    return false;
  }
  const char y = reply[3];
  if (y == '2') {
    labdev::log_warn(device_name, std::string(what) +
                                      " refused, compressor reports SYSTEM "
                                      "ERROR");
    return false;
  }
  if (y != (on ? '1' : '0')) {
    labdev::log_warn(device_name, std::string(what) +
                                      " not confirmed, compressor said: " +
                                      reply);
    return false;
  }
  labdev::log_info(device_name, on ? "system set ON" : "system set OFF");
  return true;
}

bool coolpak::setCoolerOn() { return set_system(true); }
bool coolpak::setCoolerOff() { return set_system(false); }

bool coolpak::get_logged_errors(std::vector<coolpak_error> &errors) {
  errors.clear();
  std::string reply;
  if (!basic_cmd("ERR", reply))
    return false;
  // "ERR" then entries "xxyyyyy" separated by '/', yyyyy = hours (5 digits).
  std::string body = reply.substr(3);
  size_t start = 0;
  while (start < body.size()) {
    size_t pos = body.find('/', start);
    std::string e = body.substr(
        start, pos == std::string::npos ? std::string::npos : pos - start);
    start = (pos == std::string::npos) ? body.size() : pos + 1;
    if (e.size() < 6)
      continue; // empty or truncated entry
    double code = 0.0, hours = 0.0;
    if (!parse_double(e.substr(0, e.size() - 5), &code) ||
        !parse_double(e.substr(e.size() - 5), &hours))
      return false;
    errors.push_back({static_cast<int>(code), static_cast<long>(hours)});
  }
  return true;
}

void coolpak::printStatus() {
  coolpak_t s;
  if (!get_status(s)) {
    std::cout << "failed to read status" << std::endl;
    return;
  }
  std::cout << "========== COOLPAK STATUS ==========\n";
  std::cout << "Version:          " << s.version << "\n";
  std::cout << "Hours:            " << s.hours << "\n";
  std::cout << "Startup delay:    " << s.delay << " s\n";
  std::cout << "Command status:   "
            << (s.command_status == 0   ? "OFF"
                : s.command_status == 1 ? "ON"
                                        : "SYSTEM ERROR")
            << "\n";
  std::cout << "Compressor:       " << (s.running ? "ON" : "OFF") << "\n";
  std::cout << "Active errors:    " << s.active_errors << "\n";
  for (int code = 1; code <= 16; ++code)
    if (s.error_mask & (1u << (code - 1)))
      std::cout << "    " << error_name(code) << "\n";
  std::cout << "Logged errors:    " << s.logged_errors << "\n";
  std::cout << "====================================\n";
}

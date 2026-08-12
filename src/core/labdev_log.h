#ifndef LABDEV_LOG_H
#define LABDEV_LOG_H

#include <atomic>
#include <string>

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:::   File logging for liblabdev, built on the UICS Log class        :::
//:::   (uics/inst_log.h). One file per day: <path>/yyyymmdd.txt, with :::
//:::   a GMT timestamp on every line. Default path is /var/tmp/labdev/:::
//:::                                                                 :::
//:::   Deliberately coarse. What belongs here: device construction,   :::
//:::   transport open / connect / close, hardware faults, exceptions, :::
//:::   online <-> offline transitions. What does NOT belong here:     :::
//:::   readings, and anything reached from a per-transaction helper    :::
//:::   (serial::lireport / ecrireport / communicate, ethernet read_E  :::
//:::   / write_E, ethernet_device::r_socket / w_socket / checkConnec- :::
//:::   tion) -- those run at the poll rate and would saturate the     :::
//:::   file within hours.                                            :::
//:::                                                                 :::
//:::   For anything reached from a polling loop use log_latch, which  :::
//:::   only writes on the edges of the fault state.                   :::
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

namespace labdev {

//:::   Lower is more important: an entry is kept when its level is    :::
//:::   <= the configured verbosity, matching UICS                     :::
//:::   Log::writeto(text, level). Use -1 to silence the log.          :::
enum loglevel { LOG_ERROR = 0, LOG_WARN = 1, LOG_INFO = 2, LOG_DEBUG = 3 };

// Destination used until log_set_path() says otherwise.
extern const char *const default_log_path; // "/var/tmp/labdev/"

//:::   Creates the directory when it does not exist. Call this before :::
//:::   constructing any device: device constructors already log, and  :::
//:::   those lines land in the default location otherwise.            :::
void log_set_path(const std::string &path);
std::string log_path();

// Default verbosity is LOG_INFO.
void log_set_level(int level);
int log_level();

void log_write(int level, const std::string &msg);

void log_error(const std::string &msg);
void log_warn(const std::string &msg);
void log_info(const std::string &msg);
void log_debug(const std::string &msg);

// Same, prefixed with the device name.
void log_error(const std::string &device, const std::string &msg);
void log_warn(const std::string &device, const std::string &msg);
void log_info(const std::string &device, const std::string &msg);
void log_debug(const std::string &device, const std::string &msg);

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:::   Edge triggered logging, for faults detected inside a polling   :::
//:::   loop. fail() writes only on the transition into the fault      :::
//:::   state, clear() only on the transition out of it, so a device   :::
//:::   failing on every cycle costs two lines in total instead of two :::
//:::   per second. Both return true when a line was actually written. :::
//:::                                                                 :::
//:::   Guard the message construction with failed() when the string   :::
//:::   is built on the hot path:                                     :::
//:::                                                                 :::
//:::     if (fault.failed())                                         :::
//:::       fault.clear(name + ": link restored");                    :::
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
class log_latch {
public:
  log_latch();

  bool fail(const std::string &msg, int level = LOG_ERROR);
  bool clear(const std::string &msg, int level = LOG_INFO);
  bool failed() const;

private:
  std::atomic<bool> failed_;
};

} // namespace labdev

#endif

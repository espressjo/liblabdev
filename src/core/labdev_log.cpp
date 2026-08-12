#include "labdev_log.h"

#include <iostream>
#include <mutex>
#include <uics/inst_log.h>

namespace labdev {

const char *const default_log_path = "/var/tmp/labdev/";

namespace {

// Mirror of the backend verbosity: UICS Log keeps its level private, and the
// read has to be cheap enough to sit in front of the mutex.
std::atomic<int> current_level{LOG_INFO};

// The UICS Log is not thread safe: writeto() reopens the file in append mode
// on every call, so two threads can interleave inside a single line.
// liblabdev writes from the DeviceManager thread, from device threads (the
// EtherCAT gauge has its own) and from the consumer's thread.
std::mutex &log_mutex() {
  static std::mutex m;
  return m;
}

//:::   Function local static, so the first log call from any thread    :::
//:::   creates and configures the backend. Device constructors log     :::
//:::   before the consumer gets a chance to call log_set_path().       :::
//:::   Intentionally leaked: destroying it at exit would race against  :::
//:::   device threads that are still shutting down.                    :::
Log &backend() {
  static Log *lg = [] {
    Log *l = new Log();
    l->setPath(default_log_path); // creates the directory when missing
    l->setLevel(current_level.load());
    if (!l->isFolder(l->path)) {
      std::cerr << "ERROR labdev: cannot create log directory " << l->path
                << ", file logging is disabled" << std::endl;
    }
    return l;
  }();
  return *lg;
}

const char *tag(int level) {
  switch (level) {
  case LOG_ERROR:
    return "[labdev][error] ";
  case LOG_WARN:
    return "[labdev][warn]  ";
  case LOG_INFO:
    return "[labdev][info]  ";
  default:
    return "[labdev][debug] ";
  }
}

} // namespace

void log_set_path(const std::string &path) {
  std::lock_guard<std::mutex> lock(log_mutex());
  backend().setPath(path);
  if (!backend().isFolder(backend().path)) {
    std::cerr << "ERROR labdev: cannot create log directory " << path
              << ", file logging is disabled" << std::endl;
  }
}

std::string log_path() {
  std::lock_guard<std::mutex> lock(log_mutex());
  return backend().path;
}

void log_set_level(int level) {
  std::lock_guard<std::mutex> lock(log_mutex());
  current_level = level;
  backend().setLevel(level);
}

int log_level() { return current_level.load(); }

void log_write(int level, const std::string &msg) {
  // Early out before locking: keeps a silenced or filtered call cheap enough
  // to be harmless if it ever ends up on a hot path.
  if (level > current_level.load()) {
    return;
  }
  std::lock_guard<std::mutex> lock(log_mutex());
  backend().writeto(tag(level) + msg, level);
}

void log_error(const std::string &msg) { log_write(LOG_ERROR, msg); }
void log_warn(const std::string &msg) { log_write(LOG_WARN, msg); }
void log_info(const std::string &msg) { log_write(LOG_INFO, msg); }
void log_debug(const std::string &msg) { log_write(LOG_DEBUG, msg); }

void log_error(const std::string &device, const std::string &msg) {
  log_write(LOG_ERROR, device + ": " + msg);
}
void log_warn(const std::string &device, const std::string &msg) {
  log_write(LOG_WARN, device + ": " + msg);
}
void log_info(const std::string &device, const std::string &msg) {
  log_write(LOG_INFO, device + ": " + msg);
}
void log_debug(const std::string &device, const std::string &msg) {
  log_write(LOG_DEBUG, device + ": " + msg);
}

log_latch::log_latch() : failed_(false) {}

bool log_latch::fail(const std::string &msg, int level) {
  // exchange() makes the edge detection race free, so concurrent callers
  // cannot both emit the entry.
  if (failed_.exchange(true)) {
    return false;
  }
  log_write(level, msg);
  return true;
}

bool log_latch::clear(const std::string &msg, int level) {
  if (!failed_.exchange(false)) {
    return false;
  }
  log_write(level, msg);
  return true;
}

bool log_latch::failed() const { return failed_.load(); }

} // namespace labdev

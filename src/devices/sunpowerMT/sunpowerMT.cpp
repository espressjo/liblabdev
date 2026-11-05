#include "sunpowerMT.h"
#include "labdevices.h"
#include <uics/parse_number.h>
#include "serial.h"
#include <ctime>
#include <prometheus/registry.h>
#include <uics/uics_config.h>

sunpowerMT::sunpowerMT(std::string configfile, std::string devicename,
                       prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry) {
  sunpowerMT::port = cfg_get_string(configfile, "DEVICE", "/dev/ttyACM0");
  sunpowerMT::baudrate = cfg_get_int(configfile, "BAUDRATE", 9600);
  ignore_cariage_return = false;
  init_gauges();
  status = ouvrirport(sunpowerMT::port, sunpowerMT::baudrate);
  if (status != OK) {
    labdev::log_error(device_name, "cannot open " + sunpowerMT::port + " at " +
                                       std::to_string(sunpowerMT::baudrate) +
                                       " baud (from " + configfile + ")");
  }
}

sunpowerMT::sunpowerMT(std::string port, int baudrate, std::string devicename,
                       prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry) {
  sunpowerMT::port = port;
  sunpowerMT::baudrate = baudrate;
  ignore_cariage_return = false;
  init_gauges();
  status = ouvrirport(port, baudrate);
  if (status != OK) {
    labdev::log_error(device_name, "cannot open " + port + " at " +
                                       std::to_string(baudrate) + " baud");
  }
}

// Registers the gauges that update() actually sets.
void sunpowerMT::init_gauges() {
  timeout = -1;
  addGauge("Power_Measured", "sunpower measured power (W)");
  addGauge("Power_Commanded", "sunpower commanded power (W)");
  addGauge("Coldhead_Temperature", "sunpower coldhead temperature");
  addGauge("Reject_Temperature", "sunpower reject temperature");
  addGauge("Target_Temperature", "sunpower target temperature");
  addGauge("Cooler", "sunpower cooler state (1 = ON, 0 = OFF)");
}

void sunpowerMT::set_timeout(int to_s) { timeout = to_s; }
bool sunpowerMT::cold_finger_temperature(double *temperature) {

  std::string buff = "";
  std::cout << "[debug] " << "sending command TC\r" << std::endl;
  ecrireport("TC\r");

  std::cout << "[debug] " << "reading port: " << port << std::endl;
  lireport(&buff, 128);

  std::cout << "[debug] " << "buffer: " << buff << std::endl;
  return true;
}

std::string sunpowerMT::trim(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  size_t end = str.find_last_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  return str.substr(start, end - start + 1);
}

sunpowerMT_t sunpowerMT::parseStatus(const std::string &input) {
  sunpowerMT_t data = {};
  std::istringstream stream(input);
  std::string line;

  while (std::getline(stream, line)) {
    // Check if line contains '='
    size_t pos = line.find('=');
    if (pos == std::string::npos)
      continue;

    // Split into key and value
    std::string key = trim(line.substr(0, pos));
    std::string value = trim(line.substr(pos + 1));

    // Parse based on key
    if (key == "Power Measured") {
      parse_double(value, &data.power_measured);
    } else if (key == "Power Commanded") {
      parse_double(value, &data.power_commanded);
    } else if (key == "Target Temp") {
      parse_double(value, &data.target_t);
    } else if (key == "Reject Temp") {
      parse_double(value, &data.reject_t);
    } else if (key == "Coldhead Temp") {
      parse_double(value, &data.coldhead_t);
    }
  }

  return data;
}

void sunpowerMT::update() {
  sunpowerMT_t status_t;
  if (get_status(status_t)) {
    setGauge("Power_Measured", status_t.power_measured);

    setGauge("Power_Commanded", status_t.power_commanded);
    setGauge("Coldhead_Temperature", status_t.coldhead_t);
    setGauge("Reject_Temperature", status_t.reject_t);
    setGauge("Target_Temperature", status_t.target_t);
    setGauge("Cooler", status_t.cooler_status);

    online = true;
  } else {
    online = false;
  }
}

bool sunpowerMT::get_status(sunpowerMT_t &s_t) {
  std::string buff = "", buffer_cooler;

  if (!basic_cmd("status", buff) || !basic_cmd("COOLER", buffer_cooler)) {
    online = false;
    return false;
  }

  s_t = parseStatus(buff);
  if (buffer_cooler.find("ON") != std::string::npos) {
    s_t.cooler = true;
    s_t.cooler_status = 1.0;
  } else {
    s_t.cooler = false;
    s_t.cooler_status = 0.0;
  }

  online = true;
  return true;
}
void sunpowerMT::printStatus() {
  sunpowerMT_t data;
  if (!get_status(data)) {
    std::cout << "failed to read status" << std::endl;
  }
  std::cout << "========== STATUS DATA ==========\n";
  std::cout << "Power Measured:        " << data.power_measured << "\n";
  std::cout << "COOLER:     " << (data.cooler ? "ON" : "OFF") << "\n";
  std::cout << "Power Commanded:  " << data.power_commanded << "\n";
  std::cout << "Target Temperature:         " << data.target_t << "\n";
  std::cout << "Reject Temperature:         " << data.reject_t << "\n";
  std::cout << "Coldhead Temerature:         " << data.coldhead_t << "\n";
  std::cout << "=================================\n";
}
bool sunpowerMT::basic_cmd(std::string cmd, std::string &ret) {
  // make sure \r
  std::string original_cmd = cmd;
  if (cmd.substr(cmd.length() - 1, 1).compare("\r") != 0) {
    cmd += "\r";
  }
  if (ecrireport(cmd) != OK) {
    std::cerr << "failed to send command on port: " << port << std::endl;
  }
  std::string buff = "";

  if (timeout > 0) {
    if (lireport(&buff, MAX_CHAR, timeout) < (int)(cmd.length() - 1)) {
      std::cerr << "Received: " << buff
                << ". Expecting longer reply from sunpower" << std::endl;
      return false;
    }

  } else {
    if (lireport(&buff, MAX_CHAR) < (int)(cmd.length() - 1)) {
      std::cerr << "Received: " << buff
                << ". Expecting longer reply from sunpower" << std::endl;
      return false;
    }
  }
  if (buff.substr(0, original_cmd.length()).compare(original_cmd) != 0) {
    std::cerr << "Expecting to receive: " << original_cmd
              << ". Received: " << buff.substr(0, buff.find('\r')) << std::endl;
    return false;
  }

  ret = buff.substr(buff.find('\r') + 1, buff.length());
  return true;
}
bool sunpowerMT::setCoolerOn() {
  std::string buffer = "";
  if (!basic_cmd("COOLER=ON", buffer)) {
    online = false;
    labdev::log_warn(device_name, "no reply to COOLER=ON");
    return false;
  }
  if (buffer.find("ON") == std::string::npos) {
    labdev::log_warn(device_name,
                     "COOLER=ON not confirmed, controller said: " + buffer);
    return false;
  }
  labdev::log_info(device_name, "cooler set ON");
  return true;
}

bool sunpowerMT::setCoolerOff() {
  std::string buffer = "";
  if (!basic_cmd("COOLER=OFF", buffer)) {
    online = false;
    labdev::log_warn(device_name, "no reply to COOLER=OFF");
    return false;
  }
  if (buffer.find("OFF") == std::string::npos) {
    labdev::log_warn(device_name,
                     "COOLER=OFF not confirmed, controller said: " + buffer);
    return false;
  }
  labdev::log_info(device_name, "cooler set OFF");
  return true;
}

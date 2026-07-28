#include "sunpowerMT.h"
#include "labdevices.h"
#include "serial.h"
#include <ctime>
#include <prometheus/registry.h>

sunpowerMT::sunpowerMT(std::string port, int baudrate, std::string devicename,
                   prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry) {
  sunpowerMT::port = port;
  sunpowerMT::baudrate = baudrate;
  ignore_cariage_return = false;
  status = ouvrirport(port, baudrate);

  timeout = -1;
  addGauge("VDC", "DC voltage of sunpower");

  addGauge("POWER_CMD", "sunpower");
  addGauge("TC", "sunpower");
  addGauge("RT", "sunpower");
  addGauge("WT", "sunpower");
  addGauge("VrmsOutA", "sunpower");
  addGauge("IrmsOutA", "sunpower");
  addGauge("PA", "sunpower");

  addGauge("PB", "sunpower");
  addGauge("VrmsOutB", "sunpower");
  addGauge("IrmsOutB", "sunpower");
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
      data.power_measured = std::stod(value);
    } else if (key == "Power Commanded") {
      data.power_commanded = std::stod(value);
    } else if (key == "Target Temp") {
      data.target_t = std::stod(value);
    } else if (key == "Reject Temp") {
      data.reject_t = std::stod(value);
    } else if (key == "Coldhead Temp") {
      data.coldhead_t = std::stod(value);
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

  if (!basic_cmd("status", buff)) {
    online = false;
  }
  if (!basic_cmd("COOLER", buffer_cooler)) {
    online = false;
  }

  if (buffer_cooler.find("ON") != std::string::npos) {
  }

  s_t = parseStatus(buff);
  if (buffer_cooler.find("ON") != std::string::npos) {
    s_t.cooler = true;
    s_t.cooler_status = 1.0;
  } else {
    s_t.cooler = false;
    s_t.cooler_status = 0.0;
  }

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
bool sunpowerMT::set_cooler_on() {
  std::string buffer = "";
  if (!basic_cmd("COOLER=ON", buffer)) {
    online = false;
    return false;
  }
  if (buffer.find("ON") != std::string::npos) {
    return true;
  } else {
    return false;
  }
}
bool sunpowerMT::set_cooler_off() {
  std::string buffer = "";
  if (!basic_cmd("COOLER=OFF", buffer)) {
    online = false;
    return false;
  }
  if (buffer.find("OFF") != std::string::npos) {
    return true;
  } else {
    return false;
  }
}

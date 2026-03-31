#include "sunpowerDS30.h"
#include "labdevices.h"
#include "serial.h"
#include "sunpowerDS30_t.h"
#include <ctime>
#include <prometheus/registry.h>

sunpowerDS30::sunpowerDS30(std::string port, int baudrate, std::string devicename,
                   prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry) {
  sunpowerDS30::port = port;
  sunpowerDS30::baudrate = baudrate;
  ignore_cariage_return = false;
  status = ouvrirport(port, baudrate);

  /*
   double VDC;
    double POWER_CMD;
    double TC;
    double RT;
    double WT;
    double VrmsOutA;
    double IrmsOutA;
    double PA;
    double VrmsOutB;
    double IrmsOutB;
    double PB;
    */
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

void sunpowerDS30::set_timeout(int to_s) { timeout = to_s; }
bool sunpowerDS30::cold_finger_temperature(double *temperature) {

  std::string buff = "";
  std::cout << "[debug] " << "sending command TC\r" << std::endl;
  ecrireport("TC\r");

  std::cout << "[debug] " << "reading port: " << port << std::endl;
  lireport(&buff, 128);

  std::cout << "[debug] " << "buffer: " << buff << std::endl;
  return true;
}

std::string sunpowerDS30::trim(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  size_t end = str.find_last_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  return str.substr(start, end - start + 1);
}

sunpower_t sunpowerDS30::parseStatus(const std::string &input) {
  sunpower_t data = {};
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
    if (key == "VDC") {
      data.VDC = std::stod(value);
    } else if (key == "COOLER") {
      data.COOLER = (value == "ON");
    } else if (key == "POWER CMD") {
      data.POWER_CMD = std::stod(value);
    } else if (key == "TC") {
      data.TC = std::stod(value);
    } else if (key == "RT") {
      data.RT = std::stod(value);
    } else if (key == "WT") {
      data.WT = std::stod(value);
    } else if (key == "VrmsOutA") {
      data.VrmsOutA = std::stod(value);
    } else if (key == "IrmsOutA") {
      data.IrmsOutA = std::stod(value);
    } else if (key == "PA") {
      data.PA = std::stod(value);
    } else if (key == "VrmsOutB") {
      data.VrmsOutB = std::stod(value);
    } else if (key == "IrmsOutB") {
      data.IrmsOutB = std::stod(value);
    } else if (key == "PB") {
      data.PB = std::stod(value);
    } else if (key == "RUNTIME") {
      data.RUNTIME = value;
    } else if (key == "UPTIME") {
      data.UPTIME = value;
    } else if (key == "C ERROR") {
      data.C_ERROR = value;
    } else if (key == "NC ERROR") {
      data.NC_ERROR = value;
    }
  }

  return data;
}

void sunpowerDS30::update() {
  sunpower_t status_t;

  /*
  double VDC;

      double POWER_CMD;
      double TC;
      double RT;
      double WT;
      double VrmsOutA;
      double IrmsOutA;
      double PA;
      double VrmsOutB;
      double IrmsOutB;
      double PB;
      */

  if (get_status(status_t)) {
    setGauge("VDC", status_t.VDC);

    setGauge("POWER_CMD", status_t.POWER_CMD);
    setGauge("TC", status_t.TC);
    setGauge("RT", status_t.RT);
    setGauge("WT", status_t.WT);
    setGauge("VrmsOutA", status_t.VrmsOutA);
    setGauge("VrmsOutB", status_t.VrmsOutB);
    setGauge("IrmsOutA", status_t.IrmsOutA);
    setGauge("IrmsOutB", status_t.IrmsOutB);
    setGauge("PA", status_t.PA);
    setGauge("PB", status_t.PB);

    online = true;
  } else {
    online = false;
  }
}

bool sunpowerDS30::get_status(sunpower_t &s_t) {
  std::string buff = "";
  if (!basic_cmd("status", buff)) {
    online = false;
  }

  /*
    if (timeout > 0) {
      if (lireport(&buff, 2048, timeout) < 400) {
        return false;
      }

    } else {
      if (lireport(&buff, 2048) < 400) {
        return false;
      }
    }*/
  s_t = parseStatus(buff);
  return true;
}
void sunpowerDS30::printStatus() {
  sunpower_t data;
  if (!get_status(data)) {
    std::cout << "failed to read status" << std::endl;
  }
  std::cout << "========== STATUS DATA ==========\n";
  std::cout << "VDC:        " << data.VDC << "\n";
  std::cout << "COOLER:     " << (data.COOLER ? "ON" : "OFF") << "\n";
  std::cout << "POWER CMD:  " << data.POWER_CMD << "\n";
  std::cout << "TC:         " << data.TC << "\n";
  std::cout << "RT:         " << data.RT << "\n";
  std::cout << "WT:         " << data.WT << "\n";
  std::cout << "VrmsOutA:   " << data.VrmsOutA << "\n";
  std::cout << "IrmsOutA:   " << data.IrmsOutA << "\n";
  std::cout << "PA:         " << data.PA << "\n";
  std::cout << "VrmsOutB:   " << data.VrmsOutB << "\n";
  std::cout << "IrmsOutB:   " << data.IrmsOutB << "\n";
  std::cout << "PB:         " << data.PB << "\n";
  std::cout << "RUNTIME:    " << data.RUNTIME << "\n";
  std::cout << "UPTIME:     " << data.UPTIME << "\n";
  std::cout << "C ERROR:    " << data.C_ERROR << "\n";
  std::cout << "NC ERROR:   " << data.NC_ERROR << "\n";
  std::cout << "=================================\n";
}
bool sunpowerDS30::AVC(bool status) {
  std::string buff = "";
  if (status) {
    return basic_cmd("VIBE ON", buff);
  } else {
    return basic_cmd("VIBE OFF", buff);
  }
}
bool sunpowerDS30::get_vibration() {
  std::string buff = "";
  if (!basic_cmd("VIBE", buff)) {
    std::cout << "oups!!!" << std::endl;
    return false;
  }

  return true;
}
bool sunpowerDS30::basic_cmd(std::string cmd, std::string &ret) {
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
    if (lireport(&buff, MAX_CHAR, timeout) < cmd.length() - 1) {
      std::cerr << "Received: " << buff
                << ". Expecting longer reply from sunpower" << std::endl;
      return false;
    }

  } else {
    if (lireport(&buff, MAX_CHAR) < cmd.length() - 1) {
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

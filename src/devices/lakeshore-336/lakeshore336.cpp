#include "lakeshore336.h"
#include "labdevices.h"
lakeshore336::lakeshore336(std::string IP, int port, std::string devicename,
                           bool extended = true,
                           prometheus::Registry *registry = nullptr)
    : ethernet_device(devicename, registry)
/*
 * Open socket communication.
 */
{
  if (!connect(IP, port)) {

    std::cout << "[debug] Unable to open connection with lakeshore 336"
              << std::endl;
  }
  lakeshore336::extended = extended;
  addGauge("A", "Temperature input A (kelvin)");

  addGauge("B", "Temperature input B (kelvin)");
  addGauge("C", "Temperature input c (kelvin)");
  if (extended) {
    addGauge("D1", "Temperature input D1 (kelvin)");
    addGauge("D2", "Temperature input D2 (kelvin)");
    addGauge("D3", "Temperature input D3 (kelvin)");
    addGauge("D4", "Temperature input D4 (kelvin)");

    addGauge("D5", "Temperature input D5 (kelvin)");
  } else {
    addGauge("D", "Temperature input D (kelvin)");
  }

  addGauge("setpoint1", "Setpoint 1");

  addGauge("setpoint2", "Setpoint 2");
  addGauge("setpoint3", "Setpoint 3");
  addGauge("setpoint4", "Setpoint 4");

  addGauge("heater1", "Heater 1 (%)");
  addGauge("heater2", "Heater #2 (%)");

  addGauge("analogueOut1", "Analogue output #1 (%)");
  addGauge("analogueOut2", "Analogue Output #2 (%)");
}
lakeshore336::lakeshore336(std::string configfile, std::string devicename = "",
                           int timeout = -1, bool extended = true,
                           prometheus::Registry *registry = nullptr)
    : ethernet_device(devicename, registry)
/*
 * Open socket communication.
 */
{

  lakeshore336::extended = extended;
  int port = cfg_get_int(configfile, "LSPORT", 7777);
  std::string IP = cfg_get_string(configfile, "LSIP", "127.0.0.1");
  std::cout << "[debug] " << IP << std::endl;
  // timeout in second
  if (!connect(IP, port)) {

    std::cout << "[debug] Unable to open connection with lakeshore 336"
              << std::endl;
  }

  addGauge("A", "Temperature input A (kelvin)");

  addGauge("B", "Temperature input B (kelvin)");
  addGauge("C", "Temperature input c (kelvin)");
  if (extended) {
    addGauge("D1", "Temperature input D1 (kelvin)");
    addGauge("D2", "Temperature input D2 (kelvin)");
    addGauge("D3", "Temperature input D3 (kelvin)");
    addGauge("D4", "Temperature input D4 (kelvin)");

    addGauge("D5", "Temperature input D5 (kelvin)");
  } else {
    addGauge("D", "Temperature input D (kelvin)");
  }

  addGauge("setpoint1", "Setpoint 1");

  addGauge("setpoint2", "Setpoint 2");
  addGauge("setpoint3", "Setpoint 3");
  addGauge("setpoint4", "Setpoint 4");

  addGauge("heater1", "Heater 1 (%)");
  addGauge("heater2", "Heater #2 (%)");

  addGauge("analogueOut1", "Analogue output #1 (%)");
  addGauge("analogueOut2", "Analogue Output #2 (%)");
}

void lakeshore336::update() {
  online = true;
  double dummy = 0.0;
  if (readk("KRDG? A", &dummy)) {
    setGauge("A", dummy);
  } else {
    online = false;
  }
  if (readk("KRDG? B", &dummy)) {
    setGauge("B", dummy);
  } else {
    online = false;
  }
  if (readk("KRDG? C", &dummy)) {
    setGauge("C", dummy);
  } else {
    online = false;
  }
  if (!extended) {
    if (readk("KRDG? D", &dummy)) {
      setGauge("D", dummy);
    } else {
      online = false;
    }
  } else {
    if (readk("KRDG? D1", &dummy)) {
      setGauge("D1", dummy);
    } else {
      online = false;
    }
    if (readk("KRDG? D2", &dummy)) {
      setGauge("D2", dummy);
    } else {
      online = false;
    }
    if (readk("KRDG? D3", &dummy)) {
      setGauge("D3", dummy);
    } else {
      online = false;
    }
    if (readk("KRDG? D4", &dummy)) {
      setGauge("D4", dummy);
    } else {
      online = false;
    }
    if (readk("KRDG? D5", &dummy)) {
      setGauge("D5", dummy);
    } else {
      online = false;
    }
  }

  if (readk("HTR? 1", &dummy)) {
    setGauge("heater1", dummy);
  } else {
    online = false;
  }
  if (readk("HTR? 2", &dummy)) {
    setGauge("heater2", dummy);
  } else {
    online = false;
  }
  if (readk("AOUT? 1", &dummy)) {
    setGauge("analogueOut1", dummy);
  } else {
    online = false;
  }
  if (readk("AOUT? 2", &dummy)) {
    setGauge("analogueOut2", dummy);
  } else {
    online = false;
  }
  if (readk("SETP? 1", &dummy)) {
    setGauge("setpoint1", dummy);
  } else {
    online = false;
  }

  if (readk("SETP? 2", &dummy)) {
    setGauge("setpoint2", dummy);
  } else {
    online = false;
  }
  if (readk("SETP? 3", &dummy)) {
    setGauge("setpoint3", dummy);
  } else {
    online = false;
  }
  if (readk("SETP? 4", &dummy)) {
    setGauge("setpoint4", dummy);
  } else {
    online = false;
  }
}
bool lakeshore336::readk(std::string cmd, double *value)

/*
 * Read input temperature A in Kelvin
 */
{
  std::string rsp = "";
  *value = 0.0;
  if (cmd.substr(cmd.length() - 1, 1).compare("\n") != 0) {
    cmd += "\n";
  }

  wr_socket(cmd, &rsp);

  if (find_double(rsp, value) < 0) {
    status = NOK;
    return false;
  }
  return true;
}

void lakeshore336::close(void)
/*
 * Close socket communication.
 */
{
  close_socket();
}
std::string lakeshore336::strip(std::string str)
/*
 * Strip \n from a string
 */
{
  if (str.substr(str.length() - 1, 1).compare("\n") == 0) {
    return str.substr(0, str.length() - 1);
  }
  return str;
}
std::string lakeshore336::idn()
/*
 * Return the model of the lakeshore.
 */
{
  std::string rsp = "";
  write_E("*IDN?\n");
  status = read_E(&rsp);
  return strip(rsp);
}
int lakeshore336::find_double(std::string input, double *T) {
  std::string buff = "";
  if (input.length() < 2) {
    *T = 0.0;
    return NOK;
  }
  for (auto &c : input) {
    if (std::isdigit(c) || c == '.' || c == '-') {
      buff += c;
    }
  }
  if (buff.length() < 2) {
    *T = 0.0;
    return NOK;
  }
  *T = std::stod(buff);
  return OK;
}
double lakeshore336::read_aout3()
/*
 * Read input temperature A in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("AOUT? 3\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}

double lakeshore336::read_aout4()
/*
 * Read input temperature A in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("AOUT? 4\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::readk_A()
/*
 * Read input temperature A in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? A\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::readk_B()
/*
 * Read input temperature B in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? B\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::setp_4()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("SETP? 4\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::setp_3()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("SETP? 3\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::setp_2()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("SETP? 2\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::setp_1()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("SETP? 1\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::htr_2()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("HTR? 2\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::htr_1()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("HTR? 1\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::readk_C()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? C\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::readk_D1()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? D1\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::readk_D2()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? D2\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
// bool lakeshore336::readAll(reading *T) { ; }
double lakeshore336::readk_D3()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? D3\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::readk_D4()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? D4\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}
double lakeshore336::readk_D5()
/*
 * Read input temperature in Kelvin
 */
{
  std::string rsp = "";
  double T = 0.0;
  write_E("KRDG? D5\n");
  read_E(&rsp);
  if (find_double(rsp, &T) < 0) {
    status = NOK;
  }
  return T;
}

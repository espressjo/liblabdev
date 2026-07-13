#ifndef LIM_SWITCH_H
#define LIM_SWITCH_H

#include <LabJackM.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread> // std::this_thread::sleep_for

using namespace std;

typedef struct {
  float nutec;
  float rm8;
  float TTM;
  float bench;
  float nose;
  float base;
  float nutec_ctrl;
} temperature;

class ljack {
public:
  ljack(string device, string connection_type, int serial_nb);
  ~ljack();
  void print_status();
  bool connected;
  int read_temperature(temperature *tmp);
  int dio_1(std::string dio_name);
  int dio_0(std::string dio_name);
  bool read_DIO(std::string dio_name);

private:
  int handle;
};

void read_temps_t(instHandle *handle);
double lmt85_2_temperature(double voltage);
#endif

/*
serial: 470015647
Model: T7
Connection: Ethernet/USB/WiFi
*/

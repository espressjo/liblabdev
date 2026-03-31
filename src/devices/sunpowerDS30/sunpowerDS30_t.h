#ifndef SUNPOWER_T
#define SUNPOWER_T

#include <string>
typedef struct {
  double VDC;
  bool COOLER;
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
  std::string RUNTIME;
  std::string UPTIME;
  std::string C_ERROR;
  std::string NC_ERROR;
} sunpower_t;
#endif

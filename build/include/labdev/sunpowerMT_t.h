#ifndef SUNPOWER_T
#define SUNPOWER_T

#include <string>
typedef struct {
  double power_measured;
  double power_commanded;
  double target_t;
  double reject_t;
  double coldhead_t;
  double cooler_status;
  bool cooler;

} sunpowerMT_t;
#endif

#ifndef INSTHANDLE_H
#define INSTHANDLE_H
#include "devicesManager.h"
#include <uics/states.h>
#include <string>

typedef struct {
  STATE nextState;
  STATE state;
  STATUS status;
  int loglevel;
  DeviceManager *devManager;

} instHandle;

#endif

#ifndef LABJACK_H
#define LABJACK_H
#include "labdev.h"
#include <LabJackM.h>
#include <string>
#include <unistd.h> /* read, write, close */

#define NOK -1 /* valeur de retour avec erreur */
#define OK 0

class labjack : public labdevices {

public:
  labjack(std::string configfile, std::string devicename,
          prometheus::Registry *registry);

  int readA01(double *value);
  int readA00(double *value);
  void update();

private:
  std::string IP;
  std::string devicename;
  int error, lj_handle;
};

#endif // LAKESHORE_RS232_H

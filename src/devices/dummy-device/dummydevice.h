#ifndef DUMMYDEVICE_H
#define DUMMYDEVICE_H
#include "labdev.h"
#include "labdevices.h"

#include <stdlib.h>
#include <string>
#include <unistd.h> /* read, write, close */

#define NOK -1 /* valeur de retour avec erreur */
#define OK 0

class dummydevice : public labdevices {

public:
  dummydevice(std::string devicename, prometheus::Registry *registry);
  int read_dummy(double *value, double noise = 0.6);
  void update();
  // void set_timeout(int to_s);

private:
  int timeout;
  int strip_ack(std::string ack, std::string *val);
};

#endif // LAKESHORE_RS232_H

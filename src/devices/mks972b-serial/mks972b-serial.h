#ifndef MKS972B_H
#define MKS972B_H
#include "labdev.h"
#include "labdev_log.h"
#include "labdevices.h"
#include <ctype.h> /* tolower */
#include <fcntl.h> /* open */

#include <stdio.h> /* fprintf */
#include <stdlib.h>
#include <string.h> /* strcpy, strlen */
#include <string>
#include <termios.h>  /* ioctl */
#include <unistd.h>   /* read, write, close */
#define DELAIMAX 5000 /* delai maximum pour l'attente en ms*/

#define NOK -1 /* valeur de retour avec erreur */
#define OK 0

class gauge972 : public labdevices, private serial {

public:
  gauge972(std::string configfile, std::string devicename,
           prometheus::Registry *registry);
  gauge972(int port, int baudrate, std::string devicename,
           prometheus::Registry *registry);
  int read_pressure(double *pressure);
  int read_pirani(double *pressure);
  int read_cold_cathod(double *pressure);
  void update();
  void set_timeout(int to_s);
  // Last pressure read by update(), same public member as gauge972_e.
  double pressure;

private:
  void init_gauge();
  int timeout;
  // Gauge name, "<devicename>_Pressure", same as gauge972_e.
  std::string p_gauge;
  labdev::log_latch reading_fault;
  int strip_ack(std::string ack, std::string *val);
};

#endif // LAKESHORE_RS232_H

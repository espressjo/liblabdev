#ifndef LAKESHORE336_H
#define LAKESHORE336_H
#include "labdev.h"
#include "labdevices.h"
#include <ctype.h> /* tolower */
#include <fcntl.h> /* open */
#include <stdio.h> /* fprintf */
#include <stdlib.h>
#include <string.h> /* strcpy, strlen */
#include <string>
#include <termios.h> /* ioctl */
#include <unistd.h> /* read, write, close */
#include <uics/uics_config.h>
class lakeshore336 : public ethernet_device {
public:
  lakeshore336(std::string configfile, std::string devicename, int timeout,
               bool extended, prometheus::Registry *registry);
  lakeshore336(std::string devicename, bool extended,
               prometheus::Registry *registry);

  void close(void);
  std::string idn();
  double readk_A();
  double readk_B();
  double readk_C();
  double readk_D1();
  double readk_D2();
  double readk_D3();
  double readk_D4();
  double readk_D5();
  double htr_1();  // read htr %
  double htr_2();  // read htr %
  double setp_1(); // read setp 1
  double setp_2(); // read setp 2
  double setp_3(); // read setp 3
  double setp_4(); // read setp 4
  double read_aout3();
  double read_aout4();
  int status;
  void update();

private:
  bool extended;
  std::string strip(std::string str);
  int find_double(std::string input, double *T);

  bool readk(std::string cmd, double *value);
};

#endif

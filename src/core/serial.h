#ifndef SERIAL_H
#define SERIAL_H
#include "states_io.h" /* read, write, close */
#include <chrono>
#include <ctype.h> /* tolower */
#include <fcntl.h> /* open */
#include <iostream>
#include <stdio.h> /* fprintf */
#include <stdlib.h>
#include <string.h> /* strcpy, strlen */
#include <string>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#define DELAIMAX 5000 /* delai maximum pour l'attente en ms*/
#define MAX_CHAR 4096 /* nombre de charactere maximal à lire*/

enum exchangerval { VALID = 0, NOTVALID, EMPTY };
class serial {
public:
  serial();
  int ouvrirport(int port, int vitesse);

  int lireport(std::string *str, int ncar, int timeout_sec);
  int ouvrirport(std::string port, int vitesse);
  int configport();
  int fermerport();
  int ecrireport(std::string str);
  int lireport(std::string *str, int ncar);
  int lirec(std::string *str, char c);
  int status;
  int set_usb(void);
  std::string communicate(std::string cmd, int ncar = MAX_CHAR,
                          bool strip = false);
  int vmin;
  int vtime;
  bool set_CTSRTS_flow_ctrl;
  bool set_XON;               // = false;
  bool set_XOFF;              // = false;
  bool set_DTR_HI;            // = false;
  bool ignore_cariage_return; // True by default

private:
  struct termios serie; /* pour la configuration du port serie */
  int idport;
  bool usb;
  int fd;
  std::string conversion(std::string ss);
  void delay(int ms);
  std::chrono::steady_clock::time_point last_call_time;

protected:
  int waitfor(const char a, int timeout);
  exchangerval exchange_status;
  int ncar_read;
  int call_delay; // delay between write and read ms
};

#endif // LAKESHORE_RS232_H

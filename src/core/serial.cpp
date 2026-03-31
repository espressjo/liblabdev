#include "serial.h"

serial::serial() : last_call_time(std::chrono::steady_clock::now()) {
  idport = 0;
  fd = 0;
  status = 0;
  usb = false;
  exchange_status = NOTVALID;
  call_delay = 50; // 50 ms delay
  vmin = 0;
  ncar_read = 0;
  vtime = 1;
  set_XON = false;
  set_XOFF = false;
  set_DTR_HI = false;
  ignore_cariage_return = true;
  set_CTSRTS_flow_ctrl = false;
  /*
   * In the context of a serial terminal or device,
   * enabling CRTSCTS ensures that data flow is
   * controlled and that the devices communicate readiness
   * before transmission happens.
   */
}
int serial::set_usb() {
  if (fd == 0) {
    usb = true;
    return OK;
  } else {
    return NOK;
  }
}

int serial::ouvrirport(
    std::string port,
    int vitesse) /* port = "0", "1" ou "2" , Vallee fev. 2012 */

{
  std::string fichier = "";

  if ((fd = open(port.c_str(), O_RDWR | O_NOCTTY)) == NOK) {
    fprintf(stderr, "Erreur lors de l'ouverture du port serie %s, fd =%d .\n",
            port.c_str(), fd);
    status = NOK;
    return NOK;
  }

  if (ignore_cariage_return) {
    serie.c_iflag = IGNCR; /* ignore <CR> */
  }

  else {
    serie.c_iflag &= ~IGNCR;
  }
  if (set_XON) {

    serie.c_iflag |= IXON; /* ignore <CR> */
  } else {
    serie.c_iflag &= ~IXON;
  }
  if (set_XOFF) {

    serie.c_iflag |= IXOFF; /* ignore <CR> */
  }

  else {
    serie.c_iflag &= ~IXOFF;
  }
  serie.c_oflag = 0;
  serie.c_cflag = CS8 | CREAD | CLOCAL;

  if (set_CTSRTS_flow_ctrl) {
    serie.c_cflag |= CRTSCTS;
  }

  serie.c_lflag = 0;
  serie.c_cc[VMIN] = vmin;   /* nombre minimum de caracteres */
  serie.c_cc[VTIME] = vtime; /* nombre x 0.1 s d'attente max */
                             /* lors d'un appel de read */
                             /*
                             VMIN = 0 and VTIME = 0
                             ----------------------
                                 This is a completely non-blocking read - the call is satisfied immediately
                             directly from the driver's input queue. If data are available, it's
                             transferred to the caller's buffer up to nbytes and returned. Otherwise zero
                             is immediately returned to indicate "no data". We'll note that this is
                             "polling" of the serial port, and it's almost always a bad idea. If done
                             repeatedly, it can consume enormous amounts of processor time and is highly
                             inefficient. Don't use this mode unless you really, really know what you're
                             doing.
                           
                             VMIN = 0 and VTIME > 0
                             -----------------------
                                 This is a pure timed read. If data are available in the input queue, it's
                             transferred to the caller's buffer up to a maximum of nbytes, and returned
                             immediately to the caller. Otherwise the driver blocks until data arrives, or
                             when VTIME tenths expire from the start of the call. If the timer expires
                             without data, zero is returned. A single byte is sufficient to satisfy this
                             read call, but if more is available in the input queue, it's returned to the
                             caller. Note that this is an overall timer, not an intercharacter one.
                           
                             VMIN > 0 and VTIME > 0
                             ----------------------
                                 A read() is satisfied when either VMIN characters have been transferred to
                             the caller's buffer, or when VTIME tenths expire between characters. Since
  this timer is not started until the first character arrives, this call can
                             block indefinitely if the serial line is idle. This is the most common mode of
                             operation, and we consider VTIME to be an intercharacter timeout, not an
                             overall one. This call should never return zero bytes read.
                           
                             VMIN > 0 and VTIME = 0
                             ----------------------
                             This is a counted read that is satisfied only when at least VMIN characters
                             have been transferred to the caller's buffer - there is no timing component
                             involved. This read can be satisfied from the driver's input queue (where the
                             call could return immediately), or by waiting for new data to arrive: in this
                             respect the call could block indefinitely. We believe that it's undefined
                             behavior if nbytes is less then VMIN.
                           
                             */

  speed_t baud;
  switch (vitesse) {
  case 19200:
    baud = B19200;
    break;
  case 115200:
    baud = B115200;
    break;
  case 38400:
    baud = B38400;
    break;
  case 9600:
    baud = B9600;
    break;
  case 4800:
    baud = 4800;
    break;
  case 57600:
    baud = B57600;
    break;
  case 1200:
    baud = B1200;
    break;
  case 600:
    baud = B600;
    break;
  case 300:
    baud = B300;
    break;
  default:
    baud = B9600;
    break;
  }

  if (cfsetispeed(&serie, baud) != 0) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    close(fd);
    return NOK;
  }
  if (cfsetospeed(&serie, baud) != 0) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    close(fd);
    return NOK;
  }

  if (set_DTR_HI) {

    int _status;
    if (ioctl(fd, TIOCMGET, &_status) < 0) {
      fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
      status = NOK;
      close(fd);
      return NOK;
    }
    status |= TIOCM_DTR;
    if (ioctl(fd, TIOCMSET, &_status) < 0) {
      fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
      status = NOK;
      close(fd);
      return NOK;
    }
  }

  if (tcsetattr(fd, TCSANOW, &serie) < 0) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    close(fd);
    return NOK;
  }

  idport = fd;
  status = OPEN;
  last_call_time = std::chrono::steady_clock::now();
  return 0;
}

int serial::ouvrirport(
    int port, int vitesse) /* port = "0", "1" ou "2" , Vallee fev. 2012 */

{
  std::string fichier = "";
  if (usb) {
    fichier = "/dev/ttyUSB" + std::to_string(port);
  } else {
    fichier = "/dev/ttyS" + std::to_string(port);
  }

  if ((fd = open(fichier.c_str(), O_RDWR | O_NOCTTY)) == NOK) {
    fprintf(stderr, "Erreur lors de l'ouverture du port serie %s, fd =%d .\n",
            fichier.c_str(), fd);
    status = NOK;
    return NOK;
  }

  if (ignore_cariage_return) {
    serie.c_iflag = IGNCR; /* ignore <CR> */
  }

  else {
    serie.c_iflag &= ~IGNCR;
  }
  if (set_XON) {

    serie.c_iflag |= IXON; /* ignore <CR> */
  } else {
    serie.c_iflag &= ~IXON;
  }
  if (set_XOFF) {

    serie.c_iflag |= IXOFF; /* ignore <CR> */
  }

  else {
    serie.c_iflag &= ~IXOFF;
  }

  serie.c_oflag = 0;
  serie.c_cflag = CS8 | CREAD | CLOCAL;

  if (set_CTSRTS_flow_ctrl) {
    serie.c_cflag |= CRTSCTS;
  }

  serie.c_lflag = 0;
  serie.c_cc[VMIN] = vmin;   /* nombre minimum de caracteres */
  serie.c_cc[VTIME] = vtime; /* nombre x 0.1 s d'attente max */
                             /* lors d'un appel de read */
                             /*
                             VMIN = 0 and VTIME = 0
                             ----------------------
                                 This is a completely non-blocking read - the call is satisfied immediately
                             directly from the driver's input queue. If data are available, it's
                             transferred to the caller's buffer up to nbytes and returned. Otherwise zero
                             is immediately returned to indicate "no data". We'll note that this is
                             "polling" of the serial port, and it's almost always a bad idea. If done
                             repeatedly, it can consume enormous amounts of processor time and is highly
                             inefficient. Don't use this mode unless you really, really know what you're
                             doing.
                           
                             VMIN = 0 and VTIME > 0
                             -----------------------
                                 This is a pure timed read. If data are available in the input queue, it's
                             transferred to the caller's buffer up to a maximum of nbytes, and returned
                             immediately to the caller. Otherwise the driver blocks until data arrives, or
                             when VTIME tenths expire from the start of the call. If the timer expires
                             without data, zero is returned. A single byte is sufficient to satisfy this
                             read call, but if more is available in the input queue, it's returned to the
                             caller. Note that this is an overall timer, not an intercharacter one.
                           
                             VMIN > 0 and VTIME > 0
                             ----------------------
                                 A read() is satisfied when either VMIN characters have been transferred to
                             the caller's buffer, or when VTIME tenths expire between characters. Since
  this timer is not started until the first character arrives, this call can
                             block indefinitely if the serial line is idle. This is the most common mode of
                             operation, and we consider VTIME to be an intercharacter timeout, not an
                             overall one. This call should never return zero bytes read.
                           
                             VMIN > 0 and VTIME = 0
                             ----------------------
                             This is a counted read that is satisfied only when at least VMIN characters
                             have been transferred to the caller's buffer - there is no timing component
                             involved. This read can be satisfied from the driver's input queue (where the
                             call could return immediately), or by waiting for new data to arrive: in this
                             respect the call could block indefinitely. We believe that it's undefined
                             behavior if nbytes is less then VMIN.
                           
                             */

  speed_t baud;
  switch (vitesse) {
  case 19200:
    baud = B19200;
    break;
  case 115200:
    baud = B115200;
    break;
  case 38400:
    baud = B38400;
    break;
  case 9600:
    baud = B9600;
    break;
  case 4800:
    baud = 4800;
    break;
  case 57600:
    baud = B57600;
    break;
  case 1200:
    baud = B1200;
    break;
  case 600:
    baud = B600;
    break;
  case 300:
    baud = B300;
    break;
  default:
    baud = B9600;
    break;
  }

  if (cfsetispeed(&serie, baud) != 0) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    close(fd);
    return NOK;
  }
  if (cfsetospeed(&serie, baud) != 0) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    close(fd);
    return NOK;
  }

  if (set_DTR_HI) {

    int _status;
    if (ioctl(fd, TIOCMGET, &_status) < 0) {
      fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
      status = NOK;
      close(fd);
      return NOK;
    }
    status |= TIOCM_DTR;
    if (ioctl(fd, TIOCMSET, &_status) < 0) {
      fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
      status = NOK;
      close(fd);
      return NOK;
    }
  }

  if (tcsetattr(fd, TCSANOW, &serie) < 0) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    close(fd);
    return NOK;
  }

  idport = fd;
  status = OPEN;
  last_call_time = std::chrono::steady_clock::now();
  return 0;
}

int serial::configport() {
  /* configuration du port */

  if (ioctl(fd, TCGETA, &serie) == NOK) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    return NOK;
  }

  serie.c_iflag = 0;

  if (ioctl(fd, TCSETA, &serie) == NOK) {
    fprintf(stderr, "Erreur lors de la configuration du port serie.\n");
    status = NOK;
    return (NOK);
  }
  idport = fd;
  return OK;
}

/*==========================================================================
    fonction: 	int fermerport(int fd)

    description:	Ferme le port serie.
            Retourne OK ou ERREUR.

    revisions: 	Martin Riopel 22/08/2002
============================================================================*/
int serial::fermerport() {
  if (close(fd) == NOK) {
    fprintf(stderr, "Erreur lors de la fermeture du port serie.\n");
    return NOK;
  }
  status = CLOSE;
  return OK;
}

/*==========================================================================
    fonction: 	int ecrireport(int fd, char *str)

    description:	Envoie une chaine sur le port serie.
            Retourne OK ou ERREUR.

    revisions: 	Martin Riopel 22/08/2002
============================================================================*/
int serial::ecrireport(std::string str) {
  int ncar;
  last_call_time = std::chrono::steady_clock::now();
  ncar = strlen(str.c_str());
  if (write(fd, str.c_str(), ncar) != ncar) {
    fprintf(stderr, "Erreur lors de l'ecriture sur le port serie.\n");
    return NOK;
  }
  return OK;
}

int serial::lireport(std::string *str, int ncar, int timeout_sec) {
  int i, n = 0;
  *str = "";
  char *tmpstr = new char[ncar];
  memset(tmpstr, 0, ncar);

  //:::::::::::::::::::::::::::::::::::
  //:::   Make sure N ms elapsed    :::
  //:::::::::::::::::::::::::::::::::::
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now - last_call_time)
                     .count();
  if (elapsed < call_delay) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(call_delay - elapsed));
  }

  for (i = 0; i < ncar; i++) {
    // Setup select for timeout
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    // Wait for data or timeout
    int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);

    if (select_result < 0) {
      // Error occurred
      std::cerr << "Select error in lireport" << std::endl;
      tmpstr[i] = '\0';
      *str = conversion(tmpstr);
      delete[] tmpstr;
      return -1; // Error
    } else if (select_result == 0) {
      // Timeout occurred
      tmpstr[i] = '\0';
      *str = conversion(tmpstr);
      delete[] tmpstr;
      return -2; // Timeout
    }

    // Data is available, read it
    n = read(fd, tmpstr + i, 1);

    if (n == 0) {
      tmpstr[i] = '\0';
      *str = conversion(tmpstr);
      delete[] tmpstr;
      return i;
    }
  }

  tmpstr[i] = '\0';
  *str = conversion(tmpstr);
  delete[] tmpstr;
  return i;
}

/*==========================================================================
    fonction: 	int lireport(fd, char *str, int ncar)

    description:	lit au plus ncar sur le port serie, copie les
            caracteres lus dans la chaine str, et ajoute '\0'.

            Retourne le nombre de caracteres lus.
            Retourne 0 si aucun caractere lu.
            Retourne ERREUR si une erreur a eu lieu.

    revisions: 	Martin Riopel 22/08/2002
============================================================================*/
int serial::lireport(std::string *str, int ncar)

{
  int i, n = 0;
  *str = "";
  char *tmpstr = new char[ncar];
  memset(tmpstr, 0, ncar);
  //:::::::::::::::::::::::::::::::::::
  //:::   Make sure N ms elapsed    :::
  //:::::::::::::::::::::::::::::::::::
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now - last_call_time)
                     .count();

  if (elapsed < call_delay) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(call_delay - elapsed));
  }

  for (i = 0; i < ncar; i++) {
    n = read(fd, tmpstr + i, 1);
    // printf("n: %d\n",n);
    if (n == 0) {
      tmpstr[i] = '\0';
      *str = conversion(tmpstr);
      delete[] tmpstr;
      return i;
    }
  }
  tmpstr[i] = '\0';
  *str = conversion(tmpstr);
  delete[] tmpstr;
  return i;
}

/*==========================================================================
    fonction: 	int lirec(int fd, char c)

    description:	Lit sur le port serie jusqu'au caractere c.
            Retourne OK ou ERREUR.

    revisions: 	Martin Riopel 22/08/2002
============================================================================*/
void serial::delay(int ms)
/*!
 *\brief Delay in ms
 * Create a delay. ms is delay time in millisecond
 */
{
  clock_t t0 = clock();
  while (clock() < t0 + ms * 1000)
    ;
}

int serial::lirec(std::string *str, char c)
/* Lit sur le port serie jusqu'au caractere c.
 * Retourne le nombre de charactère lu.
 */
{
  int i, n;
  *str = "";
  char *tmpstr = new char[MAX_CHAR];
  memset(tmpstr, 0, MAX_CHAR);
  //:::::::::::::::::::::::::::::::::::
  //:::   Make sure N ms elapsed    :::
  //:::::::::::::::::::::::::::::::::::
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now - last_call_time)
                     .count();

  if (elapsed < call_delay) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(call_delay - elapsed));
  }

  for (i = 0; i < MAX_CHAR; i++) {
    n = read(fd, tmpstr + i, 1);

    if (n == 0) {
      tmpstr[i] = '\0';
      *str = conversion(tmpstr);
      delete[] tmpstr;
      return i;
    } else if (tmpstr[i] == c) {
      tmpstr[i] = '\0';
      *str = conversion(tmpstr);
      delete[] tmpstr;
      return i;
    }
  }

  tmpstr[i] = '\0';
  *str = conversion(tmpstr);
  delete[] tmpstr;
  return i;
}

std::string serial::conversion(std::string ss) {
  std::string buff = "";
  for (auto &c : ss) {
    buff += c & 0x7F;
  }
  if (buff.length() > 0) {
    if (buff[buff.length() - 1] == '\n') {
      buff[buff.length() - 1] = '\0';
    }
  }
  return buff;
}
std::string serial::communicate(std::string cmd, int ncar, bool strip) {
  /*
   * description
   * -----------
   *  will perform a write + read. If the read is successfull, exchange_status
   *  will be set to VALID, otherwise to NOTVALID or EMPTY if no character
   *  was detected.
   *  ncar (default 4096) can be change and strip falg will strip ALL linefeed
   * \n and carriage return \r before return the string.
   *
   */
  exchange_status = NOTVALID;
  ncar_read = 0;
  std::string buffer = "", buffer2 = "";
  if (status != OPEN) {
    return "";
  }
  if (ecrireport(cmd) != OK) {
    return "";
  }
  ncar_read = lireport(&buffer, ncar);
  if (ncar_read == 0) {
    exchange_status = EMPTY;
    return "";
  }
  exchange_status = VALID;
  if (strip) {
    buffer2 = "";
    for (auto &c : buffer) {
      if (c != '\n' && c != '\r') {
        buffer2 += c;
      }
    }
    return buffer2;
  }
  return buffer;
}
int serial::waitfor(const char a, int timeout) {
  /*
   * timeout in ms
   */
  char dummy;
  int n = 0;
  std::chrono::steady_clock::time_point last_call_time(
      std::chrono::steady_clock::now());

  while (1) {
    n = read(fd, &dummy, 1);
    if (n == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } else if (n == -1) {
      fprintf(stderr, "Erreur lors de la lecture du port serie.\n");
      return -1;
    }
    if (n == a) {
      return 0;
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - last_call_time)
                       .count();
    if (elapsed > timeout) {
      break;
    }
  }

  return -1;
}

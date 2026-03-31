#ifndef ETHERNET_H
#define ETHERNET_H

#include "states_io.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class ethernet {
public:
  ethernet();
  int open_socket(std::string ip, uint16_t port);
  int open_socket(std::string ip, uint16_t port, int timeout); // use a timeout
  void close_socket(void);
  void error(const char *msg);
  int write_E(std::string commande);
  int read_E(std::string *response);
  int status;
  //  void set_connection_timeout(int sTime);
  // void set_recv_timeout(int sTime);

private:
  std::string ip;
  uint16_t p;
  int sockfd;
  // int connection_timeout;
  // int receive_timeout;
  char response[256];
};
#endif

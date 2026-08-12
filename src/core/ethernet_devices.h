#ifndef ETHERNET_DEVICES_H
#define ETHERNET_DEVICES_H

#include "ethernet.h"
#include "labdevices.h"
#include <prometheus/registry.h>
#include <string>
#define MAXBUFFER 4096
class ethernet_device : public labdevices, public ethernet {

public:
  ethernet_device(std::string devicename,
                  prometheus::Registry *registry = nullptr);
  ~ethernet_device();
  bool connect(std::string IP, int port);
  void set_recv_timeout(int sTime);
  void set_connection_timeout(int sTime);
  bool reconnect_automatically;
  int reconnect_timeout;
  int w_socket(const std::string &data);
  int r_socket(std::string *buff);
  int wr_socket(std::string _write, std::string *buffer);
  void close_connection();

private:
  bool _connect();
  bool _connect_w_timeout();
  int connection_timeout;
  int receive_timeout;
  bool checkConnection();
  bool reconnect();
  int port;
  int sockfd;
  std::string IP;

  //:::   reconnect() is reached from r_socket() / w_socket(), so it     :::
  //:::   runs at the poll rate while the instrument is unreachable.     :::
  //:::   The latch keeps that to one entry per outage.                  :::
  labdev::log_latch reconnect_fault;
};

#endif

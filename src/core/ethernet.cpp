
#include "ethernet.h"
#include <cstring>
#include <errno.h>
#include <fcntl.h>
ethernet::ethernet() {
  sockfd = 0;
  status = OK;
  //  receive_timeout = -1;
  //  connection_timeout = -1;
  memset(response, 0, 256);
}

int ethernet::open_socket(std::string ip, uint16_t port) {
  ethernet::ip = ip;
  ethernet::p = port;

  // Endpoint string reused by the log entries below.
  const std::string target = ip + ":" + std::to_string(port);

  struct sockaddr_in serv_addr;
  struct hostent *server;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    status = NOK;
    fprintf(stderr, "ERROR opening socket\n");
    labdev::log_error("ethernet", "socket() failed for " + target + ": " +
                                      std::string(strerror(errno)));
    return NOK;
  }
  server = gethostbyname(ip.c_str());
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    labdev::log_error("ethernet", "unknown host '" + ip + "'");
    status = NOK;
    return NOK;
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port =
      htons(p); /* converting port number using function htons() */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting\n");
    labdev::log_error("ethernet", "connect to " + target + " failed: " +
                                      std::string(strerror(errno)));
    status = NOK;
    return NOK;
  }
  status = OPEN;
  labdev::log_info("ethernet", "connected to " + target);
  return OK;
}
int ethernet::open_socket(std::string ip, uint16_t port, int timeout)
// Same as open_socket but we allow a timeout (in seconds)
{
  ethernet::ip = ip;
  ethernet::p = port;

  // Endpoint string reused by the log entries below.
  const std::string target = ip + ":" + std::to_string(port);

  struct sockaddr_in serv_addr;
  struct hostent *server;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    status = NOK;
    fprintf(stderr, "ERROR opening socket\n");
    labdev::log_error("ethernet", "socket() failed for " + target + ": " +
                                      std::string(strerror(errno)));
    return NOK;
  }

  struct timeval tout;
  tout.tv_sec = timeout;
  tout.tv_usec = 0;

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof tout) < 0) {
    perror("setsockopt failed\n");
    labdev::log_error("ethernet", "cannot set a " + std::to_string(timeout) +
                                      " s receive timeout on " + target + ": " +
                                      std::string(strerror(errno)));
    status = NOK;
    return NOK;
  }

  server = gethostbyname(ip.c_str());
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    labdev::log_error("ethernet", "unknown host '" + ip + "'");
    status = NOK;
    return NOK;
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port =
      htons(p); /* converting port number using function htons() */

  printf("ok-2\n");
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting\n");
    labdev::log_error("ethernet", "connect to " + target + " failed: " +
                                      std::string(strerror(errno)));
    status = NOK;
    return NOK;
  }

  printf("ok-3\n");
  status = OPEN;
  labdev::log_info("ethernet", "connected to " + target + ", receive timeout " +
                                   std::to_string(timeout) + " s");
  return OK;
}
/*
int ethernet::open_socket2(std::string ip, uint16_t port, int timeout)
// Same as open_socket but we allow a timeout (in seconds)
{
  ethernet::ip = ip;
  ethernet::p = port;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    status = NOK;
    fprintf(stderr, "ERROR opening socket\n");
    return NOK;
  }

  if (connection_timeout > 0) {
    // Set socket to non-blocking mode for connect timeout
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
      fprintf(stderr, "ERROR getting socket flags\n");
      close(sockfd);
      status = NOK;
      return NOK;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
      fprintf(stderr, "ERROR setting non-blocking mode\n");
      close(sockfd);
      status = NOK;
      return NOK;
    }
  }
  server = gethostbyname(ip.c_str());
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    close(sockfd);
    status = NOK;
    return NOK;
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(p);

  printf("ok-2\n");

  // Attempt connection (will return immediately with EINPROGRESS)
  int result =
      connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection_timeout > 0) {
    if (result < 0) {
      if (errno != EINPROGRESS) {
        fprintf(stderr, "ERROR connecting\n");
        close(sockfd);
        status = NOK;
        return NOK;
      }
    }

    else {
      if (result < 0) {
        fprintf(stderr, "ERROR connecting\n");
        status = NOK;
        return NOK;
      }
    }
    !!!here

          // Connection in progress - wait with timeout
          fd_set wfds;
    struct timeval tv;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    result = select(sockfd + 1, NULL, &wfds, NULL, &tv);

    if (result < 0) {
      fprintf(stderr, "ERROR in select\n");
      close(sockfd);
      status = NOK;
      return NOK;
    } else if (result == 0) {
      fprintf(stderr, "ERROR connection timeout\n");
      close(sockfd);
      status = NOK;
      return NOK;
    }

    // Check if connection actually succeeded
    int error;
    socklen_t len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
      fprintf(stderr, "ERROR getting socket error status\n");
      close(sockfd);
      status = NOK;
      return NOK;
    }

    if (error != 0) {
      fprintf(stderr, "ERROR connecting: %s\n", strerror(error));
      close(sockfd);
      status = NOK;
      return NOK;
    }
  }

  // Restore blocking mode
  if (fcntl(sockfd, F_SETFL, flags) < 0) {
    fprintf(stderr, "ERROR restoring blocking mode\n");
    close(sockfd);
    status = NOK;
    return NOK;
  }

  // Set receive timeout (as in original code)
  struct timeval tout;
  tout.tv_sec = timeout;
  tout.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof tout) < 0) {
    perror("setsockopt failed\n");
    close(sockfd);
    status = NOK;
    return NOK;
  }

  printf("ok-3\n");
  status = OPEN;
  return OK;
}

*/
void ethernet::close_socket(void) {
  close(sockfd);
  status = CLOSE;
  labdev::log_debug("ethernet",
                    "socket closed for " + ip + ":" + std::to_string(p));
}

int ethernet::read_E(std::string *response) {
  int n = 0;
  bzero(ethernet::response, strlen(ethernet::response));
  n = read(sockfd, ethernet::response, 255);
  if (n < 0) {
    fprintf(stderr, "ERROR reading from socket\n");
    return NOK;
  }
  if (n > 2)
    ethernet::response[n - 2] = '\0';
  *response = std::string(ethernet::response);
  return OK;
}
int ethernet::write_E(std::string commande) /* ecrit mais ne lit pas */
{
  int n;

  n = write(sockfd, commande.c_str(), strlen(commande.c_str()));

  if (n < 0) {
    fprintf(stderr, "Erreur a l'ecriture au socket\n");
    return NOK;
  }
  return OK;
}

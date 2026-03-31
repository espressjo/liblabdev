#include "ethernet_devices.h"
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
ethernet_device::ethernet_device(std::string device_name,
                                 prometheus::Registry *registry)
    : labdevices(device_name, registry) {

  port = 0;
  IP = "127.0.0.1";
}
void ethernet_device::set_recv_timeout(int sTime) { receive_timeout = sTime; }
void ethernet_device::set_connection_timeout(int sTime) {
  connection_timeout = sTime; // in seconds
  return;
}
bool ethernet_device::connect(std::string IP, int port) {
  ethernet_device::port = port;
  ethernet_device::IP = IP;

  if (connection_timeout > 0) {
    if (ethernet_device::_connect_w_timeout()) {

      online = true;
      return true;
    } else {
      online = false;
      return false;
    }
  } else {
    if (ethernet_device::_connect()) {
      online = true;
      return true;
    } else {
      online = false;
      return false;
    }
  }
}
bool ethernet_device::_connect_w_timeout()
// Same as open_socket but we allow a timeout (in seconds)
{
  struct sockaddr_in serv_addr;
  struct hostent *server;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    status = NOK;
    fprintf(stderr, "ERROR opening socket\n");
    return false;
  }
  int flags = 0;
  if (connection_timeout > 0) {
    // Set socket to non-blocking mode for connect timeout
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
      fprintf(stderr, "ERROR getting socket flags\n");
      close(sockfd);
      status = NOK;
      return false;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
      fprintf(stderr, "ERROR setting non-blocking mode\n");
      close(sockfd);
      status = NOK;
      return false;
    }
  }
  server = gethostbyname(IP.c_str());
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    close(sockfd);
    status = NOK;
    return false;
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(static_cast<uint16_t>(port));

  // Attempt connection (will return immediately with EINPROGRESS)
  int result =
      ::connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection_timeout > 0) {
    if (result < 0) {
      if (errno != EINPROGRESS) {
        fprintf(stderr, "ERROR connecting\n");
        close(sockfd);
        status = NOK;
        return false;
      }
    }

    else {
      if (result < 0) {
        fprintf(stderr, "ERROR connecting\n");
        status = NOK;
        return false;
      }
    }

    // Connection in progress - wait with timeout
    fd_set wfds;
    struct timeval tv;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    tv.tv_sec = connection_timeout;
    tv.tv_usec = 0;

    result = select(sockfd + 1, NULL, &wfds, NULL, &tv);

    if (result < 0) {
      fprintf(stderr, "ERROR in select\n");
      close(sockfd);
      status = NOK;
      return false;
    } else if (result == 0) {
      fprintf(stderr, "ERROR connection timeout\n");
      close(sockfd);
      status = NOK;
      return false;
    }

    // Check if connection actually succeeded
    int error;
    socklen_t len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
      fprintf(stderr, "ERROR getting socket error status\n");
      close(sockfd);
      status = NOK;
      return false;
    }

    if (error != 0) {
      fprintf(stderr, "ERROR connecting: %s\n", strerror(error));
      close(sockfd);
      status = NOK;
      return false;
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
  if (receive_timeout > 0) {
    struct timeval tout;
    tout.tv_sec = receive_timeout;
    tout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof tout) < 0) {
      perror("setsockopt failed\n");
      close(sockfd);
      status = NOK;
      return NOK;
    }
  }
  status = OPEN;
  return true;
}
bool ethernet_device::_connect() {

  struct sockaddr_in serv_addr;
  struct hostent *server;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    status = NOK;
    fprintf(stderr, "ERROR opening socket\n");
    return NOK;
  }
  if (receive_timeout > 0) {
    struct timeval tout;
    tout.tv_sec = receive_timeout;
    tout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof tout) < 0) {
      perror("setsockopt failed\n");
      status = NOK;
      return false;
    }
  }
  server = gethostbyname(IP.c_str());
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    status = NOK;
    return false;
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(static_cast<uint16_t>(
      port)); /* converting port number using function htons() */

  if (::connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting\n");
    status = NOK;
    return false;
  }

  status = OPEN;
  return true;
}

bool ethernet_device::reconnect() {
  // 1. Close the old socket first (very important!)
  if (sockfd >= 0) {
    close(sockfd);
    sockfd = -1; // Mark as invalid
  }

  // 2. Call your open_socket() function
  // if noo timeout put one.
  if (connection_timeout < 1) {
    set_connection_timeout(1);
  }
  if (!_connect_w_timeout()) {
    online = false;
    return false; // Reconnection failed
  }
  online = true;
  return true; // Successfully reconnected
}

bool ethernet_device::checkConnection() {
  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = POLLIN | POLLOUT;

  int result = poll(&pfd, 1, 0); // 0 timeout = non-blocking

  if (result < 0) {
    online = false;
    return false; // Error
  }

  // Check for hangup or error
  if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
    online = false;
    return false;
  }

  // If POLLIN is set, try to read to see if it's EOF
  if (pfd.revents & POLLIN) {
    char buffer;
    int n = recv(sockfd, &buffer, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) {
      online = false;
      return false; // EOF, connection closed
    }
  }
  online = true;
  return true;
}
void ethernet_device::close_connection() {
  if (sockfd >= 0) {
    close(sockfd);
    online = false;
    sockfd = -1; // Mark as invalid
  }
}
ethernet_device::~ethernet_device() {
  close_connection();
  return;
}

int ethernet_device::r_socket(std::string *buff) {
  // Check if connection is still active
  if (!checkConnection()) {
    // Connection lost, attempt to reconnect
    if (!reconnect()) {
      // Reconnection failed
      fprintf(stderr, "Error: Failed to reconnect to server\n");
      return -1;
    }

    // Successfully reconnected
    printf("Successfully reconnected to server\n");
  }

  // Create temporary buffer for recv
  char *tempBuffer = new char[MAXBUFFER];

  // Attempt to read from socket
  int bytesRead = recv(sockfd, tempBuffer, MAXBUFFER, 0);

  if (bytesRead < 0) {
    // Read error occurred
    fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));

    // Try to reconnect one more time
    if (reconnect()) {
      bytesRead = recv(sockfd, tempBuffer, MAXBUFFER, 0);

      if (bytesRead < 0) {
        fprintf(stderr, "Error: Read failed after reconnection: %s\n",
                strerror(errno));
        delete[] tempBuffer;
        return -1;
      }
    } else {
      fprintf(stderr, "Error: Reconnection failed after read error\n");
      delete[] tempBuffer;
      return 0;
    }
  } else if (bytesRead == 0) {
    // Connection closed by remote host
    fprintf(stderr, "Connection closed by remote host\n");

    // Attempt to reconnect
    if (reconnect()) {
      printf("Successfully reconnected after connection closure\n");
      delete[] tempBuffer;
      return 0;
    } else {
      fprintf(stderr, "Error: Failed to reconnect after connection closure\n");
      delete[] tempBuffer;
      return 0;
    }
  }

  // Copy data to string
  buff->assign(tempBuffer, bytesRead);
  delete[] tempBuffer;

  // Success - return number of bytes read
  return bytesRead;
}

int ethernet_device::w_socket(const std::string &data) {
  // Check if connection is still active
  if (!checkConnection()) {
    // Connection lost, attempt to reconnect
    if (!reconnect()) {
      // Reconnection failed
      fprintf(stderr, "Error: Failed to reconnect to server\n");
      return -1;
    }

    // Successfully reconnected
    printf("Successfully reconnected to server\n");
  }

  // Attempt to write to socket
  int bytesSent = send(sockfd, data.c_str(), data.size(), 0);

  if (bytesSent < 0) {
    // Write error occurred
    fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));

    // Try to reconnect one more time
    if (reconnect()) {
      printf("Reconnected after write error, retrying write...\n");
      bytesSent = send(sockfd, data.c_str(), data.size(), 0);

      if (bytesSent < 0) {
        fprintf(stderr, "Error: Write failed after reconnection: %s\n",
                strerror(errno));
        return -1;
      }
    } else {
      fprintf(stderr, "Error: Reconnection failed after write error\n");
      return -1;
    }
  } else if (bytesSent == 0) {
    // This shouldn't normally happen with send, but handle it
    fprintf(stderr, "Warning: Zero bytes sent\n");
    return 0;
  }

  // Check if all data was sent
  if (bytesSent < static_cast<int>(data.size())) {
    fprintf(stderr, "Warning: Partial write - sent %d of %zu bytes\n",
            bytesSent, data.size());
  }

  // Success - return number of bytes sent
  return bytesSent;
}
int ethernet_device::wr_socket(std::string _write, std::string *buffer) {

  if (w_socket(_write) <= 0) {
    return 0;
  }
  int ret = r_socket(buffer);
  return ret;
}

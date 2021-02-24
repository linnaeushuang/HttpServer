#include "SocketUtils.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>



const int MAX_BUFF=4096;

ssize_t readn(int fd, void *buff, size_t n) {
	size_t nleft = n;
	ssize_t nread = 0;
	ssize_t readSum = 0;
	char *ptr = (char *)buff;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) nread = 0;
			else if (errno == EAGAIN) return readSum;
			else return -1;
		} else if (nread == 0) break;
		readSum += nread;
		nleft -= nread;
		ptr += nread;
	}
	return readSum;
}


ssize_t readn(int fd, std::string &inBuffer, bool &zero) {
	ssize_t nread = 0;
	ssize_t readSum = 0;
	while (true) {
		char buff[MAX_BUFF];
		if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
			if (errno == EINTR) continue;
			else if (errno == EAGAIN) return readSum;
			else {
				perror("read error");
				return -1;
			}
		} else if (nread == 0){
			zero=true;
			break;
		}
    	readSum += nread;
		inBuffer += std::string(buff, buff + nread);
	}
	return readSum;
}


ssize_t readn(int fd, std::string &inBuffer) {
	ssize_t nread = 0;
	ssize_t readSum = 0;
	while (true) {
		char buff[MAX_BUFF];
		if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
			if (errno == EINTR) continue;
			else if (errno == EAGAIN) return readSum;
			else {
				perror("read error");
				return -1;
			}
		} else if (nread == 0){
			break;
		}
    	readSum += nread;
		inBuffer += std::string(buff, buff + nread);
	}
	return readSum;
}

ssize_t writen(int fd, void *buff, size_t n) {
	size_t nleft = n;
	ssize_t nwritten = 0;
	ssize_t writeSum = 0;
	char *ptr = (char *)buff;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0) {
				if (errno == EINTR) {
					nwritten = 0;
					continue;
				} else if (errno == EAGAIN) {
					return writeSum;
				} else
					return -1;
			}
		}
		writeSum += nwritten;
		nleft -= nwritten;
		ptr += nwritten;
	}
	return writeSum;
}


ssize_t writen(int fd, std::string &sbuff) {
	size_t nleft = sbuff.size();
	ssize_t nwritten = 0;
	ssize_t writeSum = 0;
	const char *ptr=sbuff.c_str();
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0) {
				if (errno == EINTR) {
					nwritten = 0;
					continue;
				} else if (errno == EAGAIN) {
					return writeSum;
				} else
					return -1;
			}
		}
		writeSum += nwritten;
		nleft -= nwritten;
		ptr += nwritten;
	}
	if(writeSum==static_cast<int>(sbuff.size()))
		sbuff.clear();
	else
		sbuff = sbuff.substr(writeSum);
	return writeSum;
}


void handle_for_sigpipe() {
	/*
	 * struct sigaction{
	 * void (*sa_handler)(int);
	 * void (*sa_sigaction)(int,siginfo_t *,void *);
	 * sigset_t sa_mask;
	 * int sa_flags;
	 * void (*sa_restorer)(void);
	 * }
	 */
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	// set handle action is ignore the signal
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

	// when opposite end close the socket(half-close of TCP),
	// and local end call write() 2 times.
	// the 2st time will generate SIGPIPE signal.
	// 
	// if we see SIGPIPE,
	// the way of handle this signal is set to sa(SIG_IGN)
	//
	// default action of get SIGPIPE is close process.
	if (sigaction(SIGPIPE, &sa, NULL)) return;
}

int setSocketNonBlocking(int fd) {
	// F_GETFL: get the state identifier of the fd,
	//          O_RDONLY (cannot change)
	//          O_WRONLY (cannot change)
	//          O_RDWR   (cnannot change)
	//          O_APPEND
	//          O_NONBLOCK
	//          O_SYNC
	//          O_ASYNC
	int flag = fcntl(fd, F_GETFL, 0);

	// if fcntl return error
	if (flag == -1) return -1;
	// set O_NONBLOCK
	flag |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flag) == -1) return -1;
	return 0;
}

void setSocketNodelay(int fd) {
	// set socket option
	int enable = 1;
	// Nagle algorithm is ban.
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}

void setSocketNoLinger(int fd) {
	// wait 30 until
	// send the data remained in TCP buffer,and get ACK.
	struct linger linger_;
	linger_.l_onoff = 1;
	linger_.l_linger = 30;
	setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_,
			sizeof(linger_));
}


int socket_bind_listen(int port) {
  if (port < 0 || port > 65535) return -1;
  // create socket (ipv4 + TCP)
  //  AF_INET == PF_INET, both is ipv4
  //  SOCK_STREAM is tcp
  //  SOCK_DGRAM is udp
  int listen_fd = 0;
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	  close(listen_fd);
	  return -1;
  }

  // eliminate bind "Address already in use" error
  // address can reuse.
  int optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    close(listen_fd);
    return -1;
  }

  // set IP and port. bind address and socket
  struct sockaddr_in server_addr;
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  // INADDR_ANY is 0.0.0.0
  // all,any ip addres
  //
  // host(little endian) to network(big endian) long
  // other 3 func: htons(unsigned short)
  // 			   ntohl
  // 			   ntohs
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    close(listen_fd);
    return -1;
  }

  // listen
  // max length of listen queue is 2048
  if (listen(listen_fd, 2048) == -1) {
    close(listen_fd);
    return -1;
  }

  return listen_fd;
}

void shutDownWR(int fd){
	// close this socketfd write
	// trans to FIN_WAIT_1(2)
	//
	// if parent thread and child pthread share this socketfd
	// all wolud not write.
	shutdown(fd, SHUT_WR);
}

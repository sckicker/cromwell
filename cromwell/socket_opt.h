#ifndef __SOCKET_OPT_H
#define __SOCKET_OPT_H

namespace cromwell {

int create_socket(char *err, int domain);

int tcp_connect(char *err, char *addr, int port);
int tcp_nonblock_connect(char *err, char *addr, int port);
int tcp_nonblock_bind_connect(char *err, char *addr, int port, char *source_addr);

int nonblock(char *err, int fd);
int block(char *err, int fd);

int keep_alive(char *err, int fd, int interval);

int enable_tcp_nodelay(char *err, int fd);
int disable_tcp_nodelay(char *err, int fd);

int set_send_buffer(char *err, int fd, int buffsize);
int tcp_keep_alive(char *err, int fd);
int send_timeout(char *err, int fd, long long ms);

int resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len);
int resolve_ip(char *err, char *host, char *ipbuf, size_t ipbuf_len);

int tcp_server(char *err, int port, char *bindaddr, int backlog);
int listen(char *err, int s, struct sockaddr *sa, socklen_t len, int backlog);
int accept(char* err, int serversock, char* ip, size_t ip_len, int* port);

int s_read(int fd, char *buf, int count);
int s_write(int fd, char *buf, int count);

int get_peer_string(int fd, char *ip, size_t ip_len, int *port);

int socket_create_pair(char* err, int fd[2]);

int socket_close(int fd);

}//end-cromwell.

#endif

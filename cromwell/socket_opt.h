#pragma once

namespace cromwell {

int tcp_connect(char *err, char *addr, int port);
int tcp_nonblock_connect(char *err, char *addr, int port);
int tcp_nonblock_bind_connect(char *err, char *addr, int port, char *source_addr);

void close(int sock);
void shutdown_write(int sock);
int read(int sock, void* buf, int count);
int write(int sock, const void* buf, int count);

int accept(char* err, int serversock, char* ip, size_t ip_len, int* port);

}//

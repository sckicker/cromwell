#include "socket_opt.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace cromwell {

static int kErrorMsgLen = 256;

static void set_error(char* err, const char* fmt, ...) {
	if (!err) return;

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(err, kErrorMsgLen, fmt, ap);
	va_end(ap);
}

static int set_block(char *err, int fd, int non_block) {
    int flags;

    /* Set the socket blocking (if non_block is zero) or non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        set_error(err, "fcntl(F_GETFL): %s", strerror(errno));
        return -1;
    }

    if (non_block)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        set_error(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return -1;
    }
    return 0;
}

int nonblock(char *err, int fd) {
    return set_block(err, fd, 1);
}

int block(char *err, int fd) {
    return set_block(err, fd, 0);
}

/* Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
int keep_alive(char *err, int fd, int interval) {
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
        set_error(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return -1;
    }

#ifdef __linux__
    /* Default settings are more or less garbage, with the keepalive time
     * set to 7200 by default on Linux. Modify settings to make the feature
     * actually useful. */

    /* Send first probe after interval. */
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        set_error(err, "setsockopt TCP_KEEPIDLE: %s\n", strerror(errno));
        return -1;
    }

    /* Send next probes after the specified interval. Note that we set the
     * delay as interval / 3, as we send three probes before detecting
     * an error (see the next setsockopt call). */
    val = interval/3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        set_error(err, "setsockopt TCP_KEEPINTVL: %s\n", strerror(errno));
        return -1;
    }

    /* Consider the socket in error state after three we send three ACK
     * probes without getting a reply. */
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        anetSetError(err, "setsockopt TCP_KEEPCNT: %s\n", strerror(errno));
        return -1;
    }
#else
    ((void) interval); /* Avoid unused var warning for non Linux systems. */
#endif

    return 0;
}

static int set_tcp_nodelay(char *err, int fd, int val) {
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
        set_error(err, "setsockopt TCP_NODELAY: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int enable_tcp_nodelay(char *err, int fd) {
    return set_tcp_nodelay(err, fd, 1);
}

int disable_tcp_nodelay(char *err, int fd) {
    return set_tcp_nodelay(err, fd, 0);
}


int set_send_buffer(char *err, int fd, int buffsize) {
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1) {
        set_error(err, "setsockopt SO_SNDBUF: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int tcp_keep_alive(char *err, int fd) {
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        set_error(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/* Set the socket send timeout (SO_SNDTIMEO socket option) to the specified
 * number of milliseconds, or disable it if the 'ms' argument is zero. */
int send_timeout(char *err, int fd, long long ms) {
    struct timeval tv;

    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        set_error(err, "setsockopt SO_SNDTIMEO: %s", strerror(errno));
        return -1;
    }
    return 0;
}

static int set_reuse_addr(char *err, int fd) {
    int yes = 1;
    /* Make sure connection-intensive things like the redis benckmark
     * will be able to close/open sockets a zillion of times */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        set_error(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/* anetGenericResolve() is called by anetResolve() and anetResolveIP() to
 * do the actual work. It resolves the hostname "host" and set the string
 * representation of the IP address into the buffer pointed by "ipbuf".
 *
 * If flags is set to ANET_IP_ONLY the function only resolves hostnames
 * that are actually already IPv4 or IPv6 addresses. This turns the function
 * into a validating / normalizing function. */
static int gene_resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len, int flags) {
    struct addrinfo hints, *info;
    int rv;

    memset(&hints, 0, sizeof(hints));
    if (flags & ANET_IP_ONLY) hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  /* specify socktype to avoid dups */

    if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
        set_error(err, "%s", gai_strerror(rv));
        return -1;
    }
    if (info->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
    } else {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)info->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
    }

    freeaddrinfo(info);
    return 0;
}

int resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return gene_resolve(err, host, ipbuf, ipbuf_len, ANET_NONE);
}

int resolve_ip(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return gene_resolve(err, host, ipbuf, ipbuf_len, ANET_IP_ONLY);
}

static int v6_only(char *err, int s) {
    int yes = 1;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) == -1) {
        set_error(err, "setsockopt: %s", strerror(errno));
        close(s);
        return -1;
    }
    return 0;
}

int create_socket(char *err, int domain) {
    int s;
    if ((s = socket(domain, SOCK_STREAM, 0)) == -1) {
        set_error(err, "creating socket: %s", strerror(errno));
        return -1;
    }

    /* Make sure connection-intensive things like the redis benchmark
     * will be able to close/open sockets a zillion of times */
    if (set_reuse_addr(err, s) == -1) {
        close(s);
        return -1;
    }
    return s;
}

#define CONNECT_NONE 0
#define CONNECT_NONBLOCK 1
#define CONNECT_BE_BINDING 2 /* Best effort binding. */
static int tcp_gene_connect(char *err, char *addr, int port, char *source_addr, int flags) {
    int s = ANET_ERR, rv;
    char portstr[6];  /* strlen("65535") + 1; */
    struct addrinfo hints, *servinfo, *bservinfo, *p, *b;

    snprintf(portstr, sizeof(portstr), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr, portstr, &hints, &servinfo)) != 0) {
        set_error(err, "%s", gai_strerror(rv));
        return -1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* Try to create the socket and to connect it.
         * If we fail in the socket() call, or on connect(), we retry with
         * the next entry in servinfo. */
        if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        if (set_reuse_addr(err, s) == -1) goto error;
        if (flags & CONNECT_NONBLOCK && nonblock(err,s) != 0)
            goto error;
        if (source_addr) {
            int bound = 0;
            /* Using getaddrinfo saves us from self-determining IPv4 vs IPv6 */
            if ((rv = getaddrinfo(source_addr, NULL, &hints, &bservinfo)) != 0) {
                set_error(err, "%s", gai_strerror(rv));
                goto error;
            }
            for (b = bservinfo; b != NULL; b = b->ai_next) {
                if (bind(s,b->ai_addr,b->ai_addrlen) != -1) {
                    bound = 1;
                    break;
                }
            }
            freeaddrinfo(bservinfo);
            if (!bound) {
                set_error(err, "bind: %s", strerror(errno));
                goto error;
            }
        }
        if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
            /* If the socket is non-blocking, it is ok for connect() to
             * return an EINPROGRESS error here. */
            if (errno == EINPROGRESS && flags & CONNECT_NONBLOCK)
                goto end;
            close(s);
            s = -1;
            continue;
        }

        /* If we ended an iteration of the for loop without errors, we
         * have a connected socket. Let's return to the caller. */
        goto end;
    }
    if (p == NULL)
        anetSetError(err, "creating socket: %s", strerror(errno));

error:
    if (s != -1) {
        close(s);
        s = -1;
    }

end:
    freeaddrinfo(servinfo);

    /* Handle best effort binding: if a binding address was used, but it is
     * not possible to create a socket, try again without a binding address. */
    if (s == -1 && source_addr && (flags & CONNECT_BE_BINDING)) {
        return tcp_gene_connect(err, addr, port, NULL, flags);
    } else {
        return s;
    }
}

int tcp_connect(char *err, char *addr, int port) {
    return tcp_gene_connect(err, addr, port, NULL, ANET_CONNECT_NONE);
}

int tcp_nonblock_connect(char *err, char *addr, int port) {
    return tcp_gene_connect(err, addr, port, NULL, ANET_CONNECT_NONBLOCK);
}

int tcp_nonblock_bind_connect(char *err, char *addr, int port, char *source_addr) {
    return tcp_gene_connect(err, addr, port, source_addr, ANET_CONNECT_NONBLOCK);
}

int listen(char* err, int s, struct sockaddr* sa, socklen_t len, int backlog) {
    if (bind(s, sa, len) == -1) {
        set_error(err, "bind: %s", strerror(errno));
        close(s);
        return -1;
    }

    if (listen(s, backlog) == -1) {
        set_error(err, "listen: %s", strerror(errno));
        close(s);
        return -1;
    }
    return 0;
}

static int _tcp_server(char *err, int port, char *bindaddr, int af, int backlog) {
    int s, rv;
    char _port[6];  /* strlen("65535") */
    struct addrinfo hints, *servinfo, *p;

    snprintf(_port, 6, "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    /* No effect if bindaddr != NULL */

    if ((rv = getaddrinfo(bindaddr, _port, &hints, &servinfo)) != 0) {
        set_error(err, "%s", gai_strerror(rv));
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (af == AF_INET6 && v6_only(err, s) == -1) goto error;
        if (set_reuse_addr(err, s) == -1) goto error;
        if (listen(err, s, p->ai_addr, p->ai_addrlen, backlog) == -1) goto error;
        goto end;
    }
    if (p == NULL) {
        set_error(err, "unable to bind socket");
        goto error;
    }

error:
    s = -1;
end:
    freeaddrinfo(servinfo);
    return s;
}

int tcp_server(char *err, int port, char *bindaddr, int backlog) {
    return _tcp_server(err, port, bindaddr, AF_INET, backlog);
}

static int gene_accept(char* err, int sock, struct sockaddr* sa, socklen_t* len) {
	int fd;
	while(true) {
		fd = accept(sock, sa, len);
		if (fd == -1) {
			if (errno == EINTR) continue;
			else {
				set_error(err, "accept [%s]", strerror(errno));
				return -1;
			}
		}
		break;
	}
	return fd;
}

int accept(char* err, int sock, char* ip, size_t ip_len, int* port) {
    int fd;
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if ((fd = gene_accept(err, sock, (struct sockaddr*)&sa, &salen)) == -1)
    	return -1;
     if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin6_port);
    }
    return fd;
}

/* Like read(2) but make sure 'count' is read before to return
 * (unless error or EOF condition is encountered) */
int s_read(int fd, char *buf, int count) {
    ssize_t nread, totlen = 0;
    while(totlen != count) {
        nread = read(fd, buf, count-totlen);
        if (nread == 0) return totlen;
        if (nread == -1) return -1;
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

/* Like write(2) but make sure 'count' is written before to return
 * (unless error is encountered) */
int s_write(int fd, char *buf, int count) {
    ssize_t nwritten, totlen = 0;
    while(totlen != count) {
        nwritten = write(fd, buf, count-totlen);
        if (nwritten == 0) return totlen;
        if (nwritten == -1) return -1;
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

int get_peer_string(int fd, char *ip, size_t ip_len, int *port) {
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if (getpeername(fd, (struct sockaddr*)&sa, &salen) == -1) goto error;
    if (ip_len == 0) goto error;

    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) inet_ntop(AF_INET, (void*)&(s->sin_addr), ip, ip_len);
        if (port) *port = ntohs(s->sin_port);
    } else if (sa.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6, (void*)&(s->sin6_addr), ip, ip_len);
        if (port) *port = ntohs(s->sin6_port);
    } else if (sa.ss_family == AF_UNIX) {
        if (ip) strncpy(ip, "/unixsocket", ip_len);
        if (port) *port = 0;
    } else {
        goto error;
    }
    return 0;

error:
    if (ip) {
    	if (ip_len >= 2) {
            ip[0] = '?';
            ip[1] = '\0';
        } else if (ip_len == 1) {
            ip[0] = '\0';
        }
    }
    if (port) *port = 0;
    return -1;
}

int socket_create_pair(char* err, int fd[2]) {
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
  	set_error(err, "socketpair: %s", strerror(errno));
  }
  return 0;
}

int socket_close(int fd) {
  return (fd < 0 ? -1 : close(fd));
}

}//end-cromwell

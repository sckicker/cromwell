#include "socket_opt.h"

namespace cromwell {

static void set_error(char* err, const char* fmt, ...) {
	va_list ap;

	if (!err) return;
	va_start(ap, fmt);
	vsnprintf(err, 256, fmt, ap);
	va_end(ap);
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

static int create_socket(char *err, int domain) {
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

static int listen(char *err, int s, struct sockaddr *sa, socklen_t len, int backlog) {
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
        if ((s = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
            continue;

        if (af == AF_INET6 && anetV6Only(err, s) == -1) goto error;
        if (anetSetReuseAddr(err, s) == ANET_ERR) goto error;
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

}//end-cromwell
#include <cfrds.h>
#include <internal/explicit_bzero.h>
#include <internal/cfrds_int.h>
#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <../tracing/tracing.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>


#include <time.h>

#define CFRDS_MAX_RESPONSE_SIZE (100 * 1024 * 1024)
#define CFRDS_MAX_RESPONSE_TIMEOUT_SEC 60

#ifdef _WIN32
typedef SOCKET cfrds_socket;
#define CFRDS_INVALID_SOCKET INVALID_SOCKET
#define CFRDS_SEND_FLAGS 0
#define GET_SOCKET_ERRNO() WSAGetLastError()
#define IS_SOCKET_EINTR(err) ((err) == WSAEINTR)
#else
typedef int cfrds_socket;
#define CFRDS_INVALID_SOCKET (-1)
#ifdef MSG_NOSIGNAL
#define CFRDS_SEND_FLAGS MSG_NOSIGNAL
#else
#define CFRDS_SEND_FLAGS 0
#endif
#define GET_SOCKET_ERRNO() errno
#ifdef EINTR
#define IS_SOCKET_EINTR(err) ((err) == EINTR)
#else
#define IS_SOCKET_EINTR(err) false
#endif
#endif

static void cfrds_sock_cleanup(cfrds_socket* sock);
#define cfrds_sock_defer(var) cfrds_socket var __attribute__((cleanup(cfrds_sock_cleanup))) = CFRDS_INVALID_SOCKET

/*
 * Linux suppresses SIGPIPE per send() call with MSG_NOSIGNAL. macOS and the
 * BSDs instead provide the per-socket SO_NOSIGPIPE option. Winsock never
 * raises the POSIX SIGPIPE signal, so no configuration is needed there.
 */
static bool cfrds_sock_configure_no_sigpipe(cfrds_socket sockfd)
{
#if !defined(_WIN32) && defined(SO_NOSIGPIPE)
    int enabled = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) == 0;
#else
    (void)sockfd;
    return true;
#endif
}

static bool cfrds_buffer_skip_httpheader(const char **data, size_t *remaining)
{
    if ((data == NULL) || (*data == NULL) || (remaining == NULL))
        return false;

    if (*remaining < 4)
        return false;

    const char *p = *data;
    size_t limit = *remaining - 3;
    const char *body = NULL;

    for (size_t i = 0; i < limit; i++) {
        if (p[i] == '\r' && p[i+1] == '\n' && p[i+2] == '\r' && p[i+3] == '\n') {
            body = p + i;
            break;
        }
    }

    if (body == NULL)
        return false;

    size_t header_len = (size_t)(body - *data);
    *data = body + 4;
    *remaining -= (header_len + 4);

    return true;
}

static cfrds_status http_build_request(cfrds_server *server, const char *command, cfrds_buffer *payload, cfrds_buffer *send_buf)
{
    char datasize_str[16] = {0, };
    uint16_t port = cfrds_server_get_port(server);

    int n = snprintf(datasize_str, sizeof(datasize_str), "%zu", cfrds_buffer_data_size(payload));
    if (n < 0 || (size_t)n >= sizeof(datasize_str))
    {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "snprintf() returned < 0 or truncated...");
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    bool ok = true;
    ok = ok && cfrds_buffer_append(send_buf, "POST /CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=");
    ok = ok && cfrds_buffer_append(send_buf, command);
    ok = ok && cfrds_buffer_append(send_buf, " HTTP/1.0\r\nHost: ");
    ok = ok && cfrds_buffer_append(send_buf, cfrds_server_get_host(server));
    if (port != 80)
    {
        char port_str[16] = {0, };
        n = snprintf(port_str, sizeof(port_str), "%d", port);
        if (n < 0 || (size_t)n >= sizeof(port_str))
        {
            cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "snprintf() returned < 0 or truncated...");
            return CFRDS_STATUS_MEMORY_ERROR;
        }
        ok = ok && cfrds_buffer_append(send_buf, ":");
        ok = ok && cfrds_buffer_append(send_buf, port_str);
    }
    ok = ok && cfrds_buffer_append(send_buf, "\r\nConnection: close\r\nUser-Agent: Mozilla/3.0 (compatible; Macromedia RDS Client)\r\nAccept: text/html, */*\r\nAccept-Encoding: deflate\r\nContent-type: text/html\r\nContent-length: ");
    ok = ok && cfrds_buffer_append(send_buf, datasize_str);
    ok = ok && cfrds_buffer_append(send_buf, "\r\n\r\n");
    ok = ok && cfrds_buffer_append_buffer(send_buf, payload);

    if (!ok)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "buffer append failed building request");
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    return CFRDS_STATUS_OK;
}

static cfrds_status http_connect(cfrds_server *server, cfrds_socket *out_sockfd)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    char port_str[16] = {0, };
    uint16_t port = cfrds_server_get_port(server);

    explicit_bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int n = snprintf(port_str, sizeof(port_str), "%u", port);
    if (n < 0 || (size_t)n >= sizeof(port_str)) {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "failed to format port string");
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    trace_net_start("getaddrinfo");
    int gai_err = getaddrinfo(cfrds_server_get_host(server), port_str, &hints, &result);
    trace_net_end();
    if (gai_err != 0) {
        cfrds_server_set_error(server, CFRDS_STATUS_SOCKET_HOST_NOT_FOUND, "failed to resolve hostname...");
        return CFRDS_STATUS_SOCKET_HOST_NOT_FOUND;
    }

    int saved_errno = 0;
    cfrds_socket sockfd = CFRDS_INVALID_SOCKET;

    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
    {
        trace_net_start("socket");
        cfrds_socket fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        trace_net_end();
        if (fd == CFRDS_INVALID_SOCKET)
            continue;

        if (!cfrds_sock_configure_no_sigpipe(fd)) {
            saved_errno = GET_SOCKET_ERRNO();
            cfrds_sock_cleanup(&fd);
            continue;
        }

        trace_net_start("connect");
        int res = connect(fd, rp->ai_addr, rp->ai_addrlen);
        trace_net_end();
        if (res == 0) {
            sockfd = fd;
            break;
        }

        saved_errno = GET_SOCKET_ERRNO();
        cfrds_sock_cleanup(&fd);
    }

    freeaddrinfo(result);

    if (sockfd == CFRDS_INVALID_SOCKET) {
        server->_errno = saved_errno;
        cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to establish connection to the server...");
        return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
    }

#ifdef _WIN32
    DWORD tv = 30000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0 ||
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
        server->_errno = GET_SOCKET_ERRNO();
        cfrds_sock_cleanup(&sockfd);
        cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to set socket timeout");
        return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
    }
#else
    struct timeval tv = { .tv_sec = 30, .tv_usec = 0 };
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        server->_errno = GET_SOCKET_ERRNO();
        cfrds_sock_cleanup(&sockfd);
        cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to set socket timeout");
        return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
    }
#endif

    *out_sockfd = sockfd;
    return CFRDS_STATUS_OK;
}

static cfrds_status http_send_all(cfrds_server *server, cfrds_socket sockfd, cfrds_buffer *send_buf)
{
    const char *send_ptr = cfrds_buffer_data(send_buf);
    size_t send_remaining = cfrds_buffer_data_size(send_buf);

    while (send_remaining > 0)
    {
        size_t send_size = send_remaining;
#ifdef _WIN32
        /* Winsock's send() length parameter is an int. */
        if (send_size > INT_MAX)
            send_size = INT_MAX;
#endif

        trace_net_start("send");
#ifdef _WIN32
        ssize_t sock_written = send(sockfd, send_ptr, (int)send_size, CFRDS_SEND_FLAGS);
#else
        ssize_t sock_written = send(sockfd, send_ptr, send_size, CFRDS_SEND_FLAGS);
#endif
        trace_net_end();
        if (sock_written <= 0)
        {
            int err = sock_written < 0 ? GET_SOCKET_ERRNO() : 0;
            if (sock_written < 0 && IS_SOCKET_EINTR(err))
                continue;
            server->_errno = err;
            cfrds_server_set_error(server, CFRDS_STATUS_WRITING_TO_SOCKET_FAILED,
                                   sock_written == 0
                                       ? "socket closed while writing..."
                                       : "failed to write to socket...");
            return CFRDS_STATUS_WRITING_TO_SOCKET_FAILED;
        }
        send_ptr += sock_written;
        send_remaining -= (size_t)sock_written;
    }
    return CFRDS_STATUS_OK;
}

static cfrds_status http_receive_response(cfrds_server *server, cfrds_socket sockfd, cfrds_buffer *tmp_response)
{
    time_t start_time = time(NULL);
    while (1)
    {
        if (time(NULL) - start_time > CFRDS_MAX_RESPONSE_TIMEOUT_SEC) {
            cfrds_server_set_error(server, CFRDS_STATUS_READING_FROM_SOCKET_FAILED, "response read timed out (overall deadline exceeded)");
            return CFRDS_STATUS_READING_FROM_SOCKET_FAILED;
        }

        char recv_buf[4096];
        trace_net_start("recv");
        ssize_t nread = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
        trace_net_end();
        if (nread <= 0) {
            if (nread == -1) {
                server->_errno = GET_SOCKET_ERRNO();
                cfrds_server_set_error(server, CFRDS_STATUS_READING_FROM_SOCKET_FAILED, "failed to read from socket...");
                return CFRDS_STATUS_READING_FROM_SOCKET_FAILED;
            }
            break;
        }

        if (!cfrds_buffer_append_bytes(tmp_response, recv_buf, (size_t)nread))
            return CFRDS_STATUS_MEMORY_ERROR;

        if (cfrds_buffer_data_size(tmp_response) > CFRDS_MAX_RESPONSE_SIZE) {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_TOO_LARGE, "response exceeded maximum size");
            return CFRDS_STATUS_RESPONSE_TOO_LARGE;
        }
    }
    return CFRDS_STATUS_OK;
}

cfrds_status cfrds_http_post(cfrds_server *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response)
{
    cfrds_buffer_defer(tmp_response);
    cfrds_buffer_defer(send_buf);
    cfrds_sock_defer(sockfd);
    cfrds_status status;

    if (!cfrds_buffer_create(&send_buf)) {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "cfrds_buffer_create failed for send_buf");
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    status = http_build_request(server, command, payload, send_buf);
    if (status != CFRDS_STATUS_OK)
        return status;

    status = http_connect(server, &sockfd);
    if (status != CFRDS_STATUS_OK)
        return status;

    status = http_send_all(server, sockfd, send_buf);
    if (status != CFRDS_STATUS_OK)
        return status;

    if (!cfrds_buffer_create(&tmp_response)) {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "cfrds_buffer_create failed for tmp_response");
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    status = http_receive_response(server, sockfd, tmp_response);
    if (status != CFRDS_STATUS_OK)
        return status;

    const char *response_data = cfrds_buffer_data(tmp_response);
    size_t response_size = cfrds_buffer_data_size(tmp_response);

    const char good_response_http1_1[] = "HTTP/1.1 200 ";
    size_t min_resp_len = strlen(good_response_http1_1);

    if (response_size < min_resp_len ||
        (strncmp(response_data, good_response_http1_1, min_resp_len) != 0 &&
         strncmp(response_data, "HTTP/1.0 200 ", min_resp_len) != 0))
    {
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "Invalid server response...");
        return CFRDS_STATUS_RESPONSE_ERROR;
    }

    if (cfrds_buffer_skip_httpheader(&response_data, &response_size) == false)
        return CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND;

    const char *body_start = response_data;
    size_t body_size = response_size;

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &server->error_code))
    {
        server->error_code = -1;
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "cfrds_buffer_parse_number FAILED...");
        return CFRDS_STATUS_RESPONSE_ERROR;
    }

    if (server->error_code < 0)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, response_data);
        return CFRDS_STATUS_RESPONSE_ERROR;
    }

    if (response)
    {
        cfrds_buffer *payload_buf = NULL;
        if (!cfrds_buffer_create(&payload_buf)) {
            cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "cfrds_buffer_create failed for response payload");
            return CFRDS_STATUS_MEMORY_ERROR;
        }
        if (body_size > 0 && !cfrds_buffer_append_bytes(payload_buf, body_start, body_size)) {
            cfrds_buffer_free(payload_buf);
            cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "cfrds_buffer_append_bytes failed for response payload");
            return CFRDS_STATUS_MEMORY_ERROR;
        }
        *response = payload_buf;
    }

    return CFRDS_STATUS_OK;
}

#ifdef _WIN32
static void cfrds_sock_cleanup(SOCKET* sock)
{
    if (sock)
    {
        if (*sock != INVALID_SOCKET)
        {
            closesocket(*sock);
            *sock = INVALID_SOCKET;
        }
    }
}
#else
static void cfrds_sock_cleanup(int* sock)
{
    if ((sock != NULL)&&(*sock != CFRDS_INVALID_SOCKET))
    {
        close(*sock);
        *sock = CFRDS_INVALID_SOCKET;
    }
}
#endif

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


#define CFRDS_MAX_RESPONSE_SIZE (100 * 1024 * 1024)

#ifdef _WIN32
typedef SOCKET cfrds_socket;
#define CFRDS_INVALID_SOCKET INVALID_SOCKET
#define GET_SOCKET_ERRNO() WSAGetLastError()
#define IS_SOCKET_EINTR(err) ((err) == WSAEINTR)
#else
typedef int cfrds_socket;
#define CFRDS_INVALID_SOCKET (-1)
#define GET_SOCKET_ERRNO() errno
#ifdef EINTR
#define IS_SOCKET_EINTR(err) ((err) == EINTR)
#else
#define IS_SOCKET_EINTR(err) false
#endif
#endif

void cfrds_sock_cleanup(cfrds_socket* sock);
#define cfrds_sock_defer(var) cfrds_socket var __attribute__((cleanup(cfrds_sock_cleanup))) = CFRDS_INVALID_SOCKET

static bool cfrds_buffer_skip_httpheader(const char **data, size_t *remaining)
{
    const char *body = NULL;

    if ((data == NULL) || (*data == NULL) || (remaining == NULL))
        return false;

    body = strstr(*data, "\r\n\r\n");
    if (body == NULL)
        return false;

    *remaining -= (size_t)(body - *data);
    *data = body + 4;
    *remaining -= 4;

    return true;
}

cfrds_status cfrds_http_post(cfrds_server *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response)
{

    cfrds_buffer_defer(tmp_response);
    cfrds_buffer_defer(swap_buf);
    cfrds_buffer_defer(send_buf);
    char datasize_str[16] = {0, };
    ssize_t sock_written = 0;
    uint16_t port = 0;
    cfrds_sock_defer(sockfd);

    int n = 0;

    port = cfrds_server_get_port(server);

    n = snprintf(datasize_str, sizeof(datasize_str), "%zu", cfrds_buffer_data_size(payload));
    if (n < 0)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "snprintf() returned < 0...");
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    if (!cfrds_buffer_create(&send_buf)) {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "cfrds_buffer_create failed for send_buf");
        return CFRDS_STATUS_MEMORY_ERROR;
    }
    cfrds_buffer_append(send_buf, "POST /CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=");
    cfrds_buffer_append(send_buf, command);
    cfrds_buffer_append(send_buf, " HTTP/1.0\r\nHost: ");
    cfrds_buffer_append(send_buf, cfrds_server_get_host(server));
    if(port != 80)
    {
        char port_str[16] = {0, };

        n = snprintf(port_str, sizeof(port_str), "%d", port);
        if (n < 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "snprintf() returned < 0...");
            return CFRDS_STATUS_MEMORY_ERROR;
        }
        cfrds_buffer_append(send_buf, ":");
        cfrds_buffer_append(send_buf, port_str);
    }
    cfrds_buffer_append(send_buf, "\r\nConnection: close\r\nUser-Agent: Mozilla/3.0 (compatible; Macromedia RDS Client)\r\nAccept: text/html, */*\r\nAccept-Encoding: deflate\r\nContent-type: text/html\r\nContent-length: ");
    cfrds_buffer_append(send_buf, datasize_str);
    cfrds_buffer_append(send_buf, "\r\n\r\n");
    cfrds_buffer_append_buffer(send_buf, payload);

    {
        struct addrinfo hints;
        struct addrinfo *result = NULL;
        char port_str[16] = {0, };

        explicit_bzero(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        snprintf(port_str, sizeof(port_str), "%u", port);

        trace_net_start("getaddrinfo");
        int gai_err = getaddrinfo(cfrds_server_get_host(server), port_str, &hints, &result);
        trace_net_end();
        if (gai_err != 0) {
            cfrds_server_set_error(server, CFRDS_STATUS_SOCKET_HOST_NOT_FOUND, "failed to resolve hostname...");
            return CFRDS_STATUS_SOCKET_HOST_NOT_FOUND;
        }

        for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
        {
            trace_net_start("socket");
            cfrds_socket fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            trace_net_end();
            if (fd == CFRDS_INVALID_SOCKET)
                continue;

            trace_net_start("connect");
            int res = connect(fd, rp->ai_addr, rp->ai_addrlen);
            trace_net_end();
            if (res == 0) {
                sockfd = fd;
                break;
            }

            cfrds_sock_cleanup(&fd);
        }

        int saved_errno = GET_SOCKET_ERRNO();
        freeaddrinfo(result);

        if (sockfd == CFRDS_INVALID_SOCKET) {
            server->_errno = saved_errno;
            cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to establish connection to the server...");
            return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
        }

        {
#ifdef _WIN32
            DWORD tv = 30000;
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
                server->_errno = GET_SOCKET_ERRNO();
                cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to set socket receive timeout");
                return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
            }
            if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
                server->_errno = GET_SOCKET_ERRNO();
                cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to set socket send timeout");
                return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
            }
#else
            struct timeval tv;
            tv.tv_sec = 30;
            tv.tv_usec = 0;
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                server->_errno = GET_SOCKET_ERRNO();
                cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to set socket receive timeout");
                return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
            }
            if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
                server->_errno = GET_SOCKET_ERRNO();
                cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to set socket send timeout");
                return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
            }
#endif
        }
    }

    {
        const char *send_ptr = cfrds_buffer_data(send_buf);
        size_t send_remaining = cfrds_buffer_data_size(send_buf);

        while (send_remaining > 0)
        {
            trace_net_start("send");
            sock_written = send(sockfd, send_ptr, send_remaining, 0);
            trace_net_end();
            if (sock_written < 0)
            {
                int err = GET_SOCKET_ERRNO();
                if (IS_SOCKET_EINTR(err))
                    continue;
                server->_errno = err;
                cfrds_server_set_error(server, CFRDS_STATUS_WRITING_TO_SOCKET_FAILED, "failed to write to socket...");
                return CFRDS_STATUS_WRITING_TO_SOCKET_FAILED;
            }
            send_ptr += sock_written;
            send_remaining -= (size_t)sock_written;
        }
    }

    if (!cfrds_buffer_create(&tmp_response)) {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "cfrds_buffer_create failed for tmp_response");
        return CFRDS_STATUS_MEMORY_ERROR;
    }
    while(1)
    {
        if (cfrds_buffer_reserve_above_size(tmp_response, 4096) == false)
            return CFRDS_STATUS_MEMORY_ERROR;

        trace_net_start("recv");
        ssize_t nread = recv(sockfd, cfrds_buffer_data(tmp_response) + cfrds_buffer_data_size(tmp_response), 4096, 0);
        trace_net_end();
        if (nread <= 0) {
            if (nread == -1) {
                server->_errno = GET_SOCKET_ERRNO();
                cfrds_server_set_error(server, CFRDS_STATUS_READING_FROM_SOCKET_FAILED, "failed to read from socket...");
                return CFRDS_STATUS_READING_FROM_SOCKET_FAILED;
            }
            break;
        }

        cfrds_buffer_expand(tmp_response, (size_t)nread);

        if (cfrds_buffer_data_size(tmp_response) > CFRDS_MAX_RESPONSE_SIZE) {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_TOO_LARGE, "response exceeded maximum size");
            return CFRDS_STATUS_RESPONSE_TOO_LARGE;
        }
    }

    const char *response_data = cfrds_buffer_data(tmp_response);
    size_t response_size = cfrds_buffer_data_size(tmp_response);

    static const char *good_response_http1_1 = "HTTP/1.1 200 ";

    if (strncmp(response_data, good_response_http1_1, strlen(good_response_http1_1)) != 0)
    {
        static const char *good_response_http1_0 = "HTTP/1.0 200 ";

        if (strncmp(response_data, good_response_http1_0, strlen(good_response_http1_0)) != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "Invalid server response...");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    if (cfrds_buffer_skip_httpheader(&response_data, &response_size) == false)
        return CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND;

    if (!cfrds_buffer_create(&swap_buf)) {
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    cfrds_buffer_append_bytes(swap_buf, response_data, response_size);
    response_data = cfrds_buffer_data(swap_buf);
    response_size = cfrds_buffer_data_size(swap_buf);
    cfrds_buffer_free(tmp_response);
    tmp_response = swap_buf; swap_buf = NULL;

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
        *response = tmp_response; tmp_response = NULL;
    }

    return CFRDS_STATUS_OK;
}

#ifdef _WIN32
void cfrds_sock_cleanup(SOCKET* sock)
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
void cfrds_sock_cleanup(int* sock)
{
    if ((sock != NULL)&&(*sock != CFRDS_INVALID_SOCKET))
    {
        close(*sock);
        *sock = CFRDS_INVALID_SOCKET;
    }
}
#endif

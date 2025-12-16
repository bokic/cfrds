#include <cfrds.h>
#include <internal/cfrds_int.h>
#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>


#if defined(__APPLE__) || defined(_WIN32)
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *ptr = (volatile unsigned char *)s;
    while (n--) {
        *ptr++ = 0;
    }
}
#endif

void cfrds_sock_cleanup(cfrds_socket* sock);
#define cfrds_sock_defer(var) cfrds_socket var __attribute__((cleanup(cfrds_sock_cleanup))) = CFRDS_INVALID_SOCKET

static bool cfrds_buffer_skip_httpheader(const char **data, size_t *remaining)
{
    const char *body = nullptr;

    body = strstr(*data, "\r\n\r\n");
    if (body == nullptr)
        return false;

    *remaining -= body - *data;
    *data = body + 4;
    *remaining -= 4;

    return true;
}

enum cfrds_status cfrds_http_post(cfrds_server_int *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response)
{
    struct sockaddr_in servaddr;

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

    cfrds_buffer_create(&send_buf);
    cfrds_buffer_append(send_buf, "POST /CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=");
    cfrds_buffer_append(send_buf, command);
    cfrds_buffer_append(send_buf, " HTTP/1.0\r\nHost: ");
    cfrds_buffer_append(send_buf, cfrds_server_get_host(server));
    if(port != 80)
    {
        char port_str[8] = {0, };

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

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        server->_errno = errno;
        cfrds_server_set_error(server, CFRDS_STATUS_SOCKET_CREATION_FAILED, "failed to create socket...");
        return CFRDS_STATUS_SOCKET_CREATION_FAILED;
    }
    server->socket = sockfd;

    explicit_bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(cfrds_server_get_host(server));
    if (servaddr.sin_addr.s_addr == INADDR_NONE) {
        return CFRDS_STATUS_SOCKET_HOST_NOT_FOUND;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        server->_errno = errno;
        cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to establish connection to the server...");
        return CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
    }

    sock_written = send(sockfd, cfrds_buffer_data(send_buf), cfrds_buffer_data_size(send_buf), 0);
    if ((sock_written < 0)||((unsigned)sock_written != cfrds_buffer_data_size(send_buf))) {
        if (sock_written == -1)
        {
            server->_errno = errno;
            cfrds_server_set_error(server, CFRDS_STATUS_WRITING_TO_SOCKET_FAILED, "failed to write to socket...");
            return CFRDS_STATUS_WRITING_TO_SOCKET_FAILED;
        }
        else
        {
            server->_errno = errno;
            cfrds_server_set_error(server, CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET, "failed to write to all data socket...");
            return CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET;
        }
    }

    cfrds_buffer_create(&tmp_response);
    while(1)
    {
        cfrds_buffer_reserve_above_size(tmp_response, 4096);

        ssize_t readed = recv(sockfd, cfrds_buffer_data(tmp_response) + cfrds_buffer_data_size(tmp_response), 4096, 0);
        if (readed <= 0) {
            if (readed == -1) {
                server->_errno = errno;
                cfrds_server_set_error(server, CFRDS_STATUS_READING_FROM_SOCKET_FAILED, "failed to read from socket...");
                return CFRDS_STATUS_READING_FROM_SOCKET_FAILED;
            }
            break;
        }

        cfrds_buffer_expand(tmp_response, readed);
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

    cfrds_buffer_create(&swap_buf);
    cfrds_buffer_append_bytes(swap_buf, response_data, response_size);
    response_data = cfrds_buffer_data(swap_buf);
    response_size = cfrds_buffer_data_size(swap_buf);
    cfrds_buffer_free(tmp_response);
    tmp_response = swap_buf; swap_buf = nullptr;

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
        *response = tmp_response; tmp_response = nullptr;
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
    if (*sock)
    {
        if (*sock > 0)
        {
            close(*sock);
            *sock = 0;
        }
    }
}
#endif


void cfrds_sock_shutdown(cfrds_socket sock)
{
    if (sock)
    {
        if (sock != CFRDS_INVALID_SOCKET)
        {
#ifdef _WIN32
            shutdown(sock, SD_BOTH);
#else
            shutdown(sock, SHUT_RDWR);
#endif
        }
    }
}

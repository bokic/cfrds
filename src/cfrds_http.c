#include <cfrds.h>
#include <internal/cfrds_int.h>
#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>


enum cfrds_status cfrds_http_post(cfrds_server_int *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response)
{
    struct sockaddr_in servaddr;
    enum cfrds_status ret = CFRDS_STATUS_OK;
    cfrds_buffer *int_response = NULL;
    cfrds_buffer *send_buf = NULL;
    char *datasize_str = NULL;
    ssize_t sock_written = 0;
    uint16_t port = 0;
    int sockfd = -1;
    int n = 0;

    port = cfrds_server_get_port(server);

    n = asprintf(&datasize_str, "%zu", cfrds_buffer_data_size(payload));
    if (n < 0)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "asprintf() returned -1...");
        return CFRDS_STATUS_MEMORY_ERROR;
    }

    cfrds_buffer_create(&send_buf);
    cfrds_buffer_append(send_buf, "POST /CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=");
    cfrds_buffer_append(send_buf, command);
    cfrds_buffer_append(send_buf, " HTTP/1.0\r\nHost: ");
    cfrds_buffer_append(send_buf, cfrds_server_get_host(server));
    if(port != 80)
    {
        char *port_str = NULL;
        int n = 0;

        n = asprintf(&port_str, "%d", port);
        if (n < 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_MEMORY_ERROR, "asprintf() returned -1...");
            ret = CFRDS_STATUS_MEMORY_ERROR;
            goto exit;
        }
        cfrds_buffer_append(send_buf, ":");
        cfrds_buffer_append(send_buf, port_str);

        free(port_str);
    }
    cfrds_buffer_append(send_buf, "\r\nConnection: close\r\nUser-Agent: Mozilla/3.0 (compatible; Macromedia RDS Client)\r\nAccept: text/html, */*\r\nAccept-Encoding: deflate\r\nContent-type: text/html\r\nContent-length: ");
    cfrds_buffer_append(send_buf, datasize_str);
    cfrds_buffer_append(send_buf, "\r\n\r\n");
    cfrds_buffer_append_buffer(send_buf, payload);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        server->_errno = errno;
        cfrds_server_set_error(server, CFRDS_STATUS_SOCKET_CREATION_FAILED, "failed to create socket...");
        ret = CFRDS_STATUS_SOCKET_CREATION_FAILED;
        goto exit;
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(cfrds_server_get_host(server));

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        server->_errno = errno;
        cfrds_server_set_error(server, CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "failed to establish connection to the server...");
        ret = CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED;
        goto exit;
    }

    sock_written = write(sockfd, cfrds_buffer_data(send_buf), cfrds_buffer_data_size(send_buf));
    if ((sock_written < 0)||((unsigned)sock_written != cfrds_buffer_data_size(send_buf))) {
        if (sock_written == -1)
        {
            server->_errno = errno;
            cfrds_server_set_error(server, CFRDS_STATUS_WRITING_TO_SOCKET_FAILED, "failed to write to socket...");
            ret = CFRDS_STATUS_WRITING_TO_SOCKET_FAILED;
            goto exit;
        }
        else
        {
            server->_errno = errno;
            cfrds_server_set_error(server, CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET, "failed to write to all data socket...");
            ret = CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET;
            goto exit;
        }
    }

    cfrds_buffer_create(&int_response);
    while(1)
    {
        cfrds_buffer_reserve_above_size(int_response, 4096);

        ssize_t readed = read(sockfd, cfrds_buffer_data(int_response) + cfrds_buffer_data_size(int_response), 4096);
        if (readed <= 0) {
            if (readed == -1) {
                server->_errno = errno;
                cfrds_server_set_error(server, CFRDS_STATUS_READING_FROM_SOCKET_FAILED, "failed to read from socket...");
                ret = CFRDS_STATUS_READING_FROM_SOCKET_FAILED;
                goto exit;
            }
            break;
        }

        cfrds_buffer_expand(int_response, readed);
    }

    cfrds_buffer_append_char(int_response, '\0');
    char *response_data = cfrds_buffer_data(int_response);
    size_t response_size = cfrds_buffer_data_size(int_response);

    static const char *good_response = "HTTP/1.1 200 ";

    if (strncmp(response_data, good_response, strlen(good_response)) != 0)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "Invalid server response...");
        ret = CFRDS_STATUS_RESPONSE_ERROR;
        goto exit;
    }

    if (cfrds_buffer_skip_httpheader(&response_data, &response_size))
    {
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &server->error_code))
        {
            server->error_code = -1;
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "cfrds_buffer_parse_number FAILED...");
            ret = CFRDS_STATUS_RESPONSE_ERROR;
            goto exit;
        }

        if (server->error_code < 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, response_data);
            ret = CFRDS_STATUS_RESPONSE_ERROR;
            goto exit;
        }
    }
    else
    {
        ret = CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND;
    }

exit:
    if (datasize_str)
        free(datasize_str);
    if (sockfd != -1)
        close(sockfd);
    if (response)
        *response = int_response;
    else
        cfrds_buffer_free(int_response);
    if (send_buf)
        cfrds_buffer_free(send_buf);

    return ret;
}

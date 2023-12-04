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
#include <stddef.h>
#include <stdio.h>
#include <errno.h>


cfrds_buffer *cfrds_http_post(cfrds_server *server, const char *command, cfrds_buffer *payload)
{
    cfrds_buffer *send_buf = NULL;
    cfrds_buffer *recv_buf = NULL;
    char datasize_str[16] = {0, };
    ssize_t sock_written = 0;
    uint16_t port = 0;

    port = cfrds_server_get_port(server);

    snprintf(datasize_str, sizeof(datasize_str),"%zu", cfrds_buffer_data_size(payload));

    cfrds_buffer_create(&send_buf);
    cfrds_buffer_append(send_buf, "POST /CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=");
    cfrds_buffer_append(send_buf, command);
    cfrds_buffer_append(send_buf, " HTTP/1.0\r\nHost: ");
    cfrds_buffer_append(send_buf, cfrds_server_get_host(server));
    if(port != 80)
    {
        char port_str[8] = {0, };

        snprintf(port_str, sizeof(port_str), "%d", port);
        cfrds_buffer_append(send_buf, ":");
        cfrds_buffer_append(send_buf, port_str);
    }
    cfrds_buffer_append(send_buf, "\r\nConnection: close\r\nUser-Agent: Mozilla/3.0 (compatible; Macromedia RDS Client)\r\nAccept: text/html, */*\r\nAccept-Encoding: deflate\r\nContent-type: text/html\r\nContent-length: ");
    cfrds_buffer_append(send_buf, datasize_str);
    cfrds_buffer_append(send_buf, "\r\n\r\n");
    cfrds_buffer_append_buffer(send_buf, payload);

    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cfrds_buffer_free(send_buf);
        cfrds_server_set_error(server, -1, "socket creation failed...");
        return NULL;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(cfrds_server_get_host(server));

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        cfrds_buffer_free(send_buf);
        cfrds_server_set_error(server, -1, "connection with the server failed...");
        close(sockfd);
        return NULL;
    }

    sock_written = write(sockfd, cfrds_buffer_data(send_buf), cfrds_buffer_data_size(send_buf));
    cfrds_buffer_free(send_buf);
    if (sock_written == -1)
    {
        cfrds_server_set_error(server, -1, strerror(errno));
        close(sockfd);
        return NULL;
    }

    cfrds_buffer_create(&recv_buf);
    while(1)
    {
        cfrds_buffer_reserve_above_size(recv_buf, 4096);

        ssize_t readed = read(sockfd, cfrds_buffer_data(recv_buf) + cfrds_buffer_data_size(recv_buf), 4096);
        if (readed == 0)
            break;

        cfrds_buffer_expand(recv_buf, readed);
    }

    close(sockfd);

    return recv_buf;
}

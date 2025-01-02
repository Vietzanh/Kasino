#include "main.h"

int main(void)
{
    int listener = get_listener_socket("127.0.0.1", "8080", 100);

    if (listener == -1)
    {
        return 1;
    }

    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listener;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event) == -1)
    {
        perror("epoll_ctl");
        return 1;
    }

    if (set_nonblocking(listener) == -1)
    {
        fprintf(stderr, "main: Cannot set nonblocking");
        return 1;
    }

    struct epoll_event *events = calloc(MAXEVENTS, sizeof(event));

    if (events == NULL)
    {
        perror("calloc");
        return 1;
    }

    for (;;)
    {
        int n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);

        for (int i = 0; i < n; i++)
        {
            if (events[i].data.fd == listener)
            {
                int client_fd = accept_connection(listener);

                if (client_fd == -1)
                {
                    continue;
                }

                if (set_nonblocking(client_fd) == -1)
                {
                    fprintf(stderr, "main: Cannot set nonblocking");
                    return 1;
                }

                if (add_connection_to_epoll(epoll_fd, client_fd) == -1)
                {
                    fprintf(stderr, "main: Cannot add client to epoll");
                    return 1;
                }
            }
            else
            {
                conn_data_t *conn_data = events[i].data.ptr;
                if (!conn_data || conn_data->fd <= 0)
                {
                    fprintf(stderr, "Invalid connection data or fd\n");
                    continue;
                }

                char buf[MAXLINE];
                memset(buf, 0, MAXLINE);

                int nbytes = recv(conn_data->fd, buf, MAXLINE, 0);

                if (nbytes < 0)
                {
                    if (nbytes < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // No data available; try again later
                            continue;
                        }
                        perror("recv");
                        close_connection(epoll_fd, conn_data);
                        continue;
                    }
                    fprintf(stderr, "main: Cannot receive data\n");
                    close_connection(epoll_fd, conn_data);
                    continue;
                }
                else if (nbytes == 0)
                {
                    printf("Client %d disconnected\n", conn_data->fd);
                    close_connection(epoll_fd, conn_data);
                    continue;
                }
                Header *header = decode_header(buf);

                if (header == NULL)
                {
                    fprintf(stderr, "main: Cannot decode header\n");
                    close_connection(epoll_fd, conn_data);
                    continue;
                }

                switch (ntohs(header->packet_type))
                {
                case 100: // Login
                    printf("Login request from client %d\n", conn_data->fd);
                    handle_login_request(conn_data, buf, nbytes);
                    break;

                case 200: // Signup
                    printf("Signup request from client %d\n", conn_data->fd);
                    handle_signup_request(conn_data, buf, nbytes);
                    break;

                default:
                    break;
                }

                free(header);
                memset(buf, 0, MAXLINE);
            }
        }
    }
}
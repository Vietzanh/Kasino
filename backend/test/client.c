#include "main.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s [host] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int servfd;
    struct addrinfo hints, *res;

    // Prepare for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(argv[1], argv[2], &hints, &res) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Create socket
    servfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (servfd == -1)
    {
        perror("socket");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(servfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("connect");
        freeaddrinfo(res);
        close(servfd);
        exit(EXIT_FAILURE);
    }

    char recv_buffer[MAXLINE];

    while (1)
    {
        printf("Enter username <space> password: ");

        char username[30];
        char password[30];

        // Use safer input to avoid overflow
        if (scanf("%29s %29s", username, password) != 2)
        {
            fprintf(stderr, "Invalid input. Please enter username and password.\n");
            continue;
        }

        // Initialize MPack writer
        mpack_writer_t writer;
        char buffer[4096];
        mpack_writer_init(&writer, buffer, 4096);

        // Write data to the MPack map
        mpack_start_map(&writer, 2);
        mpack_write_cstr(&writer, "user");
        mpack_write_cstr(&writer, username);
        mpack_write_cstr(&writer, "pass");
        mpack_write_cstr(&writer, password);
        mpack_finish_map(&writer);

        // Check for MPack errors

        // Retrieve MPack buffer
        const char *data = writer.buffer;
        size_t size = mpack_writer_buffer_used(&writer);

        // Encode the packet
        RawBytes *encoded_message = encode_packet(1, 100, data, size);

        // Send the encoded message
        if (send(servfd, encoded_message->data, encoded_message->len, 0) == -1)
        { // Include header size (5 bytes)
            perror("send");
            free(encoded_message);
            break;
        }
        mpack_error_t error = mpack_writer_destroy(&writer);
        if (error != mpack_ok)
        {
            fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(error));
            break;
        }

        if (recv(servfd, recv_buffer, MAXLINE, 0) == -1)
        {
            perror("recv");
            break;
        }

        // Decode the response
        Header *header = decode_header(recv_buffer);
        if (header == NULL)
        {
            fprintf(stderr, "Invalid header\n");
            break;
        }

        if (header->packet_type == 100)
        {
            mpack_reader_t reader;
            mpack_reader_init(&reader, recv_buffer + sizeof(Header), header->packet_len - sizeof(Header), header->packet_len - sizeof(Header));
            // if there is error, the payload will contains 2 fields: res and msg, if not, it will contains only res

            mpack_expect_map_max(&reader, 1);
            mpack_expect_cstr_match(&reader, "res");
            int res = mpack_expect_u16(&reader);

            if (res == R_LOGIN_OK)
            {
                printf("Login success with code %d\n", res);
            }
            else
            {
                printf("Login failed with code %d\n", res);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet type\n");
        }

        // Free dynamically allocated memory
        free(encoded_message);
    }

    // Clean up resources
    freeaddrinfo(res);
    close(servfd);

    return 0;
}
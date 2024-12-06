#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_info_t;

// Thread function to handle client connections
void *handle_client(void *arg) {
    client_info_t *client_info = (client_info_t *)arg;
    int client_socket = client_info->client_socket;

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    printf("Client connected: %s:%d\n",
           inet_ntoa(client_info->client_addr.sin_addr),
           ntohs(client_info->client_addr.sin_port));

    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Received: %s", buffer);

        // Echo the message back to the client
        send(client_socket, buffer, bytes_read, 0);
    }

    printf("Client disconnected: %s:%d\n",
           inet_ntoa(client_info->client_addr.sin_addr),
           ntohs(client_info->client_addr.sin_port));

    close(client_socket);
    free(client_info);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number.\n");
        return 1;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Allocate memory for client info and spawn a thread
        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (!client_info) {
            perror("Failed to allocate memory");
            close(client_socket);
            continue;
        }

        client_info->client_socket = client_socket;
        client_info->client_addr = client_addr;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, client_info) != 0) {
            perror("Failed to create thread");
            free(client_info);
            close(client_socket);
            continue;
        }

        pthread_detach(client_thread); // Automatically reclaim thread resources when done
    }

    close(server_socket);
    return 0;
}

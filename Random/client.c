// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Loopback address for local testing
    server_addr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    int sndbuf_size;
    socklen_t optlen = sizeof(sndbuf_size);
    if (getsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, &optlen) == 0) {
        printf("Send buffer size: %d bytes\n", sndbuf_size);
    } else {
        perror("Getsockopt failed");
    }
    char message[sndbuf_size+10000];
    for(int i = 0; i < sndbuf_size+9999; i++) {
        message[i] = 'a';
    }
    message[sndbuf_size+9999] = '\0';
    // Send data to the server in a single send call
    ssize_t send_size = send(client_socket, message, strlen(message), 0);
    if (send_size < 0) {
        perror("Send failed");
    } else {
        printf("Sent to server: %s\n", message);
    }

    // Close the socket
    close(client_socket);

    return 0;
}

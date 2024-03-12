#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<errno.h>
int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);  // Replace with the desired port

    // Connect the UDP socket to a default destination address
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Send data to the connected address
    const char* message = "Hello, connected UDP socket!";
    if (send(sockfd, message, strlen(message), 0) < 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    // Send data to a different address using sendto
    struct sockaddr_in another_addr;
    another_addr.sin_family = AF_INET;
    another_addr.sin_addr.s_addr = INADDR_ANY;  // Replace with the desired destination IP
    another_addr.sin_port = htons(12345);  // Replace with the desired destination port

    const char* another_message = "Hello, another address!";
    if (sendto(sockfd, another_message, strlen(another_message), 0, (struct sockaddr*)&another_addr, sizeof(another_addr)) < 0) {
        perror("sendto");
        if(errno == EISCONN)
        {
            printf("ISCONN");
        }
        exit(EXIT_FAILURE);
    }

    // Rest of the code...

    // Close the socket when done
    close(sockfd);

    return 0;
}

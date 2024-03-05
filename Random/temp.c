#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include<sys/wait.h>
#include<signal.h>
#include<errno.h>
#include<sys/time.h>
#include<sys/select.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int sndbuf_size;
    socklen_t optlen = sizeof(sndbuf_size);

    // Get the send buffer size
    if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sndbuf_size, &optlen) == 0) {
        printf("Send buffer size: %d bytes\n", sndbuf_size);
    } else {
        perror("Getsockopt failed");
    }

    // Don't forget to close the socket when done
    close(sockfd);

    return 0;
}

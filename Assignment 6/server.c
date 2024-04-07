#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <linux/ip.h>
#include <linux/if_ether.h>

#define MAX_DOMAIN_SIZE 32
#define MAX_QUERIES 8

typedef struct {
    uint16_t id;
    uint8_t message_type;
    uint8_t num_queries;
    struct {
        uint8_t size;
        char domain[MAX_DOMAIN_SIZE];
    } queries[MAX_QUERIES];
} simDNS_QueryPacket;


int main() {
    // Create a raw socket for receiving DNS queries
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    struct sockaddr_ll sll;
    struct ifreq ifr;

    bzero(&sll, sizeof(sll));
    bzero(&ifr, sizeof(ifr));

    strcpy((char *)ifr.ifr_name, "enp0s3");
    // strcpy((char *)ifr.ifr_name, "lo");

    if ((ioctl(sockfd, SIOCGIFINDEX, &ifr)) == -1)
    {
        perror("Unable to find interface index");
        close(sockfd);
        exit(1);
    }

    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    if (bind(sockfd, (struct sockaddr *)&sll, sizeof(sll)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }

    printf("Server is listening for DNS queries...\n");

    // Main server loop to receive and handle DNS queries
    while (1) {
        simDNS_QueryPacket query_packet;

        // Receive DNS query
        ssize_t bytes_received = recv(sockfd, &query_packet, sizeof(query_packet), 0);
        if (bytes_received < 0) {
            perror("recv");
            continue;
        }

        // Process received DNS query
        printf("Received DNS query:\n");
        printf("ID: %u\n", query_packet.id);
        printf("Message Type: %u\n", query_packet.message_type);
        printf("Number of Queries: %u\n", query_packet.num_queries);

        for (int i = 0; i < query_packet.num_queries; i++) {
            printf("Query %d: %s\n", i + 1, query_packet.queries[i].domain);
        }

        // Simulate DNS response (not implemented in this example)
        // You would typically formulate a DNS response here and send it back to the client

        // For this example, we'll simply print a message indicating that the response is simulated
        printf("Simulated DNS response sent.\n");
    }

    // Close the socket
    close(sockfd);
    return 0;
}

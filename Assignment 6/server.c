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
#include <netinet/ip.h>

int main() {
    // Create a raw socket for receiving all Ethernet types
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

    strcpy((char *)ifr.ifr_name, "wlp3s0"); // Change interface name as needed

    if ((ioctl(sockfd, SIOCGIFINDEX, &ifr)) == -1) {
        perror("Unable to find interface index");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening for Ethernet packets...\n");

    // Main server loop to receive and handle Ethernet packets
    while (1) {
        unsigned char buffer[2048];
        struct sockaddr_in source, dest;
        struct ethhdr *eth_header;

        // Receive Ethernet packet
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            perror("recv");
            continue;
        }

        // Extract Ethernet header
        eth_header = (struct ethhdr *)buffer;

        // Check if it's an IP packet
        if ((unsigned int)ntohs(eth_header->h_proto) != ETH_P_IP)
        {
            // printf("Not an IP packet,%d,%d\n",ntohs(eth_header->h_proto),ETH_P_IP);
            // printf("Destination MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            //        eth_header->h_dest[0], eth_header->h_dest[1], eth_header->h_dest[2],
            //        eth_header->h_dest[3], eth_header->h_dest[4], eth_header->h_dest[5]);
            // printf("Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",eth_header->h_source[0], eth_header->h_source[1], eth_header->h_source[2],
            //        eth_header->h_source[3], eth_header->h_source[4], eth_header->h_source[5]);
            continue;
        }

        // Extract IP header
        struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));

        // Print IP packet content
        if((unsigned int)ip_header->protocol !=254)continue;
        printf("Received IP Packet:\n");
        printf("Version: %u\n", (unsigned int)ip_header->version);
        printf("Header Length: %u bytes\n", (unsigned int)(ip_header->ihl * 4));
        printf("Total Length: %u bytes\n", ntohs(ip_header->tot_len));
        printf("Source IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
        printf("Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->daddr));
        printf("Protocol: %u\n", (unsigned int)ip_header->protocol);
        printf("Destination MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   eth_header->h_dest[0], eth_header->h_dest[1], eth_header->h_dest[2],
                   eth_header->h_dest[3], eth_header->h_dest[4], eth_header->h_dest[5]);
            printf("Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",eth_header->h_source[0], eth_header->h_source[1], eth_header->h_source[2],
                   eth_header->h_source[3], eth_header->h_source[4], eth_header->h_source[5]);
        printf("\n");
    }

    // Close the socket
    close(sockfd);
    return 0;
}

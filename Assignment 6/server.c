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
//Include library for host
#include <netdb.h>



#define MAX_DOMAIN_SIZE 31
#define MAX_QUERIES 8


typedef struct {
    uint16_t id;
    uint8_t message_type;
    uint8_t num_queries;
    struct {
        uint8_t size;
        char domain[MAX_DOMAIN_SIZE];
    } queries[MAX_QUERIES];
} simDNS_ResponsePacket;

void AppendEthernetHeader(unsigned char *packet)
{
    struct ethhdr *eth = (struct ethhdr *)packet;

    // Destination MAC address
    eth->h_dest[0] = 0x08;
    eth->h_dest[1] = 0x00;
    eth->h_dest[2] = 0x27;
    eth->h_dest[3] = 0xe3;
    eth->h_dest[4] = 0x2a;
    eth->h_dest[5] = 0xa0;

    // Source MAC address
    eth->h_source[0] = 0x08;
    eth->h_source[1] = 0x00;
    eth->h_source[2] = 0x27;
    eth->h_source[3] = 0xe3;
    eth->h_source[4] = 0x2a;
    eth->h_source[5] = 0xa0;


    // Protocol type
    eth->h_proto = htons(ETH_P_IP);

    return;
}

void AppendIPHeader(unsigned char *packet)
{
    struct iphdr *ip = (struct iphdr *)(packet + sizeof(struct ethhdr));

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr)+1000);
    ip->id = htons(1111);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = 254;
    ip->check = 0;
    ip->saddr = inet_addr("10.145.104.35");
    ip->daddr = inet_addr("10.145.104.35");

    ip->check = 0;

    // Calculate the IP checksum
    unsigned short *buffer = (unsigned short *)ip;
    int length = sizeof(struct iphdr);
    unsigned long sum = 0;
    while (length > 1)
    {
        sum += *buffer++;
        length -= 2;
    }

    if (length > 0)
    {
        sum += ((*buffer) & htons(0xFF00));
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    sum = ~sum;
    ip->check = ((unsigned short)sum);

    return;
}

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

    strcpy((char *)ifr.ifr_name, "enp0s3"); // Change interface name as needed

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
        if ((unsigned int)ntohs(eth_header->h_proto) != ETH_P_IP)
        {
            // printf("Not an IP packet,%d,%d\n",ntohs(eth_header->h_proto),ETH_P_IP);
            // printf("Destination MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            //        eth_header->h_dest[0], eth_header->h_dest[1], eth_header->h_dest[2],
            //        eth_header->h_dest[3], eth_header->h_dest[4], eth_header->h_dest[5]);
            // printf("Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",eth_header->h_source[0], eth_header->h_source[1], eth_header->h_source[2],
            //        eth_header->h_source[3], eth_header->h_source[4], eth_header->h_source[5]);
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

        if(ip_header->protocol!=254){
            //Discard the packet
            continue;
        }
        //Read the number of queries in the packet
        int offset = sizeof(struct ethhdr) + sizeof(struct iphdr);
        uint8_t *ptr = buffer + offset;

        // Extract ID
        uint16_t id = ntohs(*(uint16_t *)ptr);
        printf("ID: %d\n", id);
        ptr += 2;

        // Extract message type and number of queries
        uint8_t type_and_queries = *ptr;
        uint8_t message_type = (type_and_queries >> 7) & 0x01;
        uint8_t num_queries = type_and_queries & 0x07;
        printf("Message Type: %s\n", message_type ? "Response" : "Query");
        printf("Number of Queries: %d\n", num_queries);
        ptr++;

        char *response_packet = (char *)malloc(65536);
        memset(response_packet, 0, 65536);

        // Append Ethernet header
        AppendEthernetHeader((unsigned char *)response_packet);
        // Append IP header
        AppendIPHeader((unsigned char *)response_packet);

        //Copy ID of the query packet
        char *ptr2 = response_packet + sizeof(struct ethhdr) + sizeof(struct iphdr);
        *ptr2=id;
        ptr2++;
        //Copy message type
        *ptr2 = 1 << 7;
        ptr2++;
        //Copy number of queries
        *ptr2 = num_queries;
        ptr2++;
        

    // Extract query strings
        for (int i = 0; i < num_queries; i++) {
            uint8_t size = *ptr;
            ptr++;

            char domain[MAX_DOMAIN_SIZE + 1]; // Extra byte for null-terminator
            memcpy(domain, ptr, size);
            domain[size] = '\0'; // Null-terminate the string
            ptr += size;
            struct hostent *host = gethostbyname(domain);
            if (host != NULL && host->h_addr_list[0] != NULL) {
                *ptr2 = 1 << 7;
                ptr2++;
                memcpy(ptr2, host->h_addr_list[0], host->h_length);
            }
            else {
                *ptr2 = 0 << 7;
                ptr2++;
                char *temp="000";
                memcpy(ptr2, temp, 4);
            }


        }
        // Send the response packet
        int response_packet_size = ptr2 - response_packet;
        ssize_t bytes_sent = sendto(sockfd, response_packet, response_packet_size, 0, (struct sockaddr *)&sll, sizeof(sll));
        if (bytes_sent < 0) {
            perror("sendto");
            return -1;
        }
        printf("Response packet sent\n");
        
    
    }

    



    // Close the socket
    close(sockfd);
    return 0;
}
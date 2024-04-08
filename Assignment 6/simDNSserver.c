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
#include <bits/ioctls.h>
#include <stdio.h>
#include <time.h>
#include <sys/select.h>
#include <netdb.h>


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

typedef struct{
    uint16_t id;
    uint8_t message_type;
    uint8_t num_queries;
    struct {
        uint8_t valid;
        char ip[MAX_DOMAIN_SIZE];
    } queries[MAX_QUERIES];
} simDNS_ResponsePacket;
int droppacket(){
    srand(time(0));
    int random = rand()%100;
    if(random<20){
        return 1;
    }
    return 0;
}
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
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = inet_addr("127.0.0.1");

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

int main(){
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

    while(1)
    {
        size_t struct_size = sizeof(simDNS_QueryPacket);
        size_t packet_size = struct_size + sizeof(struct ethhdr) + sizeof(struct iphdr);
        unsigned char *packet = malloc(packet_size);

        // Receive a packet
        ssize_t len = recv(sockfd, packet, packet_size, 0);
        if (len < 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        struct ethhdr *eth_header;
        eth_header = (struct ethhdr *)packet;

        // Check if it's an IP packet
        if ((unsigned int)ntohs(eth_header->h_proto) != ETH_P_IP)
        {
            continue;
        }

        // Extract IP header
        struct iphdr *ip_header = (struct iphdr *)(packet + sizeof(struct ethhdr));

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

        if(ip_header->protocol!=254){
            //Discard the packet
            continue;
        }
        if(droppacket())
        {
            printf("Server dropping packet\n");
            continue;
        }
        int offset = sizeof(struct ethhdr) + sizeof(struct iphdr);

        simDNS_QueryPacket *query_packet = (simDNS_QueryPacket *)(packet + offset);
        if(query_packet->message_type==1)continue;
        printf("Received DNS Query Packet:\n");
        printf("ID: %u\n", ntohs(query_packet->id));
        printf("Message Type: %u\n", query_packet->message_type);
        printf("Number of Queries: %u\n", query_packet->num_queries);
        int i;
        for (i = 0; i < query_packet->num_queries; i++)
        {
            printf("Query %d: %s\n", i + 1, query_packet->queries[i].domain);
        }

        //Generate a response packet
        simDNS_ResponsePacket *response_packet = (simDNS_ResponsePacket *)malloc(sizeof(simDNS_ResponsePacket));
        response_packet->id = query_packet->id;
        response_packet->message_type = 1;
        response_packet->num_queries = query_packet->num_queries;
        printf("Sending %d responses\n", response_packet->num_queries);

        for(i=0;i<query_packet->num_queries;i++){

            struct hostent *host = gethostbyname(query_packet->queries[i].domain);

            printf("\nDomain: %s\n", query_packet->queries[i].domain);
            
            if (host != NULL && host->h_addr_list[0] != NULL) {
                response_packet->queries[i].valid = 1;
                char *ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
                printf("IP Address: %s\n", ip);
                memcpy(response_packet->queries[i].ip, ip, 32);
                printf("IP Address2: %s\n", response_packet->queries[i].ip);
            }
            else {
                response_packet->queries[i].valid = 0;
                printf("IP Address: invalid\n");
            }
        }

        unsigned char *response_packet_buffer = (malloc(sizeof(simDNS_ResponsePacket) + sizeof(struct ethhdr) + sizeof(struct iphdr)));
        memcpy(response_packet_buffer + sizeof(struct ethhdr) + sizeof(struct iphdr), response_packet, sizeof(simDNS_ResponsePacket));
        AppendEthernetHeader(response_packet_buffer);
        AppendIPHeader(response_packet_buffer);

        // Send the response packet
        if (send(sockfd, response_packet_buffer, sizeof(simDNS_ResponsePacket) + sizeof(struct ethhdr) + sizeof(struct iphdr), 0) < 0) {
            perror("send");
            exit(EXIT_FAILURE);
        }
        printf("Sent successfully\n");
        fflush(stdout);


    }
}
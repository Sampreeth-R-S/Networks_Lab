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

int is_valid_domain(const char *domain) {

   printf("Domain: %s\n", domain);
    size_t len = strlen(domain);
    if (len < 3 || len > 31)
        {
            printf("Invalid length: %ld\n", len);
            return 0;
            }

    for (size_t i = 0; i < len-1; i++) {
        if (!((domain[i] >= 'a' && domain[i] <= 'z') ||
              (domain[i] >= 'A' && domain[i] <= 'Z') ||
              (domain[i] >= '0' && domain[i] <= '9') ||
              (domain[i] == '-') ||
              (domain[i] == '.' && i > 0 && i < len - 1)))
            {
                printf("Invalid character: %c\n", domain[i]);
                return 0;
                }
    }

    for (size_t i = 0; i < len - 1; i++) {
        if (domain[i] == '-' && domain[i + 1] == '-')
            {
                printf("Invalid character2: %c\n", domain[i]);
                return 0;
            }
    }

    if (domain[0] == '-' || domain[len - 1] == '-')
        {
            printf("Invalid character3: %c\n", domain[0]);
            return 0;
        }


    return 1;
}

int main(){
    while(1)
    {
        int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sockfd < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        struct ifreq if_idx;
        memset(&if_idx, 0, sizeof(struct ifreq));
        strncpy(if_idx.ifr_name, "enp0s3", IFNAMSIZ-1);
        if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
            perror("SIOCGIFINDEX");
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

        simDNS_QueryPacket query;
        query.id = 1;
        query.message_type = 0;

        char query_string[256];
        printf("Enter query string (getIP N <domain-1> <domain-2> ... <domain-N>): ");
        fgets(query_string, sizeof(query_string), stdin);

        // Parse query string
        int num_queries;
        char domain[MAX_QUERIES][MAX_DOMAIN_SIZE];
        if (sscanf(query_string, "getIP %d", &num_queries) != 1 || num_queries < 1 || num_queries > 8) {
            printf("Invalid query format or number of queries. Please try again.\n");
            continue;
        }

        
        printf("num_queries: %d\n", num_queries);
        int valid = 1;
        char *token = strtok(query_string, " ");
        token = strtok(NULL," ");
        for (int i = 0; i < num_queries; i++) {
            token = strtok(NULL, " ");
            printf("token: %s\n", token);
            //Remove newline character if present
            if (token[strlen(token) - 1] == '\n')
                token[strlen(token) - 1] = '\0';
            if (token == NULL || !is_valid_domain(token)) {
                printf("Invalid domain name format. Please ensure all domain names follow the guidelines.\n");
                valid = 0;
                break;
            }
            strcpy(domain[i], token);
        }
    
        
        if (!valid)
            continue;

        query.num_queries = num_queries;
        for (int i = 0; i < num_queries; i++) {
            query.queries[i].size = strlen(domain[i]);
            strcpy(query.queries[i].domain, domain[i]);
        }

        unsigned char packet[sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(simDNS_QueryPacket)];
        memset(packet, 0, sizeof(packet));
        AppendEthernetHeader(packet);
        AppendIPHeader(packet);

        memcpy(packet + sizeof(struct ethhdr) + sizeof(struct iphdr), &query, sizeof(simDNS_QueryPacket));
        ssize_t packet_size = sizeof(packet);

        ssize_t bytes_sent = send(sockfd, packet, packet_size, 0);
        if (bytes_sent < 0) {
            perror("send");
            close(sockfd);
            free(packet);
            exit(EXIT_FAILURE);
        }

        printf("Query sent successfully.\n");

        //Receive the response packet
        unsigned char* response_packet = malloc(sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(simDNS_ResponsePacket));
        ssize_t bytes_received = recv(sockfd, response_packet, sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(simDNS_ResponsePacket), 0);
        if (bytes_received < 0) {
            perror("recv");
            close(sockfd);
            free(response_packet);
            exit(EXIT_FAILURE);
        }

        simDNS_ResponsePacket *response = (simDNS_ResponsePacket *)(response_packet + sizeof(struct ethhdr) + sizeof(struct iphdr));
        printf("Received DNS Response Packet:\n");
        printf("ID: %u\n", ntohs(response->id));
        printf("Message Type: %u\n", response->message_type);
        printf("Number of Queries: %u\n", response->num_queries);
        for (int i = 0; i < response->num_queries; i++) {
            if(response->queries[i].valid == 1)
                printf("IP Address for domain %s: %s\n", query.queries[i].domain, response->queries[i].ip);
            else
                printf("IP Address for domain %s: Not found\n", query.queries[i].domain);
        }

        close(sockfd);

    }
}
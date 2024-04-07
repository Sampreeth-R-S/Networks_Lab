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
void AppendEthernetHeader(unsigned char *packet)
{
    struct ethhdr *eth = (struct ethhdr *)packet;

    // Destination MAC address
    eth->h_dest[0] = 0x00;
    eth->h_dest[1] = 0x15;
    eth->h_dest[2] = 0x5d;
    eth->h_dest[3] = 0x4e;
    eth->h_dest[4] = 0x13;
    eth->h_dest[5] = 0x86;

    // Source MAC address
    eth->h_source[0] = 0x00;
    eth->h_source[1] = 0x15;
    eth->h_source[2] = 0x5d;
    eth->h_source[3] = 0x4e;
    eth->h_source[4] = 0x13;
    eth->h_source[5] = 0x86;

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
int get_interface_index(char interface_name[IFNAMSIZ], int sockfd)
{
        struct ifreq if_idx;
        memset(&if_idx, 0, sizeof(struct ifreq));
        strncpy(if_idx.ifr_name, interface_name, IFNAMSIZ-1);
        if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
                perror("SIOCGIFINDEX");

        return if_idx.ifr_ifindex;
}

int send_query(int sockfd, simDNS_QueryPacket *packet, size_t packet_size) {
    struct sockaddr_ll dest;
    char name[16];
    strcpy(name, "eth0");
    dest.sll_ifindex = get_interface_index(name, sockfd);
    dest.sll_halen = ETH_ALEN;
    char dest_addr[6]={0x00,0x15,0x5d,0x4e,0x13,0x86};
    memcpy(dest.sll_addr, dest_addr, ETH_ALEN);
    printf("attempting to send\n");
    ssize_t bytes_sent = sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)&dest, sizeof(dest));
    if (bytes_sent < 0) {
        perror("sendto");
        return -1;
    }
    return 0;
}

void AppendData(unsigned char *packet, simDNS_QueryPacket *query_packet) {
int offset = sizeof(struct ethhdr) + sizeof(struct iphdr);
    uint8_t *ptr = packet + offset;

    // Append query packet fields
    *(uint16_t *)ptr = htons(query_packet->id); // ID
    ptr += 2;
    *ptr = (query_packet->message_type & 0x01) << 7; // Message Type
    *ptr |= query_packet->num_queries & 0x07; // Number of queries
    ptr++;

    // Append query strings
    for (int i = 0; i < query_packet->num_queries; i++) {
        *ptr = query_packet->queries[i].size; // Size of domain name
        ptr++;
        memcpy(ptr, query_packet->queries[i].domain, query_packet->queries[i].size); // Domain name
        ptr += query_packet->queries[i].size;
    }

}

int main() {
    // Open raw socket
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_ll sll;
    struct ifreq ifr;

    bzero(&sll, sizeof(sll));
    bzero(&ifr, sizeof(ifr));

    strcpy((char *)ifr.ifr_name, "eth0");
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

    while (1) {
        // Read query string from user

        char *packet = (char *)malloc(65536);
        memset(packet, 0, 65536);
        AppendEthernetHeader(packet);
        AppendIPHeader(packet);

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
            if (token == NULL || !is_valid_domain(token)) {
                printf("Invalid domain name format. Please ensure all domain names follow the guidelines.\n");
                valid = 0;
                break;
            }
            strcpy(domain[i], token);
        }
    
        
        if (!valid)
            continue;
        
        printf("Query string parsed successfully.\n");

        // Construct query packet
        simDNS_QueryPacket query_packet;
        query_packet.id = rand() % 65536; // Generate random ID
        query_packet.message_type = 0; // Query
        query_packet.num_queries = num_queries;
        for (int i = 0; i < num_queries; i++) {
            query_packet.queries[i].size = strlen(domain[i]);
            strcpy(query_packet.queries[i].domain, domain[i]);
        }
        printf("Query packet constructed successfully.\n");
        // Print packet details before sending
        printf("\nPacket Details Before Sending:\n");
        printf("ID: %d\n", query_packet.id);
        printf("Message Type: %d\n", query_packet.message_type);
        printf("Number of Queries: %d\n", query_packet.num_queries);
        for (int i = 0; i < query_packet.num_queries; i++) {
            printf("Query %d: %s\n", i + 1, query_packet.queries[i].domain);
        }

        // Append data to the packet
        AppendData(packet, &query_packet);

        // Print packet details after appending data
        printf("\nPacket Details After Appending Data:\n");
        printf("Ethernet Header:\n");
        printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", packet[6], packet[7], packet[8], packet[9], packet[10], packet[11]);
        printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);
        printf("IP Header:\n");
        struct iphdr *ip = (struct iphdr *)(packet + sizeof(struct ethhdr));
        printf("Source IP: %s\n", inet_ntoa(*(struct in_addr *)&ip->saddr));
        printf("Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&ip->daddr));
        printf("ID: %d\n", ntohs(ip->id));
        printf("Total Length: %d\n", ntohs(ip->tot_len));
        printf("Protocol: %d\n", ip->protocol);
        printf("Checksum: %04x\n", ntohs(ip->check));
        printf("Queries:\n");
        uint8_t *ptr = (uint8_t *)packet + sizeof(struct ethhdr) + sizeof(struct iphdr) + 3; // Skip Ethernet and IP headers and move to the start of queries
        for (int i = 0; i < num_queries; i++) {
            uint8_t size = *ptr;
            ptr++;
            char domain[MAX_DOMAIN_SIZE];
            memcpy(domain, ptr, size);
            domain[size] = '\0';
            ptr += size;
            printf("Query %d: %s\n", i + 1, domain);
        }

        // Send query
        if (send_query(sockfd, &query_packet, sizeof(query_packet)) < 0) {
            fprintf(stderr, "Failed to send query\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Query sent successfully.\n");
    }

    close(sockfd);
    return 0;
}

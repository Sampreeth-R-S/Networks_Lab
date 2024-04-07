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
int get_interface_index(char interface_name[IFNAMSIZ], int sockfd)
{
        struct ifreq if_idx;
        memset(&if_idx, 0, sizeof(struct ifreq));
        strncpy(if_idx.ifr_name, interface_name, IFNAMSIZ-1);
        if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
                perror("SIOCGIFINDEX");

        return if_idx.ifr_ifindex;
}

int send_query(int sockfd, char *packet, size_t packet_size) {
    struct sockaddr_ll dest;
    char name[16];
    strcpy(name, "enp0s3");
    dest.sll_ifindex = get_interface_index(name, sockfd);
    dest.sll_halen = ETH_ALEN;
    char dest_addr[6]={0x08,0x00,0x27,0xe3,0x2a,0xa0};
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
    ptr++;
    *ptr = query_packet->num_queries ; // Number of queries
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
    

    while (1) {
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
        int pid = fork();
        if(pid<0)
        {
            perror("fork");
            exit(1);
        }
        if(pid){close(sockfd);continue;}
        close(sockfd);
        sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sockfd < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        // Set up server address structure
        //struct sockaddr_ll sll;
        //struct ifreq ifr;

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

        printf("Client is listening for Ethernet packets...\n");
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
        uint8_t *ptr = (uint8_t *)packet + sizeof(struct ethhdr) + sizeof(struct iphdr) + 4; // Skip Ethernet and IP headers and move to the start of queries
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
        if (send_query(sockfd, packet, 1000) < 0) {
            fprintf(stderr, "Failed to send query\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Query sent successfully.\n");
        time_t sendtime = time(NULL);
        struct timeval tm;
        tm.tv_sec = 1;
        tm.tv_usec = 0;
        while(1)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            tm.tv_sec = 1;
            tm.tv_usec = 0;
            int ret = select(sockfd + 1, &readfds, NULL, NULL, &tm);
            if (ret < 0)
            {
                perror("select");
                break;
            }
            int difference = time(NULL) - sendtime;
            //printf("Difference=%d\n",difference);
            fflush(stdout);
            if(difference>5)break;
            if (ret == 0)
            {
                //printf("Timeout\n");
                continue;
            }
            char* buffer = (char *)malloc(65536);
            memset(buffer, 0, 65536);
            ssize_t bytes_received = recv(sockfd, buffer, 65536, 0);
            if (bytes_received < 0) {
                perror("recv");
                continue;
            }
            struct ethhdr *eth_header = (struct ethhdr *)buffer;
            if ((unsigned int)ntohs(eth_header->h_proto) != ETH_P_IP)
            {
                continue;
            }
            struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
            if((unsigned int)ip_header->protocol !=254)continue;

            if(!(inet_addr("127.0.0.1")==ip_header->saddr)){printf("Continuing\n");continue;}
            int offset = sizeof(struct ethhdr) + sizeof(struct iphdr);
            uint8_t *ptr_resp = buffer + offset;

            // Extract ID
            uint16_t id_resp = ntohs(*(uint16_t *)ptr_resp);
            printf("ID: %d\n", id_resp);
            ptr_resp += 2;

            // Extract message type and number of queries
            uint8_t type_and_queries = *ptr_resp;
            uint8_t message_type = (type_and_queries >> 7) & 0x01;
            ptr_resp++;
            uint8_t num_resp = *ptr_resp;
            printf("Message Type: %s\n", message_type ? "Response" : "Query");
            printf("Number of Queries: %d\n", num_resp);
            ptr_resp++;
            // simDNS_ResponsePacket *response_packet = (simDNS_ResponsePacket *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));
            // printf("%d,%d\n",response_packet->id,query_packet.id);
            // if(response_packet->id!=query_packet.id)continue;
            printf("Response received for query %d\n",id_resp);
            for(int i=0;i<num_resp;i++)
            {
                uint8_t is_valid= *ptr_resp;
                ptr_resp++;

                char resp_ip[4]; // Extra byte for null-terminator
                memcpy(resp_ip, ptr_resp, 4);
                ptr_resp += 4;
                
                if(is_valid)
                {
                    //Use inet_ntoa to convert the IP address to a string
                    // printf("IP is %s\n",inet_ntoa(*(struct in_addr *)&resp_ip));
                    printf("IP for %s is %s\n",query_packet.queries[i].domain,inet_ntoa(*(struct in_addr *)&resp_ip));
                }
                else
                {
                    printf("No IP found for %s\n",query_packet.queries[i].domain);
                }
            }
            exit(0);

        }
        printf("No response received for query %d\n",query_packet.id);
        printf("Resending\n");
        if (send_query(sockfd, packet, 1000) < 0) {
            fprintf(stderr, "Failed to send query\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Query sent successfully.\n");
        sendtime = time(NULL);
        while(1)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            tm.tv_sec = 1;
            tm.tv_usec = 0;
            int ret = select(sockfd + 1, &readfds, NULL, NULL, &tm);
            if (ret < 0)
            {
                perror("select");
                break;
            }
            int difference = time(NULL) - sendtime;
            //printf("Difference=%d\n",difference);
            fflush(stdout);
            if(difference>5)break;
            if (ret == 0)
            {
                //printf("Timeout\n");
                continue;
            }
            char* buffer = (char *)malloc(65536);
            memset(buffer, 0, 65536);
            ssize_t bytes_received = recv(sockfd, buffer, 65536, 0);
            if (bytes_received < 0) {
                perror("recv");
                continue;
            }
            struct ethhdr *eth_header = (struct ethhdr *)buffer;
            if ((unsigned int)ntohs(eth_header->h_proto) != ETH_P_IP)
            {
                continue;
            }
            struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
            if((unsigned int)ip_header->protocol !=254)continue;
            if(strcmp("127.0.0.1",inet_ntoa(*(struct in_addr *)&ip_header->saddr))!=0)continue;
            simDNS_ResponsePacket *response_packet = (simDNS_ResponsePacket *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));
            if(response_packet->id!=query_packet.id)continue;
            printf("Response received for query %d\n",response_packet->id);
            for(int i=0;i<response_packet->num_queries;i++)
            {
                if(response_packet->queries[i].valid)
                {
                    printf("IP for %s is %s\n",query_packet.queries[i].domain,response_packet->queries[i].ip);
                }
                else
                {
                    printf("No IP found for %s\n",query_packet.queries[i].domain);
                }
            }
            exit(0);

        }
        printf("No response received for query %d, Retrying third time\n",query_packet.id);
        printf("Query sent successfully.\n");
        sendtime = time(NULL);
        while(1)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            tm.tv_sec = 1;
            tm.tv_usec = 0;
            int ret = select(sockfd + 1, &readfds, NULL, NULL, &tm);
            if (ret < 0)
            {
                perror("select");
                break;
            }
            int difference = time(NULL) - sendtime;
            //printf("Difference=%d\n",difference);
            fflush(stdout);
            if(difference>5)break;
            if (ret == 0)
            {
                //printf("Timeout\n");
                continue;
            }
            char* buffer = (char *)malloc(65536);
            memset(buffer, 0, 65536);
            ssize_t bytes_received = recv(sockfd, buffer, 65536, 0);
            if (bytes_received < 0) {
                perror("recv");
                continue;
            }
            struct ethhdr *eth_header = (struct ethhdr *)buffer;
            if ((unsigned int)ntohs(eth_header->h_proto) != ETH_P_IP)
            {
                continue;
            }
            struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
            if((unsigned int)ip_header->protocol !=254)continue;
            if(strcmp("127.0.0.1",inet_ntoa(*(struct in_addr *)&ip_header->saddr))!=0)continue;
            simDNS_ResponsePacket *response_packet = (simDNS_ResponsePacket *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));
            if(response_packet->id!=query_packet.id)continue;
            printf("Response received for query %d\n",response_packet->id);
            for(int i=0;i<response_packet->num_queries;i++)
            {
                if(response_packet->queries[i].valid)
                {
                    printf("IP for %s is %s\n",query_packet.queries[i].domain,response_packet->queries[i].ip);
                }
                else
                {
                    printf("No IP found for %s\n",query_packet.queries[i].domain);
                }
            }
            exit(0);

        }
        printf("Tried 3 times for query %d\n",query_packet.id);
        printf("Giving up\n");
        exit(0);

    }

    //close(sockfd);
    return 0;
}
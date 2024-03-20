#include <stdio.h>
#include <sys/ipc.h>
#include <sys/select.h>
#include <sys/shm.h>	
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "msocket.h"
#include <error.h>
#include <errno.h>

int main()
{
    int sockfd = m_socket(MTP_SOCKET, SOCK_MTP, 0);
    int temp = m_bind(sockfd,"127.0.0.1",8000,"127.0.0.1",8002);
    char buffer[1024];
    strcpy(buffer,"Hello");
    struct sockaddr_in cliaddr;
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(8002);
    inet_pton(AF_INET,"127.0.0.1",&cliaddr.sin_addr);
    for(int i=0;i<10;i++)
    {
        char buffer[1024];
        sprintf(buffer,"Frame %d",i);
        int temp=m_sendto(sockfd,buffer,strlen(buffer)+2,0,cliaddr,sizeof(cliaddr));
    }
    sleep(1000);
    for(int i=0;i<10;i++)
    {
        char buffer[1024];
        sprintf(buffer,"Frame %d",i);
        int temp=m_sendto(sockfd,buffer,strlen(buffer)+2,0,cliaddr,sizeof(cliaddr));
    }
    sleep(1000);
    m_close(sockfd);
    exit(0);
}
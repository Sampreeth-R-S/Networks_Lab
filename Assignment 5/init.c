#include <stdio.h>
#include <sys/ipc.h>
#include <sys/select.h>
#include <sys/shm.h>	
#include<pthread.h>
#include<time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define T 5
struct sndwnd{
    int start,mid,end,lastwritten;
};
struct rcvwnd{
    int next_expected,next_supplied;
};
struct sh{
    int free;
    int pid;
    int sockfd;
    int mtpfd;
    char sender_ip[16];
    int sender_port;
    char receiver_ip[16];
    int receiver_port;
    char sendbuf[10][1024];
    char recvbuf[5][1024];
    struct sndwnd sendwindow;
    struct rcvwnd receivewindow;
    struct tm* timers[10];
    int timerset[10];
    int next_write;
    int send_isfree[10];
    int recv_isfree[5];
    int is_empty;
};

struct sh* shm;
pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;
void* R(void* arg)
{
    while(1)
    {
        fd_set readfds;
        int arr[25];
        for(int i=0;i<25;i++)
        {
            arr[i]=-1;
        }
        int maxfd=-1;
        int cur=0;
        pthread_mutex_lock(&shm_mutex);
        for(int i=0;i<25;i++)
        {
            if(!shm[i].free)
            {
                FD_SET(shm[i].sockfd,&readfds);
                if(shm[i].sockfd>maxfd)
                {
                    maxfd = i;
                }
                arr[cur++]=shm[i].sockfd;
            }
        }
        pthread_mutex_unlock(&shm_mutex);
        if(maxfd==-1)
        {
            sleep(1);
            continue;
        }
        struct timeval timeout;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        int ret = select(maxfd+1,&readfds,NULL,NULL,&timeout);
        if(ret==0)
        {
            continue;
        }
        pthread_mutex_lock(&shm_mutex);
        for(int i=0;i<cur;i++)
        {
            if(FD_ISSET(shm[arr[i]].sockfd,&readfds))
            {
                char buffer[1024];
                struct sockaddr_in cliaddr;
                int clilen=sizeof(cliaddr);
                int n = recvfrom(shm[arr[i]].sockfd,buffer,1024,0,(struct sockaddr *)&cliaddr,&clilen);
                if(n==0)
                {
                    continue;
                }
                if(n<0)
                {
                    perror("recvfrom");
                    continue;
                }
                int temp = buffer[0];
                if(temp&1)
                {
                    char receiver_ip[16];
                    inet_ntop(cliaddr.sin_family,&cliaddr.sin_addr,receiver_ip,16);
                    int receiver_port = ntohs(cliaddr.sin_port);
                    if(strcmp(receiver_ip,shm[arr[i]].receiver_ip)!=0)
                    {
                        continue;
                    }
                    if(receiver_port!=shm[arr[i]].receiver_port)
                    {
                        continue;
                    }
                    int sequence_number = 0;
                    for(int i=4;i>=1;i--)
                    {
                        sequence_number=sequence_number*2+((temp>>i)&1);
                    }
                    int index = arr[i];
                    if(sequence_number>shm[index].sendwindow.start)
                    {
                        if(sequence_number-shm[index].sendwindow.start<5)
                        {
                            shm[index].sendwindow.start = (sequence_number+1)%16;
                        }
                    }
                    else 
                    {
                        if(sequence_number+15-shm[index].sendwindow.start<5)
                        {
                            shm[index].sendwindow.start = (sequence_number+1)%16;
                        }
                    }
                    if((temp>>5)&1)
                    {
                        shm[index].sendwindow.end = shm[index].sendwindow.start;
                    }
                    else
                    {
                        int emptyspace = buffer[1];
                        shm[index].sendwindow.end = (shm[index].sendwindow.start+emptyspace)%16;
                    }
                    continue;
                }
                char receiver_ip[16];
                inet_ntop(cliaddr.sin_family,&cliaddr.sin_addr,receiver_ip,16);
                int receiver_port = ntohs(cliaddr.sin_port);
                if(strcmp(receiver_ip,shm[arr[i]].receiver_ip)!=0)
                {
                    continue;
                }
                if(receiver_port!=shm[arr[i]].receiver_port)
                {
                    continue;
                }
                else 
                {
                    int sequence_number = 0;
                    for(int i=4;i>=1;i--)
                    {
                        sequence_number=sequence_number*2+((temp>>i)&1);
                    }
                    if(sequence_number==shm[arr[i]].receivewindow.next_expected)
                    {
                        //for(int i=1;i<1024;i++)
                        {
                            int index = arr[i];
                            for(int i=0;i<5;i++)
                            {
                                if(shm[index].recv_isfree[i])
                                {
                                    strcpy(shm[index].recvbuf[i],buffer+1);
                                    shm[index].recv_isfree[i]=0;
                                    break;
                                }
                            }
                            shm[arr[i]].receivewindow.next_expected++;
                            shm[arr[i]].receivewindow.next_expected%=16;
                            char sendbuf[1024];
                            for(int i=0;i<1024;i++)sendbuf[i]=0;
                            int bitmask;
                            bitmask+=1;
                            for(int i=1;i<5;i++)
                            {
                                if((sequence_number>>i)&1)
                                {
                                    bitmask+=1<<i;
                                }
                            }
                            // if(shm[arr[i]].receivewindow.next_expected==shm[arr[i]].receivewindow.next_supplied)
                            // {
                            //     bitmask+=1<<5;
                            // }
                            // sendbuf[0]=bitmask;
                            // int n=sendto(shm[arr[i]].sockfd,sendbuf,1024,0,(struct sockaddr *)&cliaddr,clilen);
                            // if(n<0)
                            // {
                            //     perror("sendto");
                            // }
                            int full = 0;
                            int index = arr[i];
                            if(shm[index].receivewindow.next_expected>=shm[index].receivewindow.next_supplied)
                            {
                                if(shm[index].receivewindow.next_expected-shm[index].receivewindow.next_supplied>=5)
                                {
                                    full = 1;
                                }
                            }
                            else
                            {
                                if(shm[index].receivewindow.next_expected+16-shm[index].receivewindow.next_supplied>=5)
                                {
                                    full = 1;
                                }
                            }
                            if(full)
                            {
                                bitmask+=1<<5;
                            }
                            sendbuf[0]=bitmask;
                            int emptyspace = 0;
                            if(shm[index].receivewindow.next_expected<shm[index].receivewindow.next_supplied)
                            {
                                emptyspace = 5-(shm[index].receivewindow.next_expected+16-shm[index].receivewindow.next_supplied);
                            }
                            else
                            {
                                emptyspace = 5-(shm[index].receivewindow.next_expected-shm[index].receivewindow.next_supplied);
                            }
                            sendbuf[1]=emptyspace;
                            int n=sendto(shm[index].sockfd,sendbuf,strlen(sendbuf),0,(struct sockaddr *)&cliaddr,clilen);
                            if(n<0)
                            {
                                perror("sendto");
                            }
                        }
                    }
                }

            }
        }
        for(int i=0;i<25;i++)
        {
            if(shm[i].free)
            {
                continue;
            }
            if(shm[i].is_empty)
            {
                int emptyspace=0;
                int index=i;
                if(shm[index].receivewindow.next_expected<shm[index].receivewindow.next_supplied)
                {
                    emptyspace = 5-(shm[index].receivewindow.next_expected+16-shm[index].receivewindow.next_supplied);
                }
                else
                {
                    emptyspace = 5-(shm[index].receivewindow.next_expected-shm[index].receivewindow.next_supplied);
                }
                if(emptyspace>0)
                {
                    char sendbuf[1024];
                    for(int i=0;i<1024;i++)sendbuf[i]=0;
                    sendbuf[0]+=1;
                    sendbuf[1]=emptyspace;
                    int lastreceived = (shm[i].receivewindow.next_expected-1+16)%16;
                    for(int j=0;j<4;j++)
                    {
                        if((lastreceived>>j)&1)
                        {
                            sendbuf[0]+=1<<(j+1);
                        }
                    }
                    struct sockaddr_in cliaddr;
                    int clilen=sizeof(cliaddr);
                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_port = htons(shm[i].receiver_port);
                    inet_pton(AF_INET,shm[i].receiver_ip,&cliaddr.sin_addr);
                    int n=sendto(shm[i].sockfd,sendbuf,strlen(sendbuf),0,(struct sockaddr *)&cliaddr,clilen);
                    if(n<0)
                    {
                        perror("sendto");
                    }
                    shm[i].is_empty=0;
                }
            }
        }
        pthread_mutex_unlock(&shm_mutex);


    }
}

void* S(void* arg)
{
    while(1)
    {
        pthread_mutex_lock(&shm_mutex);
        for(int i=0;i<25;i++)
        {
            if(!shm[i].free)
            {

            }
        }
        pthread_mutex_unlock(&shm_mutex);
    }
}
int main()
{
    key_t key = ftok("init.c",64);
    int shmid = shmget(key, sizeof(struct sh)*30, 0777|IPC_CREAT);
    shm = (struct sh*)shmat(shmid, (void*)0, 0);
    for(int i=0;i<30;i++)
    {
        shm[i].free = 1;
    }
    pthread_t t1,t2;
    pthread_create(&t1,NULL,R,NULL);
    pthread_create(&t2,NULL,S,NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    return 0;


}
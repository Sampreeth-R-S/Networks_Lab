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
#include <error.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <signal.h>
#ifdef DEBUG
    #define myprintf printf
#else
    #define myprintf //
#endif
#define T 5
const float P = 0.8;
#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
				   the P(s) operation */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
				   the V(s) operation */
int retransmissions=0;

struct sockinfo{
    int sock_id;
    char ip[16];
    int port;
    int err_no;
};

struct sndwnd{
    int start,mid,end,next_seq_no;
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
    struct tm timers[16];
    //int timerset[10];
    int next_write;
    int send_isfree[10];
    int recv_isfree[5];
    //int seq_no_is_available[16];
    int is_empty;
    int marked_deletion;
    int is_bound;
};

struct sh* shm;
pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;
int droppacket()
{
    
    float random = (float)rand()/(float)RAND_MAX;
    myprintf("Random value generated %f\n",random);
    if(random<P)
    {
        return 1;
    }
    return 0;
}
void *G(void* arg)
{
    key_t key1 = ftok("init.c",64);
    int mutex = semget(key1, 1, 0666|IPC_CREAT);
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
    while(1)
    {
        P(mutex);
        for(int i=0;i<25;i++)
        {
            if(!shm[i].free)
            {
                int pid = shm[i].pid;
                if(kill(pid,0)==0)
                {
                    if(shm[i].marked_deletion)
                    {
                        myprintf("Socket at index %d marked for deletion\n",i);
                        shm[i].free=1;
                        close(shm[i].sockfd);
                    }
                    continue;
                }
                myprintf("Process with pid %d dead, freeing the socket\n",pid);
                shm[i].free=1;
                close(shm[i].sockfd);
            }
        }
        V(mutex);
        sleep(60);
    }
    
}
void* R(void* arg)
{
    key_t key1 = ftok("init.c",64);
    int mutex = semget(key1, 1, 0666|IPC_CREAT);
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
    while(1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        int arr[25];
        for(int i=0;i<25;i++)
        {
            arr[i]=-1;
        }
        int maxfd=-1;
        int cur=0;
        P(mutex);
        for(int i=0;i<25;i++)
        {
            if(!shm[i].free)
            {
                FD_SET(shm[i].sockfd,&readfds);
                if(shm[i].sockfd>maxfd)
                {
                    maxfd = shm[i].sockfd;
                }
                arr[cur++]=i;
            }
        }
        V(mutex);
        myprintf("cur:%d\n",cur);
        if(maxfd==-1)
        {
            sleep(1);
            continue;
        }
        struct timeval timeout;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        myprintf("Going to wait on select, maxfd=%d\n",maxfd);
        int ret = select(maxfd+1,&readfds,NULL,NULL,&timeout);
        myprintf("Returned from select\n");
        if(ret==0)
        {
            myprintf("Returned from select with timeout\n");
            P(mutex);
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
                        sendbuf[0]+=1<<6;
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
                        myprintf("Sent ack to %s:%d to clear emptyspace\n",shm[i].receiver_ip,shm[i].receiver_port);
                        shm[i].is_empty=0;
                    }
                }
            }
            V(mutex);
            continue;
        }
        P(mutex);
        for(int i=0;i<cur;i++)
        {
            myprintf("arr[%d]=%d\n",i,arr[i]);
            if(FD_ISSET(shm[arr[i]].sockfd,&readfds))
            {
                if(shm[arr[i]].free)
                {
                    continue;
                }
                char buffer[1024];
                struct sockaddr_in cliaddr;
                int clilen=sizeof(cliaddr);
                myprintf("Calling recvfrom,shm[arr[i]].sockfd=%d\n",shm[arr[i]].sockfd);
                int n = recvfrom(shm[arr[i]].sockfd,buffer,1024,0,(struct sockaddr *)&cliaddr,&clilen);
                if(n<0)
                {
                    perror("recvfrom:");
                    continue;
                }
                myprintf("Received packet from %s:%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
                if(droppacket())
                {
                    myprintf("Dropping packet\n");
                    continue;
                }
                if(n==0)
                {
                    printf("n=%d\n",n);
                    continue;
                }
                if(n<0)
                {
                    perror("recvfrom");
                    continue;
                }
                int temp = buffer[0];
                if((temp>>7)&1)
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
                        sendbuf[0]+=1<<6;
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
                        myprintf("Sent ack to %s:%d to clear empty space\n",shm[i].receiver_ip,shm[i].receiver_port);
                    }
                    continue;
                }
                if(temp&1)
                {
                    myprintf("ACK frame\n");
                    char receiver_ip[16];
                    inet_ntop(cliaddr.sin_family,&cliaddr.sin_addr,receiver_ip,16);
                    int receiver_port = ntohs(cliaddr.sin_port);
                    if(strcmp(receiver_ip,shm[arr[i]].receiver_ip)!=0)
                    {
                        myprintf("Received ACK from client not bound to me, discarding\n");
                        continue;
                    }
                    if(receiver_port!=shm[arr[i]].receiver_port)
                    {
                        myprintf("Received ACK from port not bound to me, discarding\n");
                        continue;
                    }
                    int sequence_number = 0;
                    for(int i=4;i>=1;i--)
                    {
                        sequence_number=sequence_number*2+((temp>>i)&1);
                    }
                    myprintf("ACK sequence number: %d\n",sequence_number);
                    int index = arr[i];
                    int new_ack=0;
                    if(sequence_number>=shm[index].sendwindow.start)
                    {
                        new_ack=1;
                        if(sequence_number-shm[index].sendwindow.start<5)
                        {
                            for(int j=shm[index].sendwindow.start;j!=(sequence_number+1)%16;j=(j+1)%16)
                            {
                                int found = 0;
                                for(int k=0;k<10;k++)
                                {
                                    if(!shm[index].send_isfree[k])
                                    {
                                        int temp_sequence_number=0;
                                        for(int l=4;l>=1;l--)
                                        {
                                            temp_sequence_number=temp_sequence_number*2+((shm[index].sendbuf[k][0]>>l)&1);
                                        }
                                        if(temp_sequence_number==j)
                                        {
                                            found = 1;
                                            shm[index].send_isfree[k] = 1;
                                            myprintf("Freeing space at index %d from sendbuf with frame %d\n",k,temp_sequence_number);
                                            break;
                                        }
                                    }
                                }
                            }
                            shm[index].sendwindow.start = (sequence_number+1)%16;
                        }
                    }
                    else 
                    {
                        new_ack=1;
                        if(sequence_number+16-shm[index].sendwindow.start<5)
                        {
                            for(int j=shm[index].sendwindow.start;j!=(sequence_number+1)%16;j=(j+1)%16)
                            {
                                int found = 0;
                                for(int k=0;k<10;k++)
                                {
                                    if(!shm[index].send_isfree[k])
                                    {
                                        int temp_sequence_number=0;
                                        for(int l=4;l>=1;l--)
                                        {
                                            temp_sequence_number=temp_sequence_number*2+((shm[index].sendbuf[k][0]>>l)&1);
                                        }
                                        if(temp_sequence_number==j)
                                        {
                                            found = 1;
                                            shm[index].send_isfree[k]=1;
                                            myprintf("Freeing space at index %d from sendbuf with frame %d\n",k,temp_sequence_number);
                                            break;
                                        }
                                    }
                                }
                            }
                            shm[index].sendwindow.start = (sequence_number+1)%16;
                        }
                    }
                    if(!new_ack)
                    {
                        continue;
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
                    myprintf("Empty space set as %d,start = %d, end = %d\n",buffer[1],shm[index].sendwindow.start,shm[index].sendwindow.end);
                    continue;
                }
                myprintf("Data frame\n");
                char receiver_ip[16];
                inet_ntop(cliaddr.sin_family,&cliaddr.sin_addr,receiver_ip,16);
                int receiver_port = ntohs(cliaddr.sin_port);
                if(strcmp(receiver_ip,shm[arr[i]].receiver_ip)!=0)
                {
                    myprintf("Not my receiver\n");
                    myprintf("%s,%s\n",receiver_ip,shm[arr[i]].receiver_ip);
                    continue;
                }
                if(receiver_port!=shm[arr[i]].receiver_port)
                {
                    myprintf("Not my port\n");
                    myprintf("%d,%d\n",receiver_port,shm[arr[i]].receiver_port);
                    continue;
                }
                myprintf("Processing ack frame\n");
                {
                    int sequence_number = 0;
                    for(int i=4;i>=1;i--)
                    {
                        sequence_number=sequence_number*2+((temp>>i)&1);
                    }
                    myprintf("Sequence Number of data frame received: %d\n",sequence_number);
                    if(sequence_number==shm[arr[i]].receivewindow.next_expected)
                    {
                        //for(int i=1;i<1024;i++)
                        myprintf("Creating ack frame\n");
                        {
                            int index = arr[i];
                            for(int i=0;i<5;i++)
                            {
                                if(shm[index].recv_isfree[i])
                                {
                                    strcpy(shm[index].recvbuf[i],buffer);
                                    for(int j=0;j<strlen(shm[index].recvbuf[i]);j++)
                                    {
                                        myprintf("%c",shm[index].recvbuf[i][j]);
                                    }
                                    myprintf("\n");
                                    shm[index].recv_isfree[i]=0;
                                    break;
                                }
                            }
                            // shm[arr[i]].receivewindow.next_expected++;
                            // shm[arr[i]].receivewindow.next_expected%=16;
                            while(1)
                            {
                                int found=0;
                                for(int j=0;j<5;j++)
                                {
                                    int temp_seq_number=0;
                                    if(!shm[index].recv_isfree[j])
                                    {
                                        for(int k=4;k>=1;k--)
                                        {
                                            temp_seq_number=temp_seq_number*2+((shm[index].recvbuf[j][0]>>k)&1);
                                        }
                                        if(temp_seq_number==shm[index].receivewindow.next_expected)
                                        {
                                            found=1;
                                            shm[index].receivewindow.next_expected++;
                                            shm[index].receivewindow.next_expected%=16;
                                            break;
                                        }
                                    }
                                }
                                if(!found)
                                {
                                    break;
                                }
                            }
                            while(sequence_number!=shm[index].receivewindow.next_expected)
                            {
                                char sendbuf[1024];
                                for(int j=0;j<1024;j++)sendbuf[j]=0;
                                int bitmask=0;
                                index = arr[i];
                                bitmask += 1;
                                for(int j=0;j<5;j++)
                                {
                                    if((sequence_number>>j)&1)
                                    {
                                        bitmask+=1<<(j+1);
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
                                index = arr[i];
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
                                bitmask += 1<<6;
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
                                if(emptyspace==0)
                                {
                                    shm[index].is_empty=1;
                                }
                                int n=sendto(shm[index].sockfd,sendbuf,strlen(sendbuf)+2,0,(struct sockaddr *)&cliaddr,clilen);
                                if(n<0)
                                {
                                    perror("sendto");
                                }
                                myprintf("Sent ack to %s:%d\n",shm[index].receiver_ip,shm[index].receiver_port);
                                myprintf("Buffer[0]=%d,buffer[1]=%d\n",sendbuf[0],sendbuf[1]);
                                sequence_number++;
                                sequence_number%=16;
                            }
                        }
                    }
                    else
                    {
                        myprintf("Detected out of order frame, sending ACK again\n");
                        int bitmask = 1;
                        int index = arr[i];
                        int previous_received = (shm[index].receivewindow.next_expected-1+16)%16;
                        for(int j=0;j<4;j++)
                        {
                            if((previous_received>>j)&1)
                            {
                                bitmask+=1<<(j+1);
                            }
                        }
                        int full = 0;
                        index = arr[i];
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
                        bitmask += 1<<6;
                        char sendbuf[1024];
                        for(int j=0;j<1024;j++)sendbuf[j]=0;
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
                        if(emptyspace==0)
                        {
                            shm[index].is_empty=1;
                        }
                        int n=sendto(shm[index].sockfd,sendbuf,strlen(sendbuf)+2,0,(struct sockaddr *)&cliaddr,clilen);
                        if(n<0)
                        {
                            perror("sendto");
                        }
                        myprintf("Sent ack to %s:%d with sequence number %d\n",shm[index].receiver_ip,shm[index].receiver_port,previous_received);
                        previous_received = shm[index].receivewindow.next_expected;
                        int found=0;
                        for(int l=0;l<5;l++)
                        {
                            if(!shm[index].recv_isfree[l])
                            {
                                int temp_sequence = 0;
                                for(int m=4;m>=1;m--)
                                {
                                    if((shm[index].recvbuf[l][0]>>m)&1)
                                    {
                                        temp_sequence+=1<<(m-1);
                                    }
                                }
                                printf("Found frame %d at index %d, sequence number=%d\n",temp_sequence,l,sequence_number);
                                if(temp_sequence==sequence_number)
                                {
                                    found=1;
                                    break;
                                }
                            }
                        }
                        while(emptyspace&&!found)
                        {
                            emptyspace--;
                            if(previous_received==sequence_number)
                            {
                                myprintf("Storing out of order frame\n");
                                for(int k=0;k<5;k++)
                                {
                                    if(shm[index].recv_isfree[k])
                                    {
                                        strcpy(shm[index].recvbuf[k],buffer);
                                        shm[index].recv_isfree[k]=0;
                                        found=1;
                                        break;
                                        // strcpy(shm[index].recvbuf[i],buffer);
                                        // for(int j=0;j<strlen(shm[index].recvbuf[i]);j++)
                                        // {
                                        //     myprintf("%c",shm[index].recvbuf[i][j]);
                                        // }
                                        // myprintf("\n");
                                        // shm[index].recv_isfree[i]=0;
                                        // break;
                                    }
                                }
                            }
                            previous_received++;
                            previous_received%=16;

                        }
                        if(!found)
                        {
                            myprintf("Discarding out of order frame\n");
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
                    sendbuf[0]+=1<<6;
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
                    myprintf("Sent ack to %s:%d to clear empty space\n",shm[i].receiver_ip,shm[i].receiver_port);
                }
            }
        }
        V(mutex);


    }
}

void* S(void* arg)
{
    key_t key1 = ftok("init.c",64);
    int mutex = semget(key1, 1, 0666|IPC_CREAT);
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
    while(1)
    {
        P(mutex);
        for(int i=0;i<25;i++)
        {
            if(!shm[i].free)
            {
                time_t currentTime;
                time(&currentTime);
                myprintf("i=%d, start=%d,address = %d\n",i,shm[i].sendwindow.start,shm[i].timers[shm[i].sendwindow.start].tm_hour);
                double time_difference = difftime(currentTime,mktime(&shm[i].timers[shm[i].sendwindow.start]));
                if(time_difference>=T)
                {
                    myprintf("Timeout detected\n");
                    for(int j=shm[i].sendwindow.start;j!=shm[i].sendwindow.mid;j=(j+1)%16)
                    {
                        int found = 0;
                        for(int k=0;k<10;k++)
                        {
                            if(!shm[i].send_isfree[k])
                            {
                                int sequence_number=0;
                                for(int l=4;l>=1;l--)
                                {
                                    sequence_number=sequence_number*2+((shm[i].sendbuf[k][0]>>l)&1);
                                }
                                if(sequence_number==j)
                                {
                                    found = 1;
                                    struct sockaddr_in cliaddr;
                                    int clilen=sizeof(cliaddr);
                                    cliaddr.sin_family = AF_INET;
                                    cliaddr.sin_port = htons(shm[i].receiver_port);
                                    inet_pton(AF_INET,shm[i].receiver_ip,&cliaddr.sin_addr);
                                    int n=sendto(shm[i].sockfd,shm[i].sendbuf[k],strlen(shm[i].sendbuf[k]),0,(struct sockaddr *)&cliaddr,clilen);
                                    if(n<0)
                                    {
                                        perror("sendto");
                                    }
                                    time_t t = time(NULL);
                                    shm[i].timers[j] = *(localtime(&t));
                                    myprintf("Sent packet %d at time %d hours,%d seconds due to timeout from %s:%d\n",j,shm[i].timers[j].tm_hour,shm[i].timers[j].tm_sec,shm[i].sender_ip,shm[i].sender_port);
                                    //shm[i].send_isfree[k]=1;
                                    break;
                                }
                            }
                        }
                        if(!found)
                        {
                            break;
                        }
                    }
                }
            }
        }
        for(int i=0;i<25;i++)
        {
            if(!shm[i].free)
            {
                while(shm[i].sendwindow.mid!=shm[i].sendwindow.end)
                {
                    int found = 0;
                    for(int j=0;j<10;j++)
                    {
                        if(!shm[i].send_isfree[j])
                        {
                            int sequence_number=0;

                            for(int k=4;k>=1;k--)
                            {
                               sequence_number=sequence_number*2+((shm[i].sendbuf[j][0]>>k)&1);
                            }
                            myprintf("Found frame %d at index %d\n",sequence_number,j);
                            if(sequence_number==shm[i].sendwindow.mid)
                            {
                                found = 1;
                                struct sockaddr_in cliaddr;
                                int clilen=sizeof(cliaddr);
                                cliaddr.sin_family = AF_INET;
                                cliaddr.sin_port = htons(shm[i].receiver_port);
                                inet_pton(AF_INET,shm[i].receiver_ip,&cliaddr.sin_addr);
                                int n=sendto(shm[i].sockfd,shm[i].sendbuf[j],strlen(shm[i].sendbuf[j]),0,(struct sockaddr *)&cliaddr,clilen);
                                if(n<0)
                                {
                                    perror("sendto");
                                }
                                time_t t = time(NULL);
                                shm[i].timers[shm[i].sendwindow.mid] = *(localtime(&t));
                                myprintf("Sent packet %d at time %d hours,%d seconds from %s:%d\n",shm[i].sendwindow.mid,shm[i].timers[shm[i].sendwindow.mid].tm_hour,shm[i].timers[shm[i].sendwindow.mid].tm_sec,shm[i].sender_ip,shm[i].sender_port);
                                //shm[i].send_isfree[j]=1;
                                break;
                            }
                        }
                    }
                    if(!found)
                    {
                        break;
                    }
                    shm[i].sendwindow.mid++;
                    shm[i].sendwindow.mid%=16;
                }
            }
        }
        for(int i=0;i<25;i++)
        {
            if(!shm[i].free)
            {
                if(shm[i].sendwindow.start==shm[i].sendwindow.end)
                {
                    char sendbuf[10];
                    for(int i=0;i<10;i++)
                    {
                        sendbuf[i]=0;
                    }
                    sendbuf[0]=(1<<7);
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
                    myprintf("Sent special frame to check for emptyspace for sender %d\n",i);
                }
            }
        }
        V(mutex);
        sleep((T)/2);
    }
}
int main()
{
    key_t key = ftok("init.c",64);
    int shmid = shmget(key, sizeof(struct sh)*30, 0777|IPC_CREAT);
    key_t key1 = ftok("init.c",64);
    key_t key2 = ftok("init.c",65);
    key_t key3 = ftok("init.c",66);
    int shmid2 = shmget(key2, sizeof(struct sockinfo), 0777|IPC_CREAT);
    struct sockinfo* SOCK_INFO = (struct sockinfo*)shmat(shmid2, (void*)0, 0);
    int mutex = semget(key1, 1, 0666|IPC_CREAT);
    int main_wait = semget(key2, 1, 0666|IPC_CREAT);
    int func_wait = semget(key3, 1, 0666|IPC_CREAT);
    semctl(mutex, 0, SETVAL, 1);
    semctl(main_wait,0,SETVAL,0);
    semctl(func_wait,0,SETVAL,0);
    myprintf("mutex = %d\n",mutex);
    myprintf("main_wait = %d\n",main_wait);
    myprintf("func_wait = %d\n",func_wait);
    shm = (struct sh*)shmat(shmid, (void*)0, 0);
    myprintf("%ld\n",(long int)shm);
    for(int i=0;i<25;i++)
    {
        shm[i].free = 1;
    }
    pthread_t t1,t2,t3;
    pthread_create(&t1,NULL,R,NULL);
    pthread_create(&t2,NULL,S,NULL);
    pthread_create(&t3,NULL,G,NULL);
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
    while(1)
    {
        P(main_wait);
        P(mutex);
            if(SOCK_INFO->sock_id==0)
            {
                int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                if(sockfd<0)
                {
                    SOCK_INFO->err_no = errno;
                    perror("Socket:");
                }
                else
                    SOCK_INFO->sock_id = sockfd;
                myprintf("Socket created\n");
            }
            else
            {
                struct sockaddr_in servaddr;
                servaddr.sin_family = AF_INET;
                inet_pton(AF_INET,SOCK_INFO->ip,&servaddr.sin_addr);
                servaddr.sin_port = htons(SOCK_INFO->port);
                int bindret = bind(SOCK_INFO->sock_id,(struct sockaddr *)&servaddr,sizeof(servaddr));
                if(bindret<0)
                {
                    SOCK_INFO->err_no = errno;
                    myprintf("Bind failed\n");
                    perror("");
                }
                else
                {
                    SOCK_INFO->err_no = 0;
                }
                myprintf("Bind complete\n");
            }
        V(mutex);
        V(func_wait);

    }
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    return 0;
    

}
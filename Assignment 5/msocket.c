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
#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
				   the P(s) operation */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
				   the V(s) operation */

int m_socket(int domain_name, int type, int protocol)
{
    key_t key = ftok("init.c",64);
    int shmid = shmget(key, sizeof(struct sh)*30, 0777|IPC_CREAT);
    shm = (struct sh*)shmat(shmid, (void*)0, 0);
    key_t key1 = ftok("init.c",64);
    key_t key2 = ftok("init.c",65);
    key_t key3 = ftok("init.c",66);
    int mutex = semget(key1, 1, 0666|IPC_CREAT);
    int main_wait = semget(key2, 1, 0666|IPC_CREAT);
    int func_wait = semget(key3, 1, 0666|IPC_CREAT);
    myprintf("mutex = %d\n",mutex);
    myprintf("main_wait = %d\n",main_wait);
    myprintf("func_wait = %d\n",func_wait);
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
    int shmid2 = shmget(key2, sizeof(struct sockinfo), 0777|IPC_CREAT);
    struct sockinfo* SOCK_INFO = (struct sockinfo*)shmat(shmid2, (void*)0, 0);
    if(shmid==-1)
    {
        perror("shmget");
        exit(1);
    }
    if(shmid2==-1)
    {
        perror("shmget");
        exit(1);
    }
    if(SOCK_INFO==-1)
    {
        perror("shmat");
        exit(1);
    }
    P(mutex);
    int index=-1;
    if(domain_name != MTP_SOCKET)
    {
        errno = EINVAL;
        return -1;
    }
    if(type != SOCK_MTP)
    {
        errno = EINVAL;
        return -1;
    }
    for(int i=0;i<25;i++)
    {
       if(shm[i].free)
       {
            index = i;
            shm[i].free=0;
            break;
       }
    }
    V(mutex);
    if(index==-1)
    {
        errno = ENOBUFS;
        return -1;
    }
    P(mutex);
    if(index!=-1)
    {
        SOCK_INFO->sock_id = 0;
        SOCK_INFO->err_no = 0;
        SOCK_INFO->port = 0;
        shm[index].pid = getpid();
        for(int j=0;j<16;j++)
        {
            shm[index].timers[j].tm_hour = 1e9;
        }
        shm[index].sendwindow.start = shm[index].sendwindow.mid  = shm[index].sendwindow.next_seq_no = 0;
        shm[index].sendwindow.end = 5;
        shm[index].receivewindow.next_expected = shm[index].receivewindow.next_supplied = 0;
        for(int j=0;j<10;j++)
        {
            shm[index].send_isfree[j] = 1;
        }
        for(int j=0;j<5;j++)
        {
            shm[index].recv_isfree[j] = 1;
        }
        shm[index].is_empty = 0; 
        shm[index].sender_port = 0;
        shm[index].receiver_port = 0;
        strcpy(shm[index].sender_ip,"");
        strcpy(shm[index].receiver_ip,"");
        myprintf("Free index found at %d\n",index);
    }
    V(mutex);
    V(main_wait);
    P(func_wait);
    P(mutex);
    if(SOCK_INFO->err_no)
    {
        errno = SOCK_INFO->err_no;
        shm[index].free = 1;
        V(mutex);
        return -1;
    }
    shm[index].sockfd = SOCK_INFO->sock_id;
    myprintf("socket created at index %d with sockfd %d\n",index,shm[index].sockfd);
    V(mutex);
    return index;
}
int m_bind(int sockfd,char* sourceip, int sourceport, char* destinationip, int destinationport)
{
    if(sockfd<0||sockfd>=25)
    {
        errno = EINVAL;
        return -1;
    }
    key_t key = ftok("init.c",64);
    int shmid = shmget(key, sizeof(struct sh)*30, 0777|IPC_CREAT);
    shm = (struct sh*)shmat(shmid, (void*)0, 0);
    key_t key1 = ftok("init.c",64);
    key_t key2 = ftok("init.c",65);
    key_t key3 = ftok("init.c",66);
    int mutex = semget(key1, 1, 0666|IPC_CREAT);
    int main_wait = semget(key2, 1, 0666|IPC_CREAT);
    int func_wait = semget(key3, 1, 0666|IPC_CREAT);
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
    int shmid2 = shmget(key2, sizeof(struct sockinfo), 0777|IPC_CREAT);
    struct sockinfo* SOCK_INFO = (struct sockinfo*)shmat(shmid2, (void*)0, 0);
    P(mutex);
    if(shm[sockfd].free)
    {
        errno = ENOTBOUND;
        V(mutex);
        return -1;
    }
    if(shm[sockfd].pid!=getpid())
    {
        errno = EACCES;
        V(mutex);
        return -1;
    }
    SOCK_INFO->sock_id = shm[sockfd].sockfd;
    SOCK_INFO->err_no = 0;
    SOCK_INFO->port = sourceport;
    strcpy(SOCK_INFO->ip,sourceip);
    
    V(mutex);
    V(main_wait);
    P(func_wait);
    P(mutex);
    if(SOCK_INFO->err_no)
    {
        errno = SOCK_INFO->err_no;
        V(mutex);
        return -1;
    }
    shm[sockfd].receiver_port = destinationport;
    strcpy(shm[sockfd].receiver_ip,destinationip);
    shm[sockfd].sender_port = sourceport;
    strcpy(shm[sockfd].sender_ip,sourceip);
    V(mutex);
    myprintf("Bind complete\n");
    return 0;
}
int m_sendto(int sockfd,char* buffer,int len,int flags,struct sockaddr_in cliaddr,int clilen)
{
    if(sockfd<0||sockfd>=25)
    {
        errno = EINVAL;
        return -1;
    }
    key_t key = ftok("init.c",64);
    int shmid = shmget(key, sizeof(struct sh)*30, 0777|IPC_CREAT);
    shm = (struct sh*)shmat(shmid, (void*)0, 0);
    key_t key1 = ftok("init.c",64);
    key_t key2 = ftok("init.c",65);
    key_t key3 = ftok("init.c",66);
    int mutex = semget(key1, 1, 0666|IPC_CREAT);
    int main_wait = semget(key2, 1, 0666|IPC_CREAT);
    int func_wait = semget(key3, 1, 0666|IPC_CREAT);
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
    P(mutex);
    if(shm[sockfd].free)
    {
        errno = ENOTBOUND;
        myprintf("Free buffer\n");
        V(mutex);
        return -1;
    }
    if(shm[sockfd].pid!=getpid())
    {
        errno = EACCES;
        myprintf("Not my process\n");
        V(mutex);
        return -1;
    }
    if(strcmp(shm[sockfd].receiver_ip,inet_ntoa(cliaddr.sin_addr))!=0||shm[sockfd].receiver_port!=ntohs(cliaddr.sin_port))
    {
        errno = ENOTBOUND;
        myprintf("%s,%s,%d,%d\n",shm[sockfd].receiver_ip,inet_ntoa(cliaddr.sin_addr),shm[sockfd].receiver_port,ntohs(cliaddr.sin_port));
        myprintf("Not bound to this address\n");
        V(mutex);
        return -1;
    }
    int sequence_number = shm[sockfd].sendwindow.next_seq_no;
    shm[sockfd].sendwindow.next_seq_no = (shm[sockfd].sendwindow.next_seq_no+1)%16;
    int index=-1;
    for(int i=0;i<10;i++)
    {
        if(shm[sockfd].send_isfree[i])
        {
            index = i;
            shm[sockfd].send_isfree[i] = 0;
            break;
        }
    }
    if(index==-1)
    {
        errno = ENOBUFS;
        V(mutex);
        return -1;
    }
    char temp[1024];
    for(int i=0;i<len;i++)
    {
        temp[i]=0;
    }
    for(int i=4;i>=0;i--)
    {
        if((sequence_number>>i)&1)
        {
            temp[0]+=1<<(i+1);
        }
    }
    for(int i=0;i<len;i++)
    {
        temp[i+1] = buffer[i];
    }
    strcpy(shm[sockfd].sendbuf[index],temp);
    V(mutex);
    return 0;
}

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
    int temp1 = m_sendto(sockfd,buffer,5,0,cliaddr,sizeof(cliaddr));
    if(temp1)
    {
        perror("Error in sendto");
    }
}
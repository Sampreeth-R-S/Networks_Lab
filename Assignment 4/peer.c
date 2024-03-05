#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include<sys/wait.h>
#include<signal.h>
#include<errno.h>
#include<sys/time.h>
#include<sys/select.h>
#ifndef DEBUG
    #define myprint //
#else 
    #define myprint printf
#endif
int main(int argc, char*argv[])
{
    struct sockaddr_in server;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_family=AF_INET;
    server.sin_port=htons(atoi(argv[1]));
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
        perror("socket: ");
        exit(0);
    }
    if(bind(sockfd,(struct sockaddr*)&server,sizeof(server))<0)
    {
        perror("bind: ");
        exit(0);
    }
    int curport=atoi(argv[1]);
    int curuser;
    listen(sockfd,10);
    int user[3]={50000,50001,50002};
    for(int i=0;i<3;i++)
    {
        if(curport==user[i])curuser=i;
    }
    printf("Started server for user %d(type exit to exit)\n",curuser+1);
    int fd[5];
    fd[0]=sockfd;
    fd[1]=0;
    for(int i=2;i<5;i++)fd[i]=-1;
    int is_connected[3]={0,0,0};
    int clientfd[3];
    for(int i=0;i<3;i++)clientfd[i]=-1;
    while(1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd=0;
        for(int i=0;i<4;i++)
        {
            if(fd[i]!=-1)
            {
                FD_SET(fd[i],&readfds);
                if(fd[i]>maxfd)maxfd=fd[i];
            }
        }
        struct timeval timer;
        timer.tv_sec=5;
        timer.tv_usec=0;
        int result=select(maxfd+1,&readfds,NULL,NULL,&timer);
        if(result==0)
        {
            myprint("No input detected for long time, closing all connections\n");
            fflush(stdout);
            for(int i=0;i<3;i++)
            {
                if(is_connected[i])
                {
                    close(clientfd[i]);
                    clientfd[i]=-1;
                    is_connected[i]=0;
                }
            }
        }
        if(FD_ISSET(fd[0],&readfds))
        {
            struct sockaddr_in client;
            int len=sizeof(client);
            int newfd=accept(sockfd,(struct sockaddr*)&client,&len);
            if(newfd<0)
            {
                perror("connect");
                exit(0);
            }
            int clientport=ntohs(client.sin_port);
            for(int i=0;i<5;i++)
            {
                if(fd[i]==-1)
                {
                    fd[i]=newfd;
                    myprint("New fd %d at %d\n",newfd,i);
                    break;
                }
            }
        }
        if(FD_ISSET(fd[1],&readfds))
        {
            char buffer[1000];
            fgets(buffer,1000,stdin);
            int i=0;
            char name[1000];
            while(buffer[i]!='/')
            {
                name[i]=buffer[i];
                i++;
            }
            if(strcmp(buffer,"exit\n")==0)
            {
                printf("Closing chatapp\n");
                exit(0);
            }
            name[i]=='\0';
            char temp[3];
            temp[0]=name[i-1];
            temp[1]='\0';
            i++;
            
            int cliname=atoi(temp)-1;
            myprint("client=%d\n",cliname);
            if(!is_connected[cliname])
            {
                struct sockaddr_in serv;
                serv.sin_family = AF_INET;
                inet_aton("127.0.0.1", &serv.sin_addr);
                serv.sin_port = htons(user[cliname]);
                int socktemp=socket(AF_INET,SOCK_STREAM,0);
                if (connect(socktemp, (struct sockaddr *)&serv, sizeof(serv)) < 0)
                {
                    perror("connect: ");
                    exit(0);
                }
                is_connected[cliname]=1;
                clientfd[cliname]=socktemp;
            }
            char message[1000];
            int j=0;
            while(buffer[i]!='\n')
            {
                message[j++]=buffer[i++];
                myprint("%d ",i);
                fflush(stdout);
            }
            message[j++]=buffer[i++];
            message[j++]='\0';
            myprint("Sending at %d\n",clientfd[cliname]);
            char sendmessage[1000];
            sprintf(sendmessage,"User %d: %s",curuser+1,message);
            send(clientfd[cliname],sendmessage,strlen(sendmessage),0);
        }
        for(int i=2;i<5;i++)
        {
            if(fd[i]==-1)
            {
                continue;
            }
            myprint("fd set at %d=%d\n",i,FD_ISSET(fd[i],&readfds));
            if(FD_ISSET(fd[i],&readfds))
            {
                int end=0;
                //printf("Message from ");
                int printed=0;
                fflush(stdout);
                while(!end)
                {
                    char buffer[1000];
                    for(int i=0;i<1000;i++)buffer[i]='\0';
                    int len=recv(fd[i],buffer,1000,0);
                    myprint("%d=len\n",len);
                    
                    fflush(stdout);
                    if(len==0)
                    {
                        close(fd[i]);
                        fd[i]=-1;
                        break;
                    }
                    if(printed==0)
                    {
                        printed=1;
                        printf("Message from ");

                    }
                    printf("%s",buffer);
                    for(int i=0;i<1000;i++)
                    {
                        if(buffer[i]=='\n')end=1;
                    }
                }
            }
        }
    }

}

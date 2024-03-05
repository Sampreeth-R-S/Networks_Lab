#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include<time.h>
int checkuser(char* user)
{
    FILE* f=fopen("user.txt","r");
    //printf("%s\n",user);
    if(f==NULL)
    {
        printf("Error opening file\n");
        exit(0);
    }
    //for(int i=0;i<strlen(user);i++)printf("%d ",user[i]);
    while(!feof(f))
    {
        char buffer[1000];
        fgets(buffer,1000,f);
        //printf("%s\n",buffer);
        char username[1000];
        int i=0;
        //printf("Hello\n");
        while(buffer[i]!=' ')
        {
            //printf("%d\n",i);
            username[i]=buffer[i];
            i++;
            //printf("%d\n",i);
        }
        username[i]='\0';
        //printf("%s=user,%s=username,%d=strcmp\n",user,username,strcmp(username,user));
        if(strcmp(username,user)==0)
        {
            fclose(f);
            //printf("Returning\n");
            return 1;
        }
        //printf("Hello\n");
        fflush(stdout);
    }
    fclose(f);
    return 0;
}
void tokenise(char* buffer,char** result)
{
    int i=0;
    int j=0,k=0;
    while(buffer[i]!='\r')
    {
        if(buffer[i]==' ')
        {
            result[k][j]='\0';
            k++;
            j=0;
            while(buffer[i]==' ')i++;
            continue;
        }
        else
        {
            result[k][j]=buffer[i];
            j++;
        }
        i++;
    }
    result[k][j]='\0';
}
char buffer2[1000];
int curpointer=0;
int prevlen=0;
void receive(int sockfd,char* buffer)
{
    int i=0;int count=0;
    for(int i=0;i<1000;i++)buffer[i]='\0';
    while(count<2)
    {
        if(curpointer==prevlen){
            for(int i=0;i<1000;i++)buffer2[i]='\0';
            prevlen=recv(sockfd,buffer2,1000,0);
            curpointer=0;
        }
        for(;curpointer<prevlen&&count<2;curpointer++)
        {
            buffer[i++]=buffer2[curpointer];
            //printf("%d ",buffer2[curpointer]);fflush(stdout);
            if(buffer2[curpointer]==10)count++;
            if(buffer2[curpointer]==13)count++;
            //printf("%d=count\n",count);
        }
        if(prevlen==0)break;
    }
    //printf("Receive returned\n");
    //fflush(stdout);
}
int main(int argc,char* argv[])
{
    if(argc==1)
    {
        printf("Usage: ./smtpserver <port>\n");
        exit(0);
    }
    int sockfd;
    struct sockaddr_in servaddr;
    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=INADDR_ANY;
    servaddr.sin_port=htons(atoi(argv[1]));
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
        perror("socket: ");
        exit(0);
    }
    int status=bind(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    if(status<0)
    {
        perror("bind: ");
        exit(0);
    }
    listen(sockfd,5);
    struct sockaddr_in cliaddr;
    int clilen=sizeof(cliaddr);
    while(1)
    {
        int newsockfd=accept(sockfd,(struct sockaddr*)&cliaddr,&clilen);
        if(newsockfd<0)
        {
            perror("accept: ");
        }
        int pid=fork();
        if(pid)
        {
            close(newsockfd);
            continue;
        }
        close(sockfd);
        char buffer[1000];
        for(int i=0;i<1000;i++)buffer[i]='\0';
        sprintf(buffer,"%d <coldmail.com> Service ready\r\n",220);
        send(newsockfd,buffer,strlen(buffer),0);
        //printf("%d HELLO\n",strlen(buffer));
        printf("Service ready sent\n");
        for(int i=0;i<1000;i++)buffer[i]='\0';
        receive(newsockfd,buffer);
        char** result=(char**)malloc(100*sizeof(char*));
        for(int i=0;i<100;i++)result[i]=(char*)malloc(100*sizeof(char));
        tokenise(buffer,result);
        if(strcmp(result[0],"HELO")!=0)
        {
            sprintf(buffer,"501 Syntax error in parameters or arguments.\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            close(newsockfd);
            exit(0);
        }
        for(int i=0;i<1000;i++)buffer[i]='\0';
        sprintf(buffer,"250 OK hello coldmail.com\r\n");
        send(newsockfd,buffer,strlen(buffer),0);
        for(int i=0;i<1000;i++)buffer[i]='\0';
        receive(newsockfd,buffer);
        tokenise(buffer,result);
        for(int i=0;i<1000;i++)buffer[i]='\0';
        char sender[100];
        for(int i=1;i<strlen(result[2])-1;i++)sender[i-1]=result[2][i];
        sender[strlen(result[2])-2]='\0';
        int flag=checkuser(sender);
        if(!flag)
        {
            for(int i=0;i<1000;i++)buffer[i]='\0';
            sprintf(buffer,"550 No such user\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            close(newsockfd);
            exit(0);
        }
        for(int i=0;i<1000;i++)buffer[i]='\0';
        sprintf(buffer,"250 %s Sender ok\r\n",result[2]);
        send(newsockfd,buffer,strlen(buffer),0);
        for(int i=0;i<1000;i++)buffer[i]='\0';
        receive(newsockfd,buffer);
        tokenise(buffer,result);
        if(strcmp(result[0],"RCPT"))
        {
            sprintf(buffer,"501 Syntax error in parameters or arguments.\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            close(newsockfd);
            exit(0);
        }
        char** receivers=(char**)malloc(100*sizeof(char*));
        int curreceiver=0;
        while(strcmp(result[0],"RCPT")==0)
        {
            //printf("Hello\n");
            char username[1000];
            for(int i=0;i<1000;i++)username[i]='\0';
            for(int i=1;i<strlen(result[2])-1;i++)username[i-1]=result[2][i];
            username[strlen(result[2])-2]='\0';
            int flag=checkuser(username);
            //printf("%d\n",flag);
            if(flag)
            {
                receivers[curreceiver++]=strdup(username);
                for(int i=0;i<1000;i++)buffer[i]='\0';
                sprintf(buffer,"250 root... Recipient OK\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
            }
            else
            {
                for(int i=0;i<1000;i++)buffer[i]='\0';
                sprintf(buffer,"550 No such user\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                close(newsockfd);
                exit(0);
            }
            for(int i=0;i<1000;i++)buffer[i]='\0';
            receive(newsockfd,buffer);
            tokenise(buffer,result);
            //printf("%s\n",result[0]);
        }
        printf("Data\n");
        fflush(stdout);
        if(strcmp(result[0],"DATA"))
        {
            sprintf(buffer,"501 Syntax error in parameters or arguments.\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            close(newsockfd);
            exit(0);
        }
        for(int i=0;i<1000;i++)buffer[i]='\0';
        sprintf(buffer,"354 Enter mail, end with \".\" on a line by itself\r\n");
        send(newsockfd,buffer,strlen(buffer),0);
        char** mail=(char**)malloc(100*sizeof(char));
        int curline=0;
        while(1)
        {
            receive(newsockfd,buffer);
            mail[curline++]=strdup(buffer);
            if(strcmp(buffer,".\r\n")==0)
            {
                break;
            }
            //printf("%d\n",curline);
            //printf("%s\n",buffer);
            //for(int i=0;i<strlen(buffer);i++)printf("%d ",buffer[i]);
            //printf("\n");
        }
        //printf("Hello\n");
        for(int i=0;i<1000;i++)buffer[i]='\0';
        sprintf(buffer,"250 OK message accepted for delivery\r\n");
        send(newsockfd,buffer,strlen(buffer),0);
        receive(newsockfd,buffer);
        if(strcmp(buffer,"QUIT\r\n"))
        {
            sprintf(buffer,"501 Syntax error in parameters or arguments.\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            close(newsockfd);
            exit(0);
        }
        printf("Bye\n");
        for(int i=0;i<1000;i++)buffer[i]='\0';
        sprintf(buffer,"221 coldmail.com closing connection\r\n");
        send(newsockfd,buffer,strlen(buffer),0);
        close(newsockfd);
        for(int i=0;i<curreceiver;i++)
        {
            char filename[1000];
            sprintf(filename,"%s/mymailbox",receivers[i]);
            int fd=open(filename,O_RDWR|O_CREAT|O_APPEND,0666);
            if(fd<0)
            {
                perror("Unable to open file\n");
                exit(0);
            }
            char temp[1000];
            for(int i=0;i<1000;i++)temp[i]='\0';
            sprintf(temp,"From: %s\r\n",sender);
            write(fd,temp,strlen(temp));
            for(int i=0;i<1000;i++)temp[i]='\0';
            sprintf(temp,"To: %s\r\n",receivers[i]);
            write(fd,temp,strlen(temp));
            for(int i=0;i<1000;i++)temp[i]='\0';
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            sprintf(temp,"Received: %s\r\n",asctime(tm));
            write(fd,temp,strlen(temp));
            for(int j=0;j<curline;j++)
            {
                write(fd,mail[j],strlen(mail[j]));
            }
        }
        exit(0);
    }
}
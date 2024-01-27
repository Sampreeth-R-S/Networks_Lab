#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
int main()
{
    int sockid;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(1212);
    if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Unable to create socket\n");
        exit(0);
    }
    if (bind(sockid, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Unable to bind local address\n");
        exit(0);
    }
    listen(sockid, 5);
    int newsockid;
    struct sockaddr_in cli_addr;
    int clilen=sizeof(cli_addr);
    while(2)
    {
        if((newsockid = accept(sockid, (struct sockaddr *)&cli_addr, &clilen)) < 0)
        {
            perror("Unable to accept connection\n");
            exit(0);
        }
        int pid=fork();
        if(pid==0)
        {
            close(sockid);
            char buf[100];
            int len;
            //printf("Hello\n");
            int key;
            char keyrecv[100];
            int i=0;
            int j;
            while(1)
            {
                len=recv(newsockid,buf,100,0);
                j=0;
                while(j<len)
                {
                    if(buf[j]=='#')
                    {
                        keyrecv[i]='\0';
                        break;
                    }
                    keyrecv[i++]=buf[j++];
                }
                if(j<100&&buf[j]=='#'){j++;break;}
            }
            key=atoi(keyrecv);
            //printf("%d\n",key);
            char filename[300];
            char temp1[100],temp2[100];
            int port;
            inet_ntop(cli_addr.sin_family, &(cli_addr.sin_addr), temp1, sizeof(temp1));
            printf("Client %s",temp1);
            port = htons(cli_addr.sin_port);
            printf(" at port %d connected\n",port);
            sprintf(temp2,"%d",port);
            sprintf(filename,"%s.%s.txt",temp1,temp2);
            int fd=open(filename,O_RDWR|O_CREAT,0666);
            if(fd<0)
            {
                printf("Error opening file\n");
                close(newsockid);
                exit(0);
            }
            int ended=0;
            for(int i=j;i<len;i++)
            {
                if(buf[i]=='#')
                {
                    ended=1;
                    break;
                }
            }
            if(ended)
            {
                write(fd,buf+j,len-j-1);
            }
            else{
                write(fd,buf+j,len-j);
            }
            for(int i=0;i<100;i++)buf[i]='\0';
            while(!ended)
            {
                len=recv(newsockid,buf,100,0);
                //write(fd,buf,len);
                //or(int i=0;i<len;i++)printf("%c\n",buf[i]);
                //printf("len=%d\n",len);
                int end=0;
                j=0;
                while(j<len)
                {
                    if(buf[j]=='#')break;
                    j++;
                }
                if(j<100&&buf[j]=='#')
                {
                    end=1;
                }
                if(end)
                {
                    write(fd,buf,len-1);
                }
                else{
                    write(fd,buf,len);
                }
                if(j<100&&buf[j]=='#')break;
            }
            printf("File received successfully\n");
            close(fd);
            fd=open(filename,O_RDONLY);
            if(fd<0)
            {
                printf("Error opening file\n");
                close(newsockid);
                exit(0);
            }
            for(int i=0;i<100;i++)buf[i]='\0';
            char filename2[400];
            sprintf(filename2,"%s.enc",filename);
            int fd2=open(filename2,O_RDWR|O_CREAT,0666);
            if(fd2<0)
            {
                printf("Error opening file\n");
                close(newsockid);
                exit(0);
            }
            while((len=read(fd,buf,100))>0)
            {
                for(int i=0;i<len;i++)
                {
                    if(buf[i]<='z'&&buf[i]>='a')
                    {
                        buf[i]='a'+(buf[i]-'a'+key)%26;
                    }
                    else if(buf[i]<='Z'&&buf[i]>='A')
                    {
                        buf[i]='A'+(buf[i]-'A'+key)%26;
                    }
                }
                write(fd2,buf,len);
            }
            close(fd);
            fd2=open(filename2,O_RDONLY);
            if(fd2<0)
            {
                printf("Error opening file\n");
                close(newsockid);
                exit(0);
            }
            printf("Sending file\n");
            while((len=read(fd2,buf,100))>0)
            {
                send(newsockid,buf,len,0);
            }
            close(fd2);
            close(newsockid);
            break;
        }
        else{
            close(newsockid);
            continue;
        }
    }
}
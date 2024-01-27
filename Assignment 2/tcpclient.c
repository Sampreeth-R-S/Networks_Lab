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
    while(1)
    {
        char filename[1000];
        printf("Enter filename: ");
        scanf("%s",filename);
        int fd=open(filename,O_RDONLY);
        while(fd<0)
        {
            printf("Enter valid filename: ");
            scanf("%s",filename);
            fd=open(filename,O_RDONLY);
        }
        printf("Enter key: ");
        int k;
        scanf("%d",&k);
        int sockid;
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        inet_aton("127.0.0.1", &serv_addr.sin_addr);
        serv_addr.sin_port = htons(1212);
        if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Unable to create socket\n");
            exit(0);
        }
        if ((connect(sockid, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
        {
            perror("Unable to connect to server\n");
            exit(0);
        }
        char buf[100];
        for(int i=0;i<100;i++) buf[i]='\0';
        sprintf(buf,"%d",k);
        int i=0;
        while(buf[i]!='\0')i++;
        buf[i]='#';
        send(sockid,buf,i+1,0);
        int len;
        while((len=read(fd,buf,100))>=100)
        {
            send(sockid,buf,len,0);
            //for(int i=0;i<100;i++)printf("%c",buf[i]);
            //for(int i=0;i<100;i++) buf[i]='\0';
        }
        if(len>=100)len=0;
        buf[len]='#';
        send(sockid,buf,len+1,0);
        close(fd);
        printf("File sent\n");
        char filename2[2000];
        sprintf(filename2,"%s.enc",filename);
        fd=open(filename2,O_RDWR|O_CREAT,0666);
        if(fd<0)
        {
            perror("Unable to create file\n");
            exit(0);
        }
        printf("Receiving file\n");
        while(1)
        {
            len=recv(sockid,buf,100,0);
            if(len==0)break;
            write(fd,buf,len);
        }
        close(fd);
        close(sockid);
    }
}
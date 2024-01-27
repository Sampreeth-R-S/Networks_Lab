#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

const int maxlen=1000;

int main()
{
    char* buffer;
    buffer=(char*)malloc(maxlen*sizeof(char));
    int sockid;
    struct sockaddr_in serveraddr,clientaddr;
    sockid=socket(AF_INET,SOCK_DGRAM,0);
    if(!sockid)
    {
        perror("Socket creation failed: ");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully\n");
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_addr.s_addr=INADDR_ANY;
    serveraddr.sin_port=htons(1308);
    clientaddr.sin_family=AF_INET;
    clientaddr.sin_addr.s_addr=INADDR_ANY;
    clientaddr.sin_port=htons(1309);
    /*if(bind(sockid,(struct sockaddr*)&clientaddr,sizeof(clientaddr))<0)
    {
        perror("Bind failed: ");
        exit(EXIT_FAILURE);
    }*/
    printf("Client Bind successful\n");
    char filename[]="content.txt";
    sendto(sockid,filename,strlen(filename),0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
    int serverlen=sizeof(serveraddr);
    while(1)
    {
        int len=recvfrom(sockid,buffer,maxlen,0,(struct sockaddr*)&serveraddr,&serverlen);
        buffer[len]='\0';
        char temp[1100];
        strcpy(temp,"NOTFOUND ");
        strcat(temp,filename);
        if(strcmp(buffer,temp)==0)
        {
            printf("File not found\n");
            exit(0);
        }
        printf("File found on server, starting receiving\n");
        int i=1;
        char temp2[1100];
        strcpy(temp2,filename);
        strcat(temp2,"(copy).txt");
        FILE*fpointer=fopen(temp2, "w");
        while(strcmp(buffer,"END")!=0)
        {
            fprintf(fpointer,"%s",buffer);
            printf("Word received: %s\n",buffer);
            //buffer="";
            char temp[100]="WORD";
            char temp2[100];
            sprintf(temp2,"%d",i);
            strcat(temp,temp2);
            i++;
            printf("Sending: %s\n",temp);
            sendto(sockid,temp,strlen(temp),0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
            int len=recvfrom(sockid,buffer,maxlen,0,(struct sockaddr*)&serveraddr,&serverlen);
            buffer[len]='\0';
        }
        fprintf(fpointer,"%s",buffer);
        printf("Word received: %s\n",buffer);
        printf("File received successfully\n");
        fclose(fpointer);
        exit(0);
    }
        
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include<string.h>
#include <arpa/inet.h>

const int maxlen=1000;

int main()
{
    char buffer[maxlen];
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
    if(bind(sockid,(struct sockaddr*)&serveraddr,sizeof(serveraddr))<0)
    {
        perror("Bind failed: ");
        exit(EXIT_FAILURE);
    }
    printf("Server Bind successful\n");
    int clientlen=sizeof(clientaddr);
    while(1)
    {
        int len=recvfrom(sockid,buffer,maxlen,0,(struct sockaddr*)&clientaddr,&clientlen);
        buffer[len]='\0';
        FILE*fpointer=fopen(buffer, "r");
        if(fpointer==NULL)
        {
            printf("File not found\n");
            char temp[1100];
            strcpy(temp,"NOTFOUND ");
            strcat(temp,buffer);
            sendto(sockid,temp,strlen(temp),0,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
            //exit(0);
        }
        else
        {
            printf("File found\n");
            char* arr[10000];
            for(int i=0;i<10000;i++)
            {
                arr[i]=(char*)malloc(1000*sizeof(char));
            }
            int i=0;
            while(!feof(fpointer))
            {
                fgets(arr[i],1000,fpointer);
                //printf("%s\n",arr[i]);
                i++;
            }
            printf("File read complete, transmitting file contents\n");
            sendto(sockid,arr[0],strlen(arr[0]),0,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
            //recvfrom(sockid,buffer,maxlen,0,(struct sockaddr*)&clientaddr,&clientlen);
            while(1)
            {
                int len=recvfrom(sockid,buffer,maxlen,0,(struct sockaddr*)&clientaddr,&clientlen);
                buffer[len]='\0';
                int j=atoi(buffer+4);
                sendto(sockid,arr[j],strlen(arr[j]),0,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
                if(j==i-1)
                {
                    break;
                }
            }
        }
    }
}
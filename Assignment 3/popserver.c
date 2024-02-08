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
#include<ctype.h>
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
int checkpass(char* user,char* pass)
{
    FILE* f=fopen("user.txt","r");
    if(f==NULL)
    {
        printf("Error opening file\n");
        exit(0);
    }
    while(!feof(f))
    {
        char buffer[1000];
        fgets(buffer,1000,f);
        char username[1000];
        int i=0;
        while(buffer[i]!=' ')
        {
            username[i]=buffer[i];
            i++;
        }
        username[i]='\0';
        if(strcmp(username,user)==0)
        {
            while(buffer[i]==' ')i++;
            int j=0;
            char password[1000];
            while(buffer[i]!='\n')
            {
                password[j++]=buffer[i++];
            }
            password[j]='\0';
            if(strcmp(password,pass)==0)
            {
                fclose(f);
                return 1;
            }
            else
            {
                fclose(f);
                return 0;
            }
        }
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
int countmail(char* filepath,int* num)
{
    FILE* f=fopen(filepath,"r");
    if(f==NULL)
    {
        printf("Error opening file\n");
        exit(0);
    }
    int count=0;
    int size=0;
    while(!feof(f))
    {
        char buffer[1000];
        fgets(buffer,1000,f);
        size+=strlen(buffer)-1;
        if(strcmp(buffer,".\r\n")==0)
        {
            count++;
            size-=strlen(buffer)-1;
        }
    }
    fclose(f);
    *num=size;
    return count;
}
int stat(char*filepath,int* num,int* delete)
{
    FILE* f=fopen(filepath,"r");
    if(f==NULL)
    {
        printf("Error opening file\n");
        exit(0);
    }
    int count=0;
    int size=0;
    while(!feof(f))
    {
        if(delete[count]==1)
        {
            char buffer[1000];
            fgets(buffer,1000,f);
            if(strcmp(buffer,".\r\n")==0)
            {
                count++;
            }
            continue;
        }
        char buffer[1000];
        fgets(buffer,1000,f);
        size+=strlen(buffer)-1;
        if(strcmp(buffer,".\r\n")==0)
        {
            count++;
            size-=strlen(buffer)-1;
        }
    }
    fclose(f);
    *num=size;
    return count;

}
void list(char* filepath,int* maillen,int * delete)
{
    FILE* f=fopen(filepath,"r");
    if(f==NULL)
    {
        printf("Error opening file\n");
        exit(0);
    }
    int count=0;
    int size=0;
    int index=0;
    while(!feof(f))
    {
        char buffer[1000];
        fgets(buffer,1000,f);
        size+=strlen(buffer)-1;
        if(strcmp(buffer,".\r\n")==0)
        {
            count++;
            size-=strlen(buffer)-1;
            maillen[index++]=size;
        }
    }
    fclose(f);
    
}
int main(int argc, char* argv[])
{
    if(argc!=2)
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
            exit(0);
        }
        if(fork()==0)
        {
            close(sockfd);
            char buffer[1000];
            char* result[100];
            for(int i=0;i<100;i++)
            {
                result[i]=(char*)malloc(100*sizeof(char));
            }
            sprintf(buffer,"+OK POP3 server ready\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            receive(newsockfd,buffer);
            tokenise(buffer,result);
            while(strcmp(result[0],"USER")!=0)
            {
                if(strcmp(result[0],"QUIT")==0)
                {
                    sprintf(buffer,"+OK POP3 server signing off\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    close(newsockfd);
                    exit(0);
                }
                sprintf(buffer,"-ERR Invalid command\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                receive(newsockfd,buffer);
                tokenise(buffer,result);
            }
            int i=0;
            char temp[1000];
            while(buffer[i]!=' ')
            {
                i++;
            }
            while(buffer[i]==' ')i++;
            int j=0;
            while(buffer[i]!='\r')
            {
                temp[j++]=buffer[i++];
            }
            temp[j]='\0';
            while(checkuser(temp)==0)
            {
                sprintf(buffer,"-ERR Invalid user\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                receive(newsockfd,buffer);
                tokenise(buffer,result);
                while(strcmp(result[0],"USER")!=0)
                {
                    if(strcmp(result[0],"QUIT")==0)
                    {
                        sprintf(buffer,"+OK POP3 server signing off\r\n");
                        send(newsockfd,buffer,strlen(buffer),0);
                        close(newsockfd);
                        exit(0);
                    }
                    sprintf(buffer,"-ERR Invalid command\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    receive(newsockfd,buffer);
                    tokenise(buffer,result);
                }
                i=0;
                while(buffer[i]!=' ')
                {
                    i++;
                }
                while(buffer[i]==' ')i++;
                j=0;
                while(buffer[i]!='\r')
                {
                    temp[j++]=buffer[i++];
                }
                temp[j]='\0';
            }
            char user[100];
            strcpy(user,temp);
            sprintf(buffer,"+OK User accepted\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            receive(newsockfd,buffer);
            tokenise(buffer,result);
            if(strcmp(result[0],"PASS")!=0)
            {
                sprintf(buffer,"-ERR Invalid command\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                sprintf(buffer,"-ERR Authorisation failed\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                sprintf(buffer,"-ERR POP3 server signing off\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                close(newsockfd);
                exit(0);
            }
            i=0;
            while(buffer[i]!=' ')
            {
                i++;
            }
            while(buffer[i]==' ')i++;
            j=0;
            while(buffer[i]!='\r')
            {
                temp[j++]=buffer[i++];
            }
            temp[j]='\0';
            if(checkpass(user,temp)==0)
            {
                sprintf(buffer,"-ERR Invalid password\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                sprintf(buffer,"-ERR Authorisation failed\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                sprintf(buffer,"-ERR POP3 server signing off\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                close(newsockfd);
                exit(0);
            }
            char filepath[1000];
            strcpy(filepath,user);
            strcat(filepath,"/mymailbox");
            FILE* f=fopen(filepath,"r");
            if(f==NULL)
            {
                sprintf(buffer,"+OK Mailbox is empty\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                sprintf(buffer,"+OK POP3 server signing off\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                close(newsockfd);
                exit(0);
            }
            int num;
            int count=countmail(filepath,&num);
            int delete[count];
            for(int i=0;i<count;i++)
            {
                delete[i]=0;
            }
            
            sprintf(buffer,"+OK %s's mailbox has %d messages (%d octets)\r\n",user,count,num);
            send(newsockfd,buffer,strlen(buffer),0);
            while(1)
            {
                receive(newsockfd,buffer);
                tokenise(buffer,result);
                int i=0;
                while(result[0][i])
                {
                    result[0][i]=toupper(result[0][i]);
                    i++;
                }
                if(strcmp("STAT",result[0]))
                {
                    //count=stat(filepath,&num,delete);
                    int tempcount,tempnum;
                    tempcount=stat(filepath,&tempnum,delete);
                    sprintf(buffer,"+OK %d %d\r\n",tempcount,tempnum);
                    send(newsockfd,buffer,strlen(buffer),0);
                }
                else if(strcmp("LIST",result[0])==0)
                {
                    int tempnum;
                    int tempcount=stat(filepath,&tempnum,delete);
                    int maillen[count];
                    for(int i=0;i<count;i++)
                    {
                        maillen[i]=0;
                    }
                    list(filepath,maillen,delete);
                    if(result[1][0]=='\0')
                    {
                        sprintf(buffer,"+OK %d messages (%d octets)\r\n",tempcount,tempnum);
                        send(newsockfd,buffer,strlen(buffer),0);
                        int index=0;
                        for(int i=0;i<count;i++)
                        {
                            if(delete[i]==1)continue;
                            sprintf(buffer,"%d  %d\r\n",++index,maillen[i]);
                            send(newsockfd,buffer,strlen(buffer),0);
                        }
                        sprintf(buffer,".\r\n");
                        send(newsockfd,buffer,strlen(buffer),0);
                    }
                    else{
                        int index=atoi(result[1]);
                        if(index>count)
                        {
                            sprintf(buffer,"-ERR No such message, only %d messages in maildrop\r\n",count);
                            send(newsockfd,buffer,strlen(buffer),0);
                        }
                        else if(delete[index-1]==1)
                        {
                            sprintf(buffer,"-ERR Message %d has been deleted\r\n",index);
                            send(newsockfd,buffer,strlen(buffer),0);
                        }
                        else{
                            sprintf(buffer,"+OK %d %d\r\n",index,maillen[index-1]);
                            send(newsockfd,buffer,strlen(buffer),0);
                        }
                    
                    }
                }
                else if(strcmp("RETR",result[0])==0)
                {
                    if(result[1][0]=='\0')
                    {
                        sprintf(buffer,"-ERR Invalid command\r\n");
                        send(newsockfd,buffer,strlen(buffer),0);
                        continue;
                    }
                    printf("Inside Retr\n");
                    //count=stat(filepath,&num,delete);
                    int maillen[count];
                    for(int i=0;i<count;i++)
                    {
                        maillen[i]=0;
                    }
                    list(filepath,maillen,delete);
                    int index=atoi(result[1]);
                    if(index>count)
                    {
                        sprintf(buffer,"-ERR No such message, only %d messages in maildrop\r\n",count);
                        send(newsockfd,buffer,strlen(buffer),0);
                    }
                    else if(delete[index-1])
                    {
                        sprintf(buffer,"-ERR Message %d has been deleted\r\n",index);
                        send(newsockfd,buffer,strlen(buffer),0);
                    }
                    else{
                        sprintf(buffer,"+OK %d octets\r\n",maillen[index-1]);
                        FILE* f=fopen(filepath,"r");
                        if(f==NULL)
                        {
                            printf("Error opening file\n");
                            exit(0);
                        }
                        int cur=0;
                        while(!feof(f))
                        {
                            if(cur>index)
                            {
                                break;
                            }
                            char temp[1000];
                            fgets(temp,1000,f);
                            if(cur==index)
                            {
                                send(newsockfd,temp,strlen(temp),0);
                            }
                            if(strcmp(temp,".\r\n")==0)
                            {
                                cur++;
                            }

                        }
                    
                    }
                }
                else if(strcmp(result[0],"DELE")==0)
                {
                    if(result[1][0]=='\0')
                    {
                        sprintf(buffer,"-ERR Invalid command\r\n");
                        send(newsockfd,buffer,strlen(buffer),0);
                        continue;
                    }
                    //count=stat(filepath,&num,delete);
                    int maillen[count];
                    for(int i=0;i<count;i++)
                    {
                        maillen[i]=0;
                    }
                    list(filepath,maillen,delete);
                    int index=atoi(result[1]);
                    if(index>count)
                    {
                        sprintf(buffer,"-ERR No such message, only %d messages in maildrop\r\n",count);
                        send(newsockfd,buffer,strlen(buffer),0);
                    }
                    else if(delete[index-1])
                    {
                        sprintf(buffer,"-ERR Message %d has been deleted\r\n",index);
                        send(newsockfd,buffer,strlen(buffer),0);
                    }
                    else{
                        delete[index-1]=1;
                        sprintf(buffer,"+OK Message %d deleted\r\n",index);
                        send(newsockfd,buffer,strlen(buffer),0);
                    }
                }
                else if(strcmp(result[0],"RSET")==0)
                {
                    for(int i=0;i<count;i++)
                    {
                        delete[i]=0;
                    }
                    //count=stat(filepath,&num,delete);
                    sprintf(buffer,"+OK Maildrop has %d messages (%d octets)\r\n",count,num);
                    send(newsockfd,buffer,strlen(buffer),0);
                }
                else if(strcmp(result[0],"QUIT")==0)
                {
                    sprintf(buffer,"+OK POP3 server signing off\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    close(newsockfd);
                    //deletemail(filepath,delete);
                    break;
                }
                else
                {
                    sprintf(buffer,"-ERR Invalid command\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                }
                
            }
            exit(0);

        }
        else{
            close(newsockfd);
            continue;
        }
    }
}
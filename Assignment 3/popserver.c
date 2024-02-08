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
    char buffer[1000];
    while(fgets(buffer,1000,f)!=NULL)
    {
        
        //fgets(buffer,1000,f);
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
    int count=0;
    char buffer[1000];
    while(fgets(buffer,1000,f)!=NULL)
    {
        
        //fgets(buffer,1000,f);
        char username[1000];
        int i=0;
        while(buffer[i]!=' ')
        {
            username[i]=buffer[i];
            i++;
        }
        username[i]='\0';
        // for(int i=0;i<strlen(username);i++)printf("%d ",username[i]);
        // printf("\n");
        // printf("%s %d\n",username,strcmp(username,user));
        if(strcmp(username,user)==0)
        {
            while(buffer[i]==' ')i++;
            //printf("Hello\n");
            int j=0;
            char password[1000];
            //printf("buffer[i]=%c ",buffer[i]);
            while(buffer[i]!='\n'&&buffer[i]!='\r')
            {
                password[j++]=buffer[i++];
            }
            password[j]='\0';
            //printf("%s ",password);
            //for(int i=0;i<strlen(password);i++)printf("%d ",password[i]);
            fflush(stdout);
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
            // printf("%d\n",count++);
            fflush(stdout);
        }

    }
    fclose(f);
    return 0;
}
void tokenise(char* buffer,char** result)
{
    
    for(int i=0;i<100;i++)
    {
        for(int j=0;j<100;j++)result[i][j]='\0';
    }
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
int curpointer = 0;
int prevlen = 0;
char prev=0;
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
            if(buffer2[curpointer]=='\n'&&prev=='\r')
            {
                count=2;
            }
            prev=buffer2[curpointer];
            //printf("%d=count\n",count);
        }
        if(prevlen==0)break;
    }
    //printf("Receive returned\n");
    //fflush(stdout);
}

int main(int argc,char*argv[])
{
    if(argc!=2)
    {
        printf("Usage: %s <port>\n",argv[0]);
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
        for(int i=0;i<1000;i++)
        {
            buffer[i]='\0';
        }
        sprintf(buffer,"+OK POP3 server ready\r\n");
        send(newsockfd,buffer,strlen(buffer),0);
        for(int i=0;i<1000;i++)
        {
            buffer[i]='\0';
        }
        receive(newsockfd,buffer);
        char* result[100];
        for(int i=0;i<100;i++)
        {
            result[i]=(char*)malloc(100*sizeof(char));
        }
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
            for(int i=0;i<1000;i++)
            {
                buffer[i]='\0';
            }
            receive(newsockfd,buffer);
            tokenise(buffer,result);
        }
        int flag=checkuser(result[1]);
        while(!flag)
        {
            for(int i=0;i<1000;i++)
            {
                buffer[i]='\0';
            }
            sprintf(buffer,"-ERR Invalid username\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            for(int i=0;i<1000;i++)
            {
                buffer[i]='\0';
            }
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
                for(int i=0;i<1000;i++)
                {
                    buffer[i]='\0';
                }
                receive(newsockfd,buffer);
                tokenise(buffer,result);
            }
            flag=checkuser(result[1]);
        }
        for(int i=0;i<1000;i++)
        {
            buffer[i]='\0';
        }
        char user[1000];
        strcpy(user,result[1]);
        sprintf(buffer,"+OK User accepted\r\n");
        send(newsockfd,buffer,strlen(buffer),0);
        for(int i=0;i<1000;i++)
        {
            buffer[i]='\0';
        }
        receive(newsockfd,buffer);
        tokenise(buffer,result);
        if(strcmp(result[0],"PASS")!=0)
        {
            sprintf(buffer,"-ERR Invalid command\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            close(newsockfd);
            exit(0);
        }
        flag=checkpass(user,result[1]);

        if(!flag)
        {
            sprintf(buffer,"-ERR Invalid password\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            sprintf(buffer,"-ERR Authentication failed, server quitting\r\n");
            close(newsockfd);
            exit(0);
        }
        sprintf(buffer,"+OK Mailbox open\r\n");
        int count,len;
        count=0;
        len=0;
        char filepath[1000];
        strcpy(filepath,user);
        strcat(filepath,"/mymailbox");
        FILE* fp=fopen(filepath,"r");
        if(fp==NULL)
        {
            sprintf(buffer,"+OK empty mailbox\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            sprintf(buffer,"+OK POP3 server signing off\r\n");
            send(newsockfd,buffer,strlen(buffer),0);
            close(newsockfd);
            exit(0);
        }
        char temp[1000];
        while(fgets(temp,1000,fp)!=NULL)
        {
            //char temp[1000];
            //fgets(temp,1000,fp);
            len+=strlen(temp)-1;
            if(strcmp(temp,".\r\n")==0)count++;
        }
        fclose(fp);
        //count--;
        // printf("Count=%d\n",count);
        fflush(stdout);
        sprintf(buffer,"+OK %s's mailbox has %d messages (%d octets)\r\n",user,count,len);
        send(newsockfd,buffer,strlen(buffer),0);
        // printf("Sent\n");
        for(int i=0;i<1000;i++)
        {
            buffer[i]='\0';
        }
        int delete[count];
        int maillen[count+10];
        for(int i=0;i<count;i++)maillen[i]=0;
        for(int i=0;i<count;i++)delete[i]=0;
        int var2=0;
        fp=fopen(filepath,"r");
        //char temp[1000];
        while(fgets(temp,1000,fp)!=NULL)
        {
            //char temp[1000];
            //fgets(temp,1000,fp);
            maillen[var2]+=strlen(temp)-1;
            if(strcmp(temp,".\r\n")==0)var2++;
        }
        fclose(fp);
        while(1)
        {
            receive(newsockfd,buffer);
            tokenise(buffer,result);
            for(int i=0;i<strlen(result[0]);i++)result[0][i]=toupper(result[0][i]);
            // printf("%s\n",result[0]);
            if(strcmp("RETR",result[0])==0)
            {
                FILE* fp=fopen(filepath,"r");
                int cur=0;
                if(result[1][0]=='\0')
                {
                    sprintf(buffer,"-ERR Invalid command\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    continue;
                }
                int index=atoi(result[1])-1;
                // for(int i=0;i<count;i++)
                // {
                //     // printf("%d ",delete[i]);
                // }
                fflush(stdout);
                if(index>count||index<0)
                {
                    sprintf(buffer,"-ERR Invalid message number\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    continue;
                }
                if(delete[index])
                {
                    sprintf(buffer,"-ERR Message deleted\r\n");
                    printf("Message deleted\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    continue;
                }
                sprintf(buffer,"+OK %d octets\r\n",maillen[index]);
                send(newsockfd,buffer,strlen(buffer),0);
                int size=0;
                char temp[1000];
                while(fgets(temp,1000,fp)!=NULL)
                {
                    //char temp[1000];
                    //fgets(temp,1000,fp);
                    if(cur==index)
                    {
                        send(newsockfd,temp,strlen(temp),0);
                    }
                    if(strcmp(temp,".\r\n")==0)cur++;
                }
            }
            else if(strcmp("STAT",result[0])==0)
            {
                FILE* fp=fopen(filepath,"r");
                int tempcount=0;
                int templen=0;
                char temp[1000];
                while(fgets(temp,1000,fp)!=NULL)
                {
                    //char temp[1000];
                    //fgets(temp,1000,fp);
                    if(!delete[tempcount])
                    templen+=strlen(temp)-1;
                    if(strcmp(temp,".\r\n")==0)tempcount++;
                }
                int delcount=0;
                for(int i=0;i<count;i++)
                {
                    if(delete[i])delcount++;
                }
                sprintf(buffer,"+OK %d %d\r\n",count-delcount,templen);
                send(newsockfd,buffer,strlen(buffer),0);
            }
            else if(strcmp("QUIT",result[0])==0)
            {
                sprintf(buffer,"+OK POP3 server signing off\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
               
                FILE* fp2=fopen("temp.txt","w");
                FILE* fp=fopen(filepath,"r");
                int tempcount=0;
                char temp[1000];
                while(fgets(temp,1000,fp)!=NULL)
                {
                    
                    //fgets(temp,1000,fp);
                    if(!delete[tempcount])
                    {
                        fputs(temp,fp2);
                    }
                    if(strcmp(temp,".\r\n")==0)tempcount++;
                }
                fclose(fp);
                fclose(fp2);
                fp=fopen(filepath,"w");
                fp2=fopen("temp.txt","r");
                while(fgets(temp,1000,fp2)!=NULL)
                {
                    fputs(temp,fp);
                }
                fclose(fp);
                fclose(fp2);
                remove("temp.txt");
                sprintf(buffer,"goodbye\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
                exit(0);
            }
            else if(strcmp("LIST",result[0])==0)
            {
                if(result[1][0]=='\0')
                {
                    int delcount=0;
                    int templen=0;
                    for(int i=0;i<count;i++)if(delete[i]){
                        delcount++;
                        templen+=maillen[i];
                    }
                    sprintf(buffer,"+OK %d messages (%d octets)\r\n",count-delcount,len-templen);
                    send(newsockfd,buffer,strlen(buffer),0);
                    for(int i=0;i<count;i++)
                    {
                        if(!delete[i])
                        {
                            sprintf(buffer,"%d %d\r\n",i+1,maillen[i]);
                            send(newsockfd,buffer,strlen(buffer),0);
                        }
                    }

                }
            }
            else if(strcmp("DELE",result[0])==0)
            {
                if(result[1][0]=='\0')
                {
                    sprintf(buffer,"-ERR Invalid command\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    continue;
                }
                int index=atoi(result[1])-1;
                if(index>count||index<0)
                {
                    sprintf(buffer,"-ERR Invalid message number\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    continue;
                }
                if(delete[index])
                {
                    sprintf(buffer,"-ERR Message already deleted\r\n");
                    send(newsockfd,buffer,strlen(buffer),0);
                    continue;
                }
                delete[index]=1;
                sprintf(buffer,"+OK Message %d deleted\r\n",index+1);
                send(newsockfd,buffer,strlen(buffer),0);
            }
            else if(strcmp("RSET",result[0])==0)
            {
                for(int i=0;i<count;i++)delete[i]=0;
                sprintf(buffer,"+OK Mailbox has %d messages\r\n",count);
                send(newsockfd,buffer,strlen(buffer),0);
            }
            else
            {
                sprintf(buffer,"-ERR Invalid command\r\n");
                send(newsockfd,buffer,strlen(buffer),0);
            }
        }




        exit(0);
    }

}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
void tokenise(char* buffer,char** result)
{
    int i=0;
    int j=0,k=0;
    for(int i=0;i<100;i++)
    {
        free(result[i]);
        result[i]=(char*)malloc(100*sizeof(char));
    }
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
int checkmailsyntax(char*temp)
{
    int i=0;
    while(temp[i]!='@')
    {
        if(temp[i]=='\0')
        {
            printf("Syntax error, @ expected in From\n");
            return 0;
        }
        i++;
    }
    i++;
    while(temp[i]!='\0')
    {
        if(temp[i]=='.')break;
        i++;
    }
    if(temp[i]=='\0')
    {
        printf("Syntax error, . expected in From\n");
        return 0;
    }
    return 1;
}
int checksyntax(char**mail,char sender[],char*receivers)
{
    /*for(int i=0;i<6;i++)
    {
        printf("%s",mail[i]);
    }*/
    char temp[100];
    for(int i=0;i<100;i++)temp[i]='\0';
    int i=0;
    while(mail[0][i]!=' '&&mail[0][i]!='\n')
    {
        temp[i]=mail[0][i];
        i++;
    }
    temp[i]='\0';
    //printf("%d\n",temp[0]);
    //printf("%d\n",mail[0][0]);
    if(strcmp(temp,"From:")!=0)
    {
        printf("Syntax ,From expected in first line\n");
        return 0;
    }
    while(mail[0][i]==' ')i++;
    int j=0;
    for(int k=0;k<100;k++)temp[k]='\0';
    while(mail[0][i]!='\n')
    {
        temp[j]=mail[0][i];
        i++;j++;
    }
    temp[j]='\0';
    int flag=checkmailsyntax(temp);
    if(!flag)
    {
        return 0;
    }
    strcpy(sender,temp);
    i=0;
    for(int k=0;k<100;k++)temp[k]='\0';
    while(mail[1][i]!=' '&&mail[1][i]!='\n')
    {
        temp[i]=mail[1][i];
        i++;
    }
    temp[i]='\0';
    if(strcmp(temp,"To:")!=0)
    {
        printf("Syntax error, To expected in second line\n");
        return 0;
    }
    while(mail[1][i]==' ')i++;
    j=0;
    for(int k=0;k<100;k++)temp[k]='\0';
    while(mail[1][i]!='\n')
    {
        temp[j]=mail[1][i];
        i++;j++;
    }
    temp[j]='\0';
    flag=checkmailsyntax(temp);
    if(!flag)
    {
        return 0;
    }
    strcpy(receivers,temp);
    i=0;
    for(int k=0;k<100;k++)temp[k]='\0';
    while(mail[2][i]!=' '&&mail[2][i]!='\n')
    {
        temp[i]=mail[2][i];
        i++;
    }
    temp[i]='\0';
    if(strcmp(temp,"Subject:")!=0)
    {
        printf("Syntax error, Subject expected in third line\n");
        return 0;
    }
    return 1;
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

void manage_mail(char *server_IP, int pop3_port, char *username, char *password)
{
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_aton(server_IP, &servaddr.sin_addr);
    servaddr.sin_port = htons(pop3_port);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket: ");
        exit(0);
    }
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect: ");
        exit(0);
    }
    char buffer[1000];
    receive(sockfd, buffer);
    printf("%s\n", buffer);
    if (strcmp(buffer, "+OK POP3 server ready\r\n") != 0)
    {
        printf("Error in connection\n");
        exit(0);
    }
    // The client must now identify and authenticate itself to the server using the USER and PASS commands

    sprintf(buffer, "USER %s\r\n", username);
    send(sockfd, buffer, strlen(buffer), 0);
    receive(sockfd, buffer);
    printf("%s\n", buffer);
    char* result[100];
    for(int i=0;i<100;i++)result[i]=(char*)malloc(100*sizeof(char));
    tokenise(buffer,result);
    if (strcmp(result[0], "+OK") != 0)
    {
        printf("Error in username\n");
        exit(0);
    }

    sprintf(buffer, "PASS %s\r\n", password);
    send(sockfd, buffer, strlen(buffer), 0);
    receive(sockfd, buffer);
    printf("%s\n", buffer);
    tokenise(buffer,result);
    if (strcmp(result[0], "+OK") != 0)
    {
        printf("Error in password\n");
        exit(0);
    }
    // The client must now request a list of all messages in the maildrop using the LIST command
    if(strcmp(buffer,"+OK empty mailbox\r\n")==0)
    {
        printf("User mailbox is empty\n");
        return;
    }
    int mail_count;
    int total_size;
    sprintf(buffer, "STAT\r\n");
    send(sockfd, buffer, strlen(buffer), 0);
    receive(sockfd, buffer);

    sscanf(buffer, "+OK %d %d", &mail_count, &total_size);
    printf("Total mail count: %d\n", mail_count);

    int line_no = 0;
    int temp = 0;
    while (temp < mail_count)
    {
        temp++;
        line_no=0;
        printf("%d ",temp);
        sprintf(buffer, "RETR %d\r\n", temp);
        send(sockfd, buffer, strlen(buffer), 0);
        receive(sockfd, buffer);
        //printf("%s", buffer);
        while (1)
        {
            //printf("%d ", (line_no + 1));
            if (line_no == 0)
            {
                receive(sockfd, buffer);
                for (int i = 6; i < strlen(buffer)-2; i++)
                {
                    printf("%c", buffer[i]);
                }
                printf(" ");
            }
            else if (line_no == 2)
            {
                receive(sockfd, buffer);
                for (int i = 10; i < strlen(buffer)-2; i++)
                {
                    printf("%c", buffer[i]);
                }
                printf(" ");
            }
            else if (line_no == 3)
            {
                receive(sockfd, buffer);
                for (int i = 8; i < strlen(buffer)-2; i++)
                {
                    printf("%c", buffer[i]);
                }
            }
            else
            {
                receive(sockfd, buffer);
                //printf("%s", buffer);
            }
            line_no++;
            if (strcmp(buffer, ".\r\n") == 0)
                break;
        }
        printf("\n");
    }
    while(1)
    {
        printf("Enter mail number to see(-1 to exit): ");
        int choice;
        scanf("%d", &choice);
        if (choice == -1)
            break;
        if(choice>mail_count||choice<=0)
        {
            printf("Mail no. out of range, give again\n");
            continue;
        }
        sprintf(buffer, "RETR %d\r\n", choice);
        send(sockfd, buffer, strlen(buffer), 0);
        
        
        //printf("Hello\n");
        int flag=0;
        while(1)
        {
            
            receive(sockfd, buffer);
            tokenise(buffer,result);
            if(strcmp(result[0],"-ERR")==0)
            {
                printf("Mail deleted\n");
                flag=1;
                break;
            }
            if(strcmp(result[0],"+OK")==0)
            {
                continue;
            }
            printf("%s", buffer);
            /*for(int i=0;i<strlen(buffer);i++)
            {
                printf("%d ",buffer[i]);
            }*/
            //printf("%d\n",strcmp(buffer,".\r\n"));
            //printf("\n");
           
            if (strcmp(buffer, ".\r\n") == 0)
                break;
        }
        if(flag)continue;

        printf("Enter 'd' to delete this mail, any other key to continue: ");
        char ch;
        scanf(" %c", &ch);
        if (ch == 'd')
        {
            sprintf(buffer, "DELE %d\r\n", choice);
            send(sockfd, buffer, strlen(buffer), 0);
            receive(sockfd, buffer);
            printf("%s\n", buffer);
        }

    }
    //Issue the QUIT command to close the connection
    sprintf(buffer, "QUIT\r\n");
    send(sockfd, buffer, strlen(buffer), 0);
    receive(sockfd, buffer);
    tokenise(buffer,result);
    if(strcmp(result[0],"-ERR")==0)
    {
        printf("Error in QUIT\n");
        return;
    }
    receive(sockfd,buffer);
    if(strcmp(buffer,"goodbye\r\n")==0)
    {
        printf("Goodbye\n");
        close(sockfd);
        return;
    }
    else
    {
        printf("Error in QUIT\n");
        close(sockfd);
        exit(0);
    }
    
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: ./mailclient <server_IP> <smtp_port> <pop3_port>\n");
        exit(0);
    }
    while (1)
    {
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        inet_aton(argv[1], &servaddr.sin_addr);

        servaddr.sin_port = htons(atoi(argv[2]));
        //int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        int pop3_port = atoi(argv[3]);

        char username[100], password[100];
        printf("Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);

        while (1)
        {
            int option;

            printf("1. Manage mail\n2. Send mail\n3. Quit\n");
            printf("Enter option: ");
            scanf("%d", &option);
            if(option==1)
            {
                manage_mail(argv[1], pop3_port, username, password);
            }
            else if(option==2)
            {
                struct sockaddr_in servaddr;
                servaddr.sin_family = AF_INET;
                inet_aton(argv[1], &servaddr.sin_addr);
                servaddr.sin_port = htons(atoi(argv[2]));
                int sockfd = socket(AF_INET,SOCK_STREAM,0);
                if(sockfd<0)
                {
                    perror("socket: ");
                    exit(0);
                } 
                if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
                {
                    perror("connect: ");
                    exit(0);
                }
                char* mail[100];
                for(int i=0;i<100;i++)mail[i]=(char*)malloc(100*sizeof(char));
                printf("Enter mail: ");
                fgets(mail[0],100,stdin);
                for(int i=0;i<100;i++)
                {
                    fgets(mail[i],100,stdin);
                    //printf("%d\n",mail[i][0]);
                    if(strcmp(mail[i],".\n")==0)break;
                }
                char sender[100];
                char receivers[100];
                /*for(int i=0;i<6;i++)
                {
                    printf("%s",mail[i]);
                }*/
                int flag=checksyntax(mail,sender,receivers);
                if(!flag)
                {
                    close(sockfd);
                    continue;
                }
                printf("Syntax correct\n");
                for(int i=0;i<100;i++)
                {
                    int j=0;
                    while(j<100&&mail[i][j]!='\n')j++;
                    if(j==100)
                    {
                        break;
                    }
                    mail[i][j]='\r';
                    mail[i][j+1]='\n';
                    j++;
                    j++;
                    while(j<100){mail[i][j]='\0';j++;}
                }
                char buffer[1000];
                for(int i=0;i<1000;i++)buffer[i]='\0';
                receive(sockfd,buffer);
                char** result=(char**)malloc(100*sizeof(char*));
                for(int i=0;i<100;i++)result[i]=(char*)malloc(100*sizeof(char));
                tokenise(buffer,result);
                if(strcmp(result[0],"220")!=0)
                {
                    printf("Error in connection\n");
                    printf("%s\n",buffer);
                    exit(0);
                }
                printf("Connection established\n");
                fflush(stdout);
                for(int i=0;i<1000;i++)buffer[i]='\0';
                char domain[100];
                for(int i=1;i<strlen(result[1])-1;i++)domain[i-1]=result[1][i];
                domain[strlen(result[1])-1]='\0';
                sprintf(buffer,"HELO %s\r\n",domain);
                send(sockfd,buffer,strlen(buffer),0);
                for(int i=0;i<1000;i++)buffer[i]='\0';
                receive(sockfd,buffer);
                printf("Received\n");
                fflush(stdout);
                tokenise(buffer,result);
                if(strcmp(result[0],"250")!=0)
                {
                    printf("Error in HELO\n");
                    printf("%s\n",result[0]);
                    exit(0);
                }
                printf("HELO successful\n");
                for(int i=0;i<1000;i++)buffer[i]='\0';
                sprintf(buffer,"MAIL FROM: <%s>\r\n",sender);
                send(sockfd,buffer,strlen(buffer),0);
                receive(sockfd,buffer);
                printf("Response received: %s\n",buffer);
                tokenise(buffer,result);
                if(strcmp(result[0],"550")==0)
                {
                    printf("User specified in FROM does not exist\n");
                    close(sockfd);
                    continue;
                }
                for(int i=0;i<1000;i++)buffer[i]='\0';
                sprintf(buffer,"RCPT TO: <%s>\r\n",receivers);
                send(sockfd,buffer,strlen(buffer),0);
                receive(sockfd,buffer);
                printf("Response received: %s",buffer);
                tokenise(buffer,result);
                if(strcmp(result[0],"550")==0)
                {
                    printf("No such user\n");
                    exit(0);
                }
                for(int i=0;i<1000;i++)buffer[i]='\0';
                sprintf(buffer,"DATA\r\n");
                send(sockfd,buffer,strlen(buffer),0);
                receive(sockfd,buffer);
                printf("Response received: %s\n",buffer);
                tokenise(buffer,result);
                for(int i=0;i<1000;i++)buffer[i]='\0';
                // sprintf(buffer,"Subject: Test\r\n\r\nHello\r\n.\r\n");
                // send(sockfd,buffer,strlen(buffer),0);
                // for(int i=0;i<1000;i++)buffer[i]='\0';
                // sprintf(buffer,".\r\n");
                // send(sockfd,buffer,strlen(buffer),0);
                // receive(sockfd,buffer);
                for(int i=0;i<100;i++)
                {
                    send(sockfd,mail[i],strlen(mail[i]),0);
                    //for(int k=0;k<strlen(mail[i]);k++)printf("%d ",mail[i][k]);
                    //printf("\n");
                    if(strcmp(mail[i],".\r\n")==0)break;
                    //printf("%d\n",i);
                }
                //printf("Hello\n");
                receive(sockfd,buffer);
                printf("Response received: %s\n",buffer);
                for(int i=0;i<1000;i++)buffer[i]='\0';
                sprintf(buffer,"QUIT\r\n");
                send(sockfd,buffer,strlen(buffer),0);
                receive(sockfd,buffer);
                printf("Response received: %s\n",buffer);
                tokenise(buffer,result);
                //for(int i=0;i<4;i++)printf("%s\n",result[i]);
                fflush(stdout);
            }
            else if(option==3)
            {
                exit(0);
            }
            else
            {
                printf("Invalid option\n");
            }
        }
    }
}
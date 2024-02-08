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
int main()
{
   
                FILE*fp=fopen("yash@coldmail.com/mymailbox","w");
                FILE*fp2=fopen("temp.txt","r");
                char temp[1000];
                while (fgets(temp, sizeof(temp), fp2) != NULL) {
                    fputs(temp, fp);
                }
                fclose(fp);
                fclose(fp2);
}
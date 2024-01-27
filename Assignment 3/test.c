#include<stdio.h>
#include<string.h>
int main()
{
    char* buffer=(char*)malloc(1000*sizeof(char));
    sprintf(buffer,"hello");
    printf("%d\n",strlen(buffer));
}
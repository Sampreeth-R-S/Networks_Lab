#include<stdio.h>
int main()
{
    FILE* fp=fopen("yash@coldmail.com/mymailbox","r");
    if(fp==NULL)
    {
        printf("Error opening file\n");
        return 0;
    }
    while(!feof(fp))
    {
        char buffer[1000];
        fgets(buffer,1000,fp);
        printf("%s",buffer);
    }
}
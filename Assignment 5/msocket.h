#define T 5


#define SOCK_MTP 100
#define ENOTBOUND 134
#define ENOMSG 135
#define ENOBUFS 136
#define MTP_SOCKET 300
#ifdef DEBUG
    #define myprintf printf
#else
    #define myprintf //
#endif
int m_bind(int, char*, int, char*, int);
int m_socket(int, int, int);
int m_sendto(int,char*,int,int,struct sockaddr_in,int);
int m_recvfrom(int sockfd,char* buffer, int len, int flags, struct sockaddr_in* cliaddr, int * clilen);
int m_close(int);

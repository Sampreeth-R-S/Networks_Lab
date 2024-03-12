#define T 5
struct sndwnd{
    int start,mid,end,next_seq_no;
};
struct rcvwnd{
    int next_expected,next_supplied;
};
struct sh{
    int free;
    int pid;
    int sockfd;
    int mtpfd;
    char sender_ip[16];
    int sender_port;
    char receiver_ip[16];
    int receiver_port;
    char sendbuf[10][1024];
    char recvbuf[5][1024];
    struct sndwnd sendwindow;
    struct rcvwnd receivewindow;
    struct tm timers[16];
    //int timerset[10];
    int next_write;
    int send_isfree[10];
    int recv_isfree[5];
    int is_empty;
};
struct sockinfo{
    int sock_id;
    char ip[16];
    int port;
    int err_no;
};

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
struct sh* shm;
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <stdbool.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id;          //902001-902020
    int AZ;          
    int BNT;         
    int Moderna;     
}registerRecord;

int handle_read(request* reqP) {
    int r;
    char buf[512];
    
    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    //open registerRecord
    file_fd = open("registerRecord",O_RDWR);
    if(file_fd < 0){
        fprintf(stderr,"no file names registerRecord");};
    //

    //set read lock and write lock in the same process
    bool readLock[21] ={false}, writeLock[21]={false};

    //
    struct timeval timeout;
    fd_set original_set, working_set;
    
    FD_ZERO(&original_set);
    FD_SET(svr.listen_fd,&original_set);

    struct flock lock;
    int w;
    while (1) {
        // TODO: Add IO multiplexing
        
        timeout.tv_sec=0;
        timeout.tv_usec = 1000;
        memcpy(&working_set,&original_set,sizeof(original_set));
        // Check new connection
        if(select(maxfd,&working_set,NULL,NULL,&timeout)<=0){
            continue;}
        if(FD_ISSET(svr.listen_fd,&working_set)){
            clilen = sizeof(cliaddr);
            //file descriptor which talk with client
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            w = write(conn_fd,"Please enter your id (to check your preference order):\n",56);
            if(w<=0)
                ERR_EXIT("something wrong when writting");
            if (conn_fd < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;  // try again
            if (errno == ENFILE) {
                (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                continue;
            }
            ERR_EXIT("accept");
            }
            FD_SET(conn_fd,&original_set);
            continue;;
        }
        else {
            conn_fd = -1;
            for(int i = 0; i < maxfd; i++) {
                if(i != svr.listen_fd && FD_ISSET(i, &working_set)){
                    conn_fd = i;
                    FD_CLR(conn_fd,&original_set);
                    break;
                }
            }
            if(conn_fd==-1)
                continue;
        }

        requestP[conn_fd].conn_fd = conn_fd;
        strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
        fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);

        int ret = handle_read(&requestP[conn_fd]); // parse data from client to requestP[conn_fd].buf
        fprintf(stderr, "ret = %d\n", ret);
	    if (ret < 0) {
            fprintf(stderr, "bad request from %s\n", requestP[conn_fd].host);
            continue;
        }
    // TODO: handle requests from clients
#ifdef READ_SERVER      
        //set lock 
        registerRecord Record;
        Record.id = atoi(requestP[conn_fd].buf);
        lock.l_type = F_RDLCK;
        lock.l_start = sizeof(registerRecord)*(Record.id-902001);
        lock.l_whence = SEEK_SET;
        lock.l_len = sizeof(registerRecord);
        if(!writeLock[requestP[conn_fd].id-902001]&& fcntl(file_fd,F_SETLK,&lock)!= -1){
            //no write lock so we could set read lock and read
            readLock[requestP[conn_fd].id-902001] = true;
            lseek(file_fd,sizeof(registerRecord)*(Record.id-902001),SEEK_SET);
            read(file_fd,&Record,sizeof(Record));
            fprintf(stderr,"id:%d Moderna:%d BNT:%d AZ:%d\n",Record.id,Record.Moderna,Record.BNT,Record.AZ);
            char rank[3][16];
            for(int i = 1 ; i < 4 ; i++){
                if(Record.Moderna == i)
                    strcpy(rank[i-1],"Moderna");
                else if (Record.BNT==i)
                    strcpy(rank[i-1],"BNT");
                else
                    strcpy(rank[i-1],"AZ");
            }
            sprintf(requestP[conn_fd].buf,"Your preference order is %s > %s > %s.\n",rank[0],rank[1],rank[2]);
            write(requestP[conn_fd].conn_fd,requestP[conn_fd].buf,strlen(requestP[conn_fd].buf));

            //unset lock
            lock.l_type= F_UNLCK;
            lock.l_start=sizeof(registerRecord)*(Record.id-902001);
            lock.l_whence = SEEK_SET;
            lock.l_len = sizeof(registerRecord);
            fcntl(file_fd,F_SETLK, &lock);
            readLock[requestP[conn_fd].id-902001] = false;
            fprintf(stderr,"finish");
        }
        else{
            sprintf(requestP[conn_fd].buf,"Locked.\n");
            write(requestP[conn_fd].conn_fd,requestP[conn_fd].buf,strlen(requestP[conn_fd].buf));
            close(requestP[conn_fd].conn_fd);
            free_request(&requestP[conn_fd]);
        }
        /***
        fprintf(stderr, "%s", requestP[conn_fd].buf);
        sprintf(buf,"%s : %s",accept_read_header,requestP[conn_fd].buf);
        write(requestP[conn_fd].conn_fd, buf, strlen(buf));   
        ***/ 
#elif defined WRITE_SERVER
        if(requestP[conn_fd].wait_for_write == 0){
            registerRecord Record;
            Record.id = atoi(requestP[conn_fd].buf);
            requestP[conn_fd].id = Record.id;
            lock.l_type = F_WRLCK;
            lock.l_start = sizeof(registerRecord)*(Record.id-902001);
            lock.l_whence = SEEK_SET;
            lock.l_len = sizeof(registerRecord);
            if(!writeLock[requestP[conn_fd].id-902001]&& fcntl(file_fd,F_SETLK,&lock)!= -1){
                //no write lock so we could set read lock and read
                readLock[requestP[conn_fd].id-902001] = true;
                lseek(file_fd,sizeof(registerRecord)*(Record.id-902001),SEEK_SET);
                read(file_fd,&Record,sizeof(Record));
                fprintf(stderr,"id:%d Moderna:%d BNT:%d AZ:%d\n",Record.id,Record.Moderna,Record.BNT,Record.AZ);
                char rank[3][16];
                for(int i = 1 ; i < 4 ; i++){
                    if(Record.Moderna == i)
                        strcpy(rank[i-1],"Moderna");
                    else if (Record.BNT==i)
                        strcpy(rank[i-1],"BNT");
                    else
                        strcpy(rank[i-1],"AZ");
                }
                sprintf(requestP[conn_fd].buf,"Your preference order is %s > %s > %s.\nPlease input your preference order respectively(AZ,BNT,Moderna):\n",rank[0],rank[1],rank[2]);
                write(requestP[conn_fd].conn_fd,requestP[conn_fd].buf,strlen(requestP[conn_fd].buf));
                //unset lock
                
                readLock[requestP[conn_fd].id-902001] = false;
                fprintf(stderr,"finish");
                requestP[conn_fd].wait_for_write = 1;
                FD_SET(conn_fd,&original_set);
                writeLock[requestP[conn_fd].id-902001] = true;
            }
            else{
                sprintf(requestP[conn_fd].buf,"Locked.\n");
                write(requestP[conn_fd].conn_fd,requestP[conn_fd].buf,strlen(requestP[conn_fd].buf));
                close(requestP[conn_fd].conn_fd);
                free_request(&requestP[conn_fd]);
            }
            continue;;
        }
        else{
            registerRecord Record;
            Record.id = requestP[conn_fd].id;
            
            
            if(1){
                lseek(file_fd,sizeof(registerRecord)*(Record.id-902001),SEEK_SET);
                char *d = " ";
                char *p = strtok(requestP[conn_fd].buf,d);


                Record.AZ = atoi(p);
                p = strtok(NULL, d);
                Record.BNT = atoi(p);
                p = strtok(NULL,d);
                Record.Moderna=atoi(p);
                char rank[3][16];
                strcpy(rank[Record.AZ-1],"AZ");
                strcpy(rank[Record.BNT-1] , "BNT");
                strcpy(rank[Record.Moderna-1] , "Moderna");
                write(file_fd,&Record,sizeof(Record));
                sprintf(requestP[conn_fd].buf,"Preference order for %d modified successed, new preference order is %s > %s > %s.\n",Record.id,rank[0],rank[1],rank[2]);
                write(requestP[conn_fd].conn_fd,requestP[conn_fd].buf,strlen(requestP[conn_fd].buf));
                lock.l_type= F_UNLCK;
                lock.l_start=sizeof(registerRecord)*(Record.id-902001);
                lock.l_whence = SEEK_SET;
                lock.l_len = sizeof(registerRecord);
                fcntl(file_fd,F_SETLK, &lock);
                fprintf(stderr,"finish writting");
                writeLock[requestP[conn_fd].id-902001] = false;
            }
            else{
                sprintf(requestP[conn_fd].buf,"Locked.\n");
                write(requestP[conn_fd].conn_fd,requestP[conn_fd].buf,strlen(requestP[conn_fd].buf));
                close(requestP[conn_fd].conn_fd);
                free_request(&requestP[conn_fd]);
            }
        }
        /***
        fprintf(stderr, "%s", requestP[conn_fd].buf);
        sprintf(buf,"%s : %s",accept_read_header,requestP[conn_fd].buf);
        write(requestP[conn_fd].conn_fd, buf, strlen(buf));   
        ***/     
#endif
        close(requestP[conn_fd].conn_fd);
        free_request(&requestP[conn_fd]);
    }
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
    reqP->wait_for_write = 0;//0 is not ready for write
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}

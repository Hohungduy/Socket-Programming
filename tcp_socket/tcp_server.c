#include <arpa/inet.h> 
#include <stdio.h>
#include <sys/socket.h> // AF_INET and SOCK_STREAM
#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE
#include <netinet/in.h> // INADDR_ANY
#include <string.h>     // bzero
#include <sys/types.h> 
#include <unistd.h>
#include <sys/time.h>
#include <errno.h> 
#include "switch.h"

#define PORT 4545
#define SIZE 1024 // Size of buffer
#define TIMEOUT 20 // Timeout for select()
#define CLIENT_NUMBER 30 //handle 30 clients

//Macro for comparing between two numbers
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define TRUE 1  //Boolean value
#define FALSE 0 //Boolean value

typedef int (*run_func_t)(int server_fd, struct sockaddr *client_address, socklen_t *addrlen, char const *argv[]);
run_func_t func_general;

int run_server_select_func(int server_fd, struct sockaddr *client_address, socklen_t *addrlen, char const *argv[]);
int run_normal_func(int server_fd, struct sockaddr *client_address, socklen_t *addrlen, char const *argv[]);


/* Open file */
FILE* open_file(const char *pathname, const char* mode)
{
    FILE *fd;
    fd = fopen(pathname,mode);
    if(fd == NULL)
    {
        perror("[-]Error opening file");
        exit(EXIT_FAILURE);
    }
    //set pointer to the beginning of the file
    fseek(fd,0,SEEK_SET);
    return fd;
}

/* ------------------------------------------------------------------*/
/* Send data and file */
int send_data(int sockfd, void *buf, int buflen)
{
    unsigned char *tmp_buf = (unsigned char *)buf;
    int byteSent = 0;
    int totByte = 0;// total byte sent
    int totleft = buflen;
    while(totleft > 0)
    {
        byteSent = send(sockfd,tmp_buf,buflen - totByte,0);
        if(byteSent == 0)
        {
            return totByte;
        }
        else if(byteSent < 0)
        {
            printf("byte send < 0 \n");
            return -1;//error
        }
        totleft -= byteSent;
        totByte += byteSent;
        tmp_buf += byteSent;
    }
    return totByte;
}
long int send_value(int sockfd, long value)
{
    value = htonl(value);
    return send_data(sockfd, &value, sizeof(value));
}
int send_file(FILE *fd, int sockfd)
{
    /* Another way to find out file length:Using fseek setting
     pointer to the end of data. Then, using ftell to find 
    current pointer and finally, using rewind() to return the beginning of the file) */ 
    char *buf = malloc(SIZE*sizeof(char));//max buf
    long buflen = SIZE;
    long filesize =0;
    long totleft = 0;//remaining
    // int i;
    long byteSend =0;

    /* Finding file size */
    fseek(fd, 0, SEEK_END);
    filesize = ftell(fd);
    rewind(fd);
    if(filesize < 0)
    {
        printf("error: filesize < 0\n");
        return -1;
    }
    printf("Size of file:%ld\n",filesize);
    if(!send_value(sockfd,filesize))
    {
        printf("error: send_value < 0\n");
        return -1;//error
    }
    totleft = filesize;
    if(filesize > 0)
    {
        while(totleft > 0)
        {
            byteSend = min(totleft,buflen);
            byteSend = fread(buf,1,byteSend,fd);
            if(byteSend < 1)
            {
                printf("error: read file des into buf (<1)\n");
                return byteSend;
            }
            byteSend = send_data(sockfd,buf,byteSend);
            if(byteSend < 1){
                printf("error: read file des into buf (<1)\n"); 
                return byteSend;
            }
            totleft -= byteSend;
        }
    }
    free(buf);
    return filesize-totleft;
}

/* ------------------------------------------------------------------*/
/* Receive file */
int receive_data(int sockfd, void *buf, int buflen)
{
    unsigned char *tmp_buf = (unsigned char *)buf;
    int byteRev = 0;
    int totByte = 0;// total byte sent
    int totleft =buflen;
    while(totleft > 0)
    {
        byteRev = recv(sockfd,tmp_buf,buflen - totByte,0);
        if(byteRev == 0)
        {
            return totByte;
        }
        else if(byteRev < 0)
        {
            return -1;//error
        }
        totleft -= byteRev;
        totByte += byteRev;
        tmp_buf += byteRev;
    }
    return totByte;
}

int receive_value(int sockfd, long *value)
{
    if(!receive_data(sockfd,value,sizeof(value)))
        return 0;
    *value = ntohl(*value);
    return 1;
}
int write_file(FILE *fd, void *buf, int buflen)
{
    unsigned char *tmp_buf = (unsigned char *)buf;
    // printf("buf:%s\n",tmp_buf);
    int byteWrite = 0;
    int totByte = 0;// total byte sent
    int totleft = buflen;
    while(totleft > 0)
    {
        // printf("byte need to write:%d\n",buflen-totByte);
        byteWrite = fwrite(tmp_buf,1,buflen-totByte,fd);
        if(byteWrite == 0)
        {
            printf("%d:error\n",__LINE__);
            return totByte;
        }
        else if(byteWrite < 0)
        {
            printf("%d:error\n",__LINE__);
            return -1;//error
        }
        totleft -= byteWrite;
        totByte += byteWrite;
        tmp_buf += byteWrite;
    }
    return totByte;
}
int receive_file(FILE *fd, int sockfd)
{
    /* Another way to find out file length:Using fseek setting
     pointer to the end of data. Then, using ftell to find 
    current pointer and finally, using rewind() to return the beginning of the file) */ 
    char *buf = malloc(SIZE*sizeof(char));//max buf
    long buflen = SIZE;
    long filesize =0;
    long totleft = 0;//remaining
    // int i;
    long byteRev=0;
    
    if(!receive_value(sockfd,&filesize))
    {
        printf("error when receving length of files\n");
        return -1;//error
    }
    
    totleft = filesize;
    printf("file len:%ld\n",filesize);

    if(filesize > 0)
    {
        while(totleft > 0)
        {
            byteRev= min(totleft,buflen);
            byteRev= receive_data(sockfd,buf,byteRev);
            if(byteRev< 1)
            {
                printf("error when receving data\n");    
                return byteRev;
            }
            byteRev= write_file(fd,buf,byteRev);
            if(byteRev< 1)
            {
                printf("error when write data to the file\n");    
                return byteRev;
            }
            totleft -= byteRev;
        }
    }
    free(buf);
    return filesize-totleft;
}

int run_server_select_func(int server_fd, struct sockaddr *client_address, socklen_t *addrlen, char const *argv[])
{
    char *message = "Hello from the server!";
    char *message2 = "Concurrent server using select() mode!";
    int num_sockfd = CLIENT_NUMBER; //30 client
    int max_sockfd = 0;
    int con_sockfd[num_sockfd];
    int byteSend =0;
    struct timeval time_out;
    fd_set writefds;
    fd_set readfds; // for future use
    int i;//index
    FILE *fd;
    int ret;
    
    char IPv4Str[INET_ADDRSTRLEN];
    //init array
    for(i =0 ; i < num_sockfd; i++)
    {
        con_sockfd[i] = 0;
    }
    while(1)
    {
        // Init list of descriptor for writing
        FD_ZERO(&readfds);
        FD_SET(server_fd,&readfds);
        max_sockfd = server_fd;
        // Init timeout
        time_out.tv_sec = TIMEOUT;
        time_out.tv_usec = 0;
        for(i=0; i < num_sockfd; i++)
        {
            if(con_sockfd[i] < 0)
                return 0;
            else if(con_sockfd[i] > 0)
                FD_SET(con_sockfd[i],&readfds);
            if (con_sockfd[i] > max_sockfd)
                max_sockfd = con_sockfd[i];
        }
        //wait for the activity on one of the sockets
        ret = select(max_sockfd + 1, &readfds, NULL, NULL,&time_out);
        if ((ret == -1) && (errno!=EINTR))
        {
            perror("error when using select()");
            exit(EXIT_FAILURE);
        }
        else if (ret == 0)
        {
            printf("timeout after %ld seconds!",time_out.tv_sec);
            return 0;
        }
        //If st happened on the master socket, then its an incoming connection
        if(FD_ISSET(server_fd, &readfds))
        {
        for(i =0 ; i < num_sockfd; i++)
            {
            if(!con_sockfd[i])
            {
                con_sockfd[i] = accept(server_fd, (struct sockaddr *)client_address,(socklen_t*)addrlen);
                if(con_sockfd[i] < 0)
                {
                    perror("[-]refuse(accept)");
                    exit(EXIT_FAILURE);
                }
                            //print information
                inet_ntop(AF_INET,&(((struct sockaddr_in *)client_address)->sin_addr),IPv4Str,INET_ADDRSTRLEN);
                printf("Client address IP:%s - port:%d\n",IPv4Str, htons(((struct sockaddr_in *)client_address)->sin_port));
                printf("socket file descriptor:%d \n\n\n",con_sockfd[i]);
                /* Send file */
                fd = open_file(argv[1],"r");
                byteSend = send_file(fd,con_sockfd[i]);
                if(byteSend <= 0)
                    return 0;
                break;
            }

                /* Send message */
                // send_data(con_sockfd[i],message,strlen(message));
                // send_data(con_sockfd[i],message2,strlen(message2));

            }
        }
    }
    return 1;
}
int run_normal_func(int server_fd, struct sockaddr *client_address, socklen_t *addrlen, char const *argv[])
{
    //Accept connection
    int con_sockfd;
    FILE *fd;
    int byteSend =0;
    char *message = "Hello from the server!";
    char *message2 = "normal mode!";

    if ((con_sockfd = accept(server_fd, (struct sockaddr *)client_address,
                             (socklen_t*)addrlen)) < 0) {
        perror("[-]refuse(accept)");
        exit(EXIT_FAILURE);
    }
    printf("[+]accept connection\n");

    /* Send file */
    // send_data(con_sockfd,message,strlen(message));
    // send_data(con_sockfd,message2,strlen(message2));

    fd = open_file(argv[1],"r");
    byteSend = send_file(fd,con_sockfd);
    if(byteSend <=0)
        return 0;
    return 1;
}
void set_func_handler(run_func_t func_handler)
{
    func_general = func_handler;
    return;
}
int run(int server_fd, struct sockaddr *client_address, socklen_t *addrlen, char const *argv[])
{
    // char *mode = argv[2]; // enter mode from terminal
    int ret = 0;
    // switchs(argv[2])
    // {   

    //     cases("poll")
    //                 /* ---------------------------------------- */
    //                 /* Sending file using poll() */
    //     cases("select")
    //                 /* ---------------------------------------- */
    //                 /* Sending file using select()*/
    //                 set_func_handler(run_server_select_func);
    //                 break;
    //     cases("multiprocess")
    //     cases("multithread")
    //     cases("normal")
    //                 /* ---------------------------------------- */
    //                 /* Sending file normal*/
    //                 set_func_handler(run_normal_func);
    //                 break;
    //     defaults
    //                 set_func_handler(run_normal_func);
    // }switchs_end;
    if(!strcmp(argv[2],"select"))
        set_func_handler(run_server_select_func);
    else 
        set_func_handler(run_normal_func);

    ret = func_general(server_fd,(struct sockaddr *)client_address,(socklen_t *)addrlen,argv);
    return ret;

}
int main(int argc, char const *argv[])
{
    int server_fd, con_sockfd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    int addrlen = sizeof(server_address);
    int opt = TRUE;  
    // Method 1: Using explicit declaration of file names
    char *filename = "test.txt";
    // Method 2: Using argument
    char IPv4Str[INET_ADDRSTRLEN];
    // FILE *fd;
    char *mode;


    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Init server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    inet_ntop(AF_INET,&(server_address.sin_addr),IPv4Str,INET_ADDRSTRLEN);
    printf("Server address IP:%s - port:%d\n",IPv4Str, htons(server_address.sin_port));
    
    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if( setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   

    // Forcefully attaching socket to the port 15150
    if (bind(server_fd, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
        perror("[-]bind failed");
        exit(EXIT_FAILURE);
    }
    printf("[+]Binding Successful\n");
    //Listen for incoming connection
    if (listen(server_fd, 3) < 0) {
        perror("[-]listen failed");
        exit(EXIT_FAILURE);
    }
    printf("[+]Listening...\n");

    //Run program

    run(server_fd,(struct sockaddr *)&client_address,(socklen_t*)&addrlen,argv);

    //print information
    inet_ntop(AF_INET,&(client_address.sin_addr),IPv4Str,INET_ADDRSTRLEN);
    printf("Client address IP:%s - port:%d\n",IPv4Str, htons(client_address.sin_port));

    close(server_fd);
    return 0;
}

// Client side C/C++ program to demonstrate Socket programming
#include <arpa/inet.h> 
#include <stdio.h>
#include <sys/socket.h> // AF_INET and SOCK_STREAM
#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE
#include <netinet/in.h> // INADDR_ANY
#include <string.h>     // bzero
#include <sys/types.h> 
#include <unistd.h>

#define PORT 4545 //set default (may be using argument)
#define LOOPBACK_ADDRESS "127.0.0.1"
#define SERVER_55_ADDRESS "172.16.32.55"
#define SIZE 1024

//Macro for comparing between two numbers
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

//Boolean value
#define TRUE 1  //Boolean value
#define FALSE 0 //Boolean value
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
    long numSend =0;

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
            numSend = min(totleft,buflen);
            numSend = fread(buf,1,numSend,fd);
            if(numSend < 1)
            {
                printf("error: read file des into buf (<1)\n");
                return numSend;
            }
            numSend = send_data(sockfd,buf,numSend);
            if(numSend < 1){
                printf("error: read file des into buf (<1)\n"); 
                return numSend;
            }
            totleft -= numSend;
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

int main(int argc, char const *argv[])
{
    // Argument 1: Using for IP Address
    // Argument 2: Using for Port
    int sockfd = 0;
    struct sockaddr_in server_address;
    FILE *fd;
    char IPv4Str[INET_ADDRSTRLEN] = LOOPBACK_ADDRESS;//loopback address
    char IPv4Str2[INET_ADDRSTRLEN] = SERVER_55_ADDRESS;//server address (using default). May be using argument
    char *buf = malloc(100 * sizeof(char));
    int byteRead;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, IPv4Str, &server_address.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    //Receive first message
    // byteRead = recv(sockfd,buf,100,0);
    // if(byteRead < 0)
    // {
    //     perror("error when receiving messages!\n");
    //     exit(EXIT_FAILURE);
    // }
    // printf("1st Message from server:\n %s\n",buf);

    // byteRead = recv(sockfd,buf,100,0);
    // printf("2nd Message from server:\n %s\n",buf);
    // if(byteRead < 0)
    // {
    //     perror("error when receiving messages!\n");
    //     exit(EXIT_FAILURE);
    // }


    fd = open_file("recv_tcptest.txt","w");
    // printf("fd:%d\n",fd);
    if((receive_file(fd,sockfd)) < 1)
    {
        printf("error when receving files\n");
        return -1;
    }

    printf("[+]write data to the file\n");
    close(sockfd);
    return 0;
}
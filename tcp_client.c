// Client side C/C++ program to demonstrate Socket programming
#include <arpa/inet.h> 
#include <stdio.h>
#include <sys/socket.h> // AF_INET and SOCK_STREAM
#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE
#include <netinet/in.h> // INADDR_ANY
#include <string.h>     // bzero
#include <sys/types.h> 
#include <unistd.h>

#define PORT 4545
#define SIZE 1024
#define min(a,b) (((a)<(b))?(a):(b))
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
int read_data(int sockfd, void *buf, int buflen)
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

int read_value(int sockfd, long *value)
{
    if(!read_data(sockfd,value,sizeof(value)))
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
    long numRev =0;
    
    if(!read_value(sockfd,&filesize))
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
            numRev = min(totleft,buflen);
            numRev = read_data(sockfd,buf,numRev);
            if(numRev < 1)
            {
                printf("error when receving data\n");    
                return numRev;
            }
            numRev = write_file(fd,buf,numRev);
            if(numRev < 1)
            {
                printf("error when write data to the file\n");    
                return numRev;
            }
            totleft -= numRev;
        }
    }
    free(buf);
    return filesize-totleft;
}

int main(int argc, char const *argv[])
{
    int sockfd = 0;
    struct sockaddr_in server_address;
    FILE *fd;
    char IPv4Str[INET_ADDRSTRLEN] = "127.0.0.1";//loopback address
    char IPv4Str2[INET_ADDRSTRLEN] = "172.16.32.55";//server address
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, IPv4Str2, &server_address.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
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
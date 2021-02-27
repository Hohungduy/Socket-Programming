// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define PORT     16505
#define MAXLINE 1024 
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
int read_data(int sockfd, void *buf, int buflen, const struct sockaddr *servaddr, int addrlen)
{
    unsigned char *tmp_buf = (unsigned char *)buf;
    int byteRev = 0;
    int totByte = 0;// total byte sent
    int totleft =buflen;
    while(totleft > 0)
    {
        byteRev = recvfrom(sockfd,(char *)tmp_buf,buflen - totByte,MSG_WAITALL, ( struct sockaddr *) &servaddr, 
                (socklen_t *)&addrlen);

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
int read_value(int sockfd, long *value, const struct sockaddr *servaddr, int addrlen)
{
    if(!read_data(sockfd,value,sizeof(value),(const struct sockaddr *) &servaddr, addrlen))
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
int receive_file(FILE *fd, int sockfd, const struct sockaddr *servaddr, int addrlen)
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
    
    if(!read_value(sockfd,&filesize,(const struct sockaddr *) &servaddr, addrlen))
    {
        printf("error when receving length of files\n");
        return -1;//error
    }
    
    totleft = filesize;
    printf("length of file:%ld\n",filesize);

    if(filesize > 0)
    {
        while(totleft > 0)
        {
            numRev = min(totleft,buflen);
            numRev = read_data(sockfd,buf,numRev,(const struct sockaddr *) &servaddr, addrlen);
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
// void write_file(int sockfd, const struct sockaddr * servaddr, int len)
// {
//     int numRev;
//     FILE *fd;
//     char *filename="recv_udptest.txt";
//     char buf[100];

//     fd = fopen(filename,"w");
//     if(fd == NULL)
//     {
//         perror("[-]Error opening file");
//         exit(EXIT_FAILURE);
//     }
//     fseek(fd,0,SEEK_SET);
//     while(1)
//     {
//         numRev = recvfrom(sockfd,(char *)buf,100,MSG_WAITALL, (struct sockaddr *) &servaddr, 
//                 (socklen_t *)&len);
 
 
//         if(numRev <= 0)
//         {
//             printf("error when send data into socket\n");
//             break;
//         }
//         fprintf(fd,"%s",buf);
//         bzero(buf,100);
//     }

// }
// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr; 
    char IPv4Str2[INET_ADDRSTRLEN] = "172.16.32.55";//server address
    FILE *fd;
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr(IPv4Str2);
      
    int len; 
    /* Send the first message*/
    sendto(sockfd, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    printf("Hello message sent.\n"); 
    /* Open file */
    fd = open_file("recv_tcptest.txt","w");
    if((receive_file(fd,sockfd,(const struct sockaddr *) &servaddr,(len = sizeof(servaddr)))) < 1)
    {
        printf("error when receving files\n");
        return -1;
    }
    // write_file(sockfd,(const struct sockaddr *) &servaddr,(len = sizeof(servaddr)));
    close(sockfd); 
    return 0; 
} 
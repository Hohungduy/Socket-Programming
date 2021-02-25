// Server side implementation of UDP client-server model 
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
    fseek(fd,0,SEEK_SET);
    return fd;
}
int send_data(int sockfd, void *buf, int buflen, const struct sockaddr *cliaddr, int addrlen)
{
    unsigned char *tmp_buf = (unsigned char *)buf;
    int byteSent = 0;
    int totByte = 0;// total byte sent
    int totleft = buflen;
    char IPv4Str[INET_ADDRSTRLEN];
    // printf("%d:buffer:%ld -address of buffer:%p\n",__LINE__,*((long *)buf),(long *)buf);
    // printf("%d:client address:%p - addrlen:%d-sockfd:%d\n",__LINE__,cliaddr,addrlen,sockfd);

    // inet_ntop(AF_INET,&(((const struct sockaddr_in *)cliaddr)->sin_addr),IPv4Str,INET_ADDRSTRLEN);
    // printf("%d:Client address IP:%s - port:%d\n",__LINE__,IPv4Str, htons(((const struct sockaddr_in *)cliaddr)->sin_port));
    while(totleft > 0)
    {
        // byteSent = send(sockfd,tmp_buf,buflen - totByte,0);//for TCP
        byteSent = sendto(sockfd,(const char *)tmp_buf,buflen - totByte,MSG_CONFIRM, (const struct sockaddr *)cliaddr, addrlen); 
        // printf("byte sent:%d\n",byteSent);
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
long int send_value(int sockfd, long value, const struct sockaddr *cliaddr, int addrlen)
{
    // printf("value:%ld\n",value);

    value = htonl(value);

    // printf("address:%p\n",&value);
    // printf("%d:client address:%p - addrlen:%d-sockfd:%d\n",__LINE__,cliaddr,addrlen,sockfd);

    return send_data(sockfd, &value, sizeof(value),(const struct sockaddr *)cliaddr, addrlen);
}
int send_file(FILE *fd, int sockfd, const struct sockaddr *cliaddr, int addrlen)
{
    char *buf = malloc(SIZE*sizeof(char));//max buf
    size_t buflen = SIZE;
    long filesize =0;
    long numSend =0;
    long totleft = 0;//remaining
    // printf("%d:client address:%p - addrlen:%d-sockfd:%d\n",__LINE__,cliaddr,addrlen,sockfd);

    /* Finding file size */
    fseek(fd, 0, SEEK_END);
    filesize = ftell(fd);
    rewind(fd);
    if(filesize < 0)
    {
        printf("error: filesize < 0\n");
        return -1;
    }
    printf("Size of File:%ld\n",filesize);
    if(!send_value(sockfd,filesize,(const struct sockaddr *)cliaddr, addrlen))
    {
        printf("error: send_value < 0\n");
        return -1;//error
    }
    totleft = filesize;
    while(totleft > 0)
    {
        numSend = min(totleft,buflen);
        /* Read data from file into user buffer */
        numSend = fread(buf,1,numSend,fd);
        printf("Print second message sent from server:\n %s (buflen:%ld)\n", buf,buflen);
        if(numSend < 1)
        {
            printf("error: read file des into buf (<1)\n");
            return numSend;
        }
        /* Send this buffer to socket*/
        numSend = send_data(sockfd,buf,numSend,(const struct sockaddr *)cliaddr, addrlen);
        if(numSend < 1){
            printf("error when send data into socket (<1)\n"); 
            return numSend;
        }
        totleft -= numSend;
    }

    free(buf);
    return filesize-totleft;
}
// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr; 
    char *filename = "test_udp.txt";
    char IPv4Str[INET_ADDRSTRLEN];
    FILE *fd;
    int numSend =0;
  
    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0) ;
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    fd = open_file(filename,"r");// maybe pass argument of program into this function

    int addrlen,n; 
  
    addrlen = sizeof(cliaddr);  //len is value/result 
    /* Receive first from client to get client address (IP, Port)*/
    printf("%d:addrlen:%d\n",__LINE__,addrlen);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                (socklen_t *)&addrlen); 
    buffer[n] = '\0'; 
    //print information
    inet_ntop(AF_INET,&(cliaddr.sin_addr),IPv4Str,INET_ADDRSTRLEN);
    printf("Client address IP:%s - port:%d\n",IPv4Str, htons(cliaddr.sin_port));
    printf("Print first message from client:\n %s\n", buffer);  
    // printf("%d:client address:%p - addrlen:%d-sockfd:%d\n",__LINE__,&cliaddr,addrlen,sockfd);
    numSend =send_file(fd,sockfd,(const struct sockaddr *)&cliaddr,addrlen);

    printf("Number of byte Sent:%d\n", numSend);
      
    return 0; 
} 
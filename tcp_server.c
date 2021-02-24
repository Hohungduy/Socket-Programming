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
    // numSend = send(sockfd,buf,buflen,0);

    free(buf);
    return filesize-totleft;
}
int main(int argc, char const *argv[])
{
    int server_fd, con_sockfd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    int addrlen = sizeof(server_address);
    //Method 1:
    char *filename = "test.txt";
    // Method 2:
    //Using argv
    char IPv4Str[INET_ADDRSTRLEN];
    FILE *fd;
    int numSend =0;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    inet_ntop(AF_INET,&(server_address.sin_addr),IPv4Str,INET_ADDRSTRLEN);
    printf("Server address IP:%s - port:%d\n",IPv4Str, htons(server_address.sin_port));

    // Forcefully attaching socket to the port 15150
    if (bind(server_fd, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
        perror("[-]bind failed");
        exit(EXIT_FAILURE);
    }
    printf("[+]Binding Successful\n");
    if (listen(server_fd, 3) < 0) {
        perror("[-]listen failed");
        exit(EXIT_FAILURE);
    }
    printf("[+]Listening...\n");

    if ((con_sockfd = accept(server_fd, (struct sockaddr *)&client_address,
                             (socklen_t*)&addrlen)) < 0) {
        perror("[-]refuse(accept)");
        exit(EXIT_FAILURE);
    }
    printf("[+]accept connection\n");
    //print information
    // inet_ntop(AF_INET,&(server_address.sin_addr.s_addr),IPv4Str,INET_ADDRSTRLEN);
    // printf("Server address IP:%s - port:%d\n",IPv4Str, htons(server_address.sin_port));
    /* Send file */
    fd = open_file(argv[1],"r");
    numSend = send_file(fd,con_sockfd);
    //print information
    inet_ntop(AF_INET,&(client_address.sin_addr),IPv4Str,INET_ADDRSTRLEN);
    printf("Client address IP:%s - port:%d\n",IPv4Str, htons(client_address.sin_port));
    close(server_fd);
    return 0;
}

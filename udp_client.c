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

void write_file(int sockfd, const struct sockaddr * servaddr, int len)
{
    int numRev;
    FILE *fp;
    char *filename="recv_udptest.txt";
    char buf[100];

    fp = fopen(filename,"w");
    if(fp == NULL)
    {
        perror("[-]Error opening file");
        exit(EXIT_FAILURE);
    }
    fseek(fp,0,SEEK_SET);
    while(1)
    {
        numRev = recvfrom(sockfd,(char *)buf,100,MSG_WAITALL, (struct sockaddr *) &servaddr, 
                (socklen_t *)&len);
 
 
        if(numRev <= 0)
        {
            printf("error when send data into socket\n");
            break;
        }
        fprintf(fp,"%s",buf);
        bzero(buf,100);
    }

}
// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr("172.16.32.55");
      
    int len; 
      
    sendto(sockfd, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    printf("Hello message sent.\n"); 

    write_file(sockfd,(const struct sockaddr *) &servaddr,(len = sizeof(servaddr)));
    close(sockfd); 
    return 0; 
} 
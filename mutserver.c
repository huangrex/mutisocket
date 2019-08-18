#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <pthread.h> 
#include <stdbool.h> //using bool to receive data
#define BUFFSIZE 256 //every data gram is BUFFSIZE +　HEADERSIZE
#define HEADERSIZE 10
//argv1 is group ip 224.0.0.0~239.255.255.255
//argv2 is local ip(use "ifconfig" to check)，
//argv3 is port number
//argv4 is file name
int main(int argc, char*argv[]){
    //declare the socket
    int sockfd;
    char sendbuff[BUFFSIZE+HEADERSIZE], recbuff[BUFFSIZE+10];
    struct sockaddr_in server_addr;
    //set port number and turn to int 
    int portnu = atoi(argv[3]);
    
    //build socket
    //SOCK_DGRAM is udp
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("failed on socket");
        exit(0);
    }
    //set group datagram 
    server_addr.sin_family = AF_INET; //IPV4
    server_addr.sin_port = htons(portnu);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]); //inet_addr is to change ip address to the binary
    //bind 
    //set local datagram outbound
    struct in_addr localinterface;
    //set the local ip to local  interface
    localinterface.s_addr = inet_addr(argv[2]);
    //setting local interface 
    if(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localinterface, sizeof(localinterface)) < 0){
        perror("Setting local interface error");
        exit(1);
    }
    else printf("Setting the local interface...OK\n");

    //send file name
    //int w1 = write(sockfd, argv[4], strlen(argv[4]));
    int w1 = sendto(sockfd, argv[4], strlen(argv[4]), 0, (struct sockaddr*)&server_addr, sizeof(server_addr) );
    if(w1 < 0){
        perror("failed to send file name\n");
        exit(1);
    } 
    
    //send file size
    FILE *in;
    in = fopen(argv[4], "rb");
    struct stat s;
    stat(argv[4], &s);
    int sz = s.st_size;
    int w2 = sendto(sockfd, (char*)&sz, sizeof(sz), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(w2 < 0){
        perror("failed on sending file size\n");
        exit(1);
    }
   
    //write(sockfd, (char*)&sz, sizeof(sz) );
    //send file
    int check = 0; // the datagram size
    int count  = 0;
    int itmp;
    char ctmp;
    bool bits[(BUFFSIZE+HEADERSIZE)*12]; //the sent data
    bool bitp[12]; //to encode the hamming code
    int k = 0; //to count the position in  8bit hamming code
    while(1){
        memset(bits, 0, sizeof(bits));
        memset(sendbuff, 0, sizeof(sendbuff));
        //scan the file content to sendbuff and avoid the header index place 
        check = fread(sendbuff+HEADERSIZE, 1, BUFFSIZE, in); //check is the number of fread size
        //count the numbers of datagram
        count++;
        //convert the number(sequenece) to 128進位
        itmp = count;
        //printf("count : %d\n", count);
        //實現128進位
        for(int i = 0; i < HEADERSIZE; i++) {
            ctmp = itmp % 128;
            sendbuff[i] = ctmp;
            itmp /= 128;
            //printf("%d\n", ctmp);
        }
        
        
        //hamming code
        //change bit pattern
        int k = 0;
        unsigned int mask = 0;
        for(int i = 0; i < check+HEADERSIZE; i++){
            //convert char to bit pattern and store in bitp array
            for(int j = 0; j < 8; j++){
                bitp[j] = sendbuff[i] & (0x01 << j);
            }
            //set bit pattern exclude 2^r
            for(int j = 0; j < 12; j++){
                if(j != 0 && j != 1 && j != 3 && j != 7){
                    bits[i*12+j] = bitp[k];
                    k++;
                }
            }
            //k should be 8
            //calculate the index 0 1 3 7 (2^0, 2^1, 2^2 ,2^3)
            bits[i*12+0] = bits[i*12+2] ^ bits[i*12+4] ^ bits[i*12+6] ^ bits[i*12+8] ^ bits[i*12+10];
            bits[i*12+1] = bits[i*12+2] ^ bits[i*12+5] ^ bits[i*12+6] ^ bits[i*12+9] ^ bits[i*12+10];
            bits[i*12+3] = bits[i*12+4] ^ bits[i*12+5] ^ bits[i*12+6] ^ bits[i*12+11];
            bits[i*12+7] = bits[i*12+8] ^ bits[i*12+9] ^ bits[i*12+10] ^ bits[i*12+11];
            k = 0;//set k to 0
        }

        //printf("%d\n", sizeof(bits));
        //total send is (check+HEADERSIZE)*12 s
        int w3 = sendto(sockfd, bits, (check+HEADERSIZE)*12, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
        if(w3 < 0){
            perror("failed on sending datagram\n");
        }
        if(check == 0)break;
    }
    printf("mission complete\n");
    close(sockfd);
    fclose(in);
    return 0;
}
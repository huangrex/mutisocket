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
#include <stdbool.h> //using bool to send data
#define BUFFSIZE 256 //every segement from file
#define HEADERSIZE 10 //the head to count the sequence
//argv1 is group address
//argv2 is client address
//argv3 is port number
int main(int argc, char *argv[]){
    //declare all basic information
    int sockfd, recfd;
    char sendbuff[BUFFSIZE];
    unsigned char recbuff[BUFFSIZE + HEADERSIZE];
    struct sockaddr_in client_addr, server_addr;
    //set port number , conver char to integer
    int portnu = atoi(argv[3]);
    //set socket
    //SOCK_DGRAM is udp
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("failed to open socket");
        exit(0);
    }
    else printf("opening datagram socket........OK\n");

	
	int reuse = 1;
    //SOL_SOCKET is the option name
    //SO_REUSEADDR allowed the port unused to be used(not listening)
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		perror("Setting SO_REUSEADDR error");
		close(sockfd);
		exit(1);
	}
	else printf("Setting SO_REUSEADDR...OK.\n");
    
    memset((char*)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portnu);
   
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("failed on binding");
        exit(0);
    }
    //set group
    struct ip_mreq group;
    group.imr_multiaddr.s_addr = inet_addr(argv[1]); //group ip
    group.imr_interface.s_addr = inet_addr(argv[2]); //local ip
    //setsockopt & check
    if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0){
        perror("fail on adding group member\n");
        close(sockfd);
        exit(1);
    } else printf("success in adding group member\n");

    //data name transfer
    socklen_t serlen;
    serlen = sizeof(server_addr); 
    char name[BUFFSIZE];
    memset(name, 0 , sizeof(name));
    //get the file name
    int r1 = read(sockfd, name, BUFFSIZE);
    if(r1 < 0){
        perror("failed on receive file name");
        exit(0);  
    } else printf("file name: %s\n", name);
    //open outputfile
    FILE *out;
    out = fopen(name, "ab");
    int sz;
    //get the  file size
    int r2 = read(sockfd, (char*)&sz, sizeof(sz));
    if(r2 < 0){
        perror("failed on receive data size");
        exit(0);
    } else printf("file size : %d\n", sz);
    int count = 0, check = 0;
    //receive datagram number
    int reccount = 0;
    char ctmp;
    int k = 1;
    bool bits[(BUFFSIZE+HEADERSIZE)*12]; //receive hamming code bits
    bool bitf[(BUFFSIZE+HEADERSIZE)*8];  // the real data
    int sum = 0;
    int sz2 = sz; //to count the size to write in file
    while(1){
        memset(bits, 0, sizeof(bits));
        memset(bitf, 0, sizeof(bitf));
        reccount = 0;
        k = 1; //128進位的輔助數
        sum = 0;
        memset(recbuff, 0, sizeof(recbuff));
        check = read(sockfd, (char*)bits, sizeof(bits));
        
        //hamming code
        for(int i = 0; i < (BUFFSIZE+HEADERSIZE); i++){
            //error correction , calculate the even parity
            sum = 0;
            if(bits[i*12] != bits[i*12+2] ^ bits[i*12+4] ^ bits[i*12+6] ^ bits[i*12+8] ^ bits[i*12+10]){
                sum += 1;
            }
            if(bits[i*12+1] != bits[i*12+2] ^ bits[i*12+5] ^ bits[i*12+6] ^ bits[i*12+9] ^ bits[i*12+10]){
                sum += 2;
            }
            if(bits[i*12+3] != bits[i*12+4] ^ bits[i*12+5] ^ bits[i*12+6] ^ bits[i*12+11]){
                sum += 4;
            }
            if(bits[i*12+7] != bits[i*12+8] ^ bits[i*12+9] ^ bits[i*12+10] ^ bits[i*12+11]){
                sum += 8;
            }
            //error correction
            if(sum)bits[i*12+sum-1] ^= 0x01;
            //delete redundancy 12->8
            int t = 0;
            for(int j = 0; j < 12; j++){
                if(j != 0 && j != 1 && j != 3 && j != 7) {
                    bitf[i*8+t] = bits[i*12+j];
                    t++;
                }
            }
            
            
        }
        
        //convert bit pattern to char
        for(int i = 0; i < BUFFSIZE+HEADERSIZE; i++){
            for(int j = 0; j < 8; j++){
                if(bitf[i*8+j]){
                    recbuff[i] |= (0x01<<j);
                }
            }
        }

        count++; // count the numbers of received data
        //printf("%d\n", check);
        for(int i = 0; i < HEADERSIZE; i++) {
            ctmp = recbuff[i];
            reccount += ctmp * k;//reccount is the number of data(sequence)
            k *= 128; //128進位實現
            //printf("%d\n", ctmp);
        }
        
               
        //data count size //sz>0
        if(check/12-HEADERSIZE) {
            fwrite(recbuff+HEADERSIZE, sizeof(char), check/12-HEADERSIZE, out);
            //printf("hello: %d\n", check/12-HEADERSIZE);
        } 
        sz2-=(check/12-HEADERSIZE); 
        //numbers of datagram greater than stop
        //if no data than the check will be the HEADERSIZE ,change to the hamming code will *12
        if(check == 12*HEADERSIZE)break;
    }
    //printf("reccount: %d\n count: %d \n", reccount ,count);
    printf("Packet loss rate %d %%\n", (reccount-count)*100 / reccount);
    printf("mission completed\n");
    close(sockfd); //close socket
    fclose(out);   //close file
    return 0;
}
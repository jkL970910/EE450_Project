/* UDP client in the internet domain */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <tr1/unordered_map>
#include <sys/wait.h>
#include <iostream>
#include <string.h>
#include <string>
using namespace std;

#define TCP_PORT_NUM 33991
#define UDP_PORT_NUM 32991
#define SERVERA_PORT_NUM 30991
#define HOSTNAME "127.0.0.1"

// Communicate with hospitalA
class HospitalServer {
    private: 
    int sockfd, port_num;
    struct sockaddr_in remote_addr, local_addr;
    socklen_t sockaddr_in_length;
    char buffer[256];

    void Error(char* msg) {
        perror(msg);
        exit(0);
    }

    public:
    string bufferMessage() {
        // convert the char to string
        string sb = buffer;
        return sb;
    }

    // block the UDP send/receive function with the hospital
    void send() {
        int res = sendto(sockfd, buffer, 256, 0, (sockaddr*) &remote_addr, sockaddr_in_length);
        if (res < 0) {
            Error((char*)"sendto");
        }
    }

    void send(char* message) {
        int res = sendto(sockfd, message, 256, 0, (sockaddr*) &remote_addr, sockaddr_in_length);
        if (res < 0) {
            Error((char*)"sendto");
        }
    }

    void receive() {   
        bzero(&buffer,256);
        int res = recvfrom(sockfd, buffer, 256, 0, (sockaddr*)&remote_addr, &sockaddr_in_length);
        if (res < 0) {
            Error((char*)"recvfrom");
        }

    }

    void connnect(int port_num) {
        this -> port_num = port_num;

        // remote address for each hospital servers
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port_num);
        remote_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        // local address
        local_addr.sin_family = AF_INET;
	    local_addr.sin_port = htons(UDP_PORT_NUM);
	    local_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0) {
            Error((char*)"socket");
        }

        sockaddr_in_length = sizeof(struct sockaddr_in);
        bind(sockfd, (struct sockaddr*) & local_addr, sockaddr_in_length); // set Scheduler port number as 32XXX

        bzero(buffer, 256);

        // this sentence has no real function, only because UDP server has to boot up first and unfreeze in while(1)
        send();

        receive(); // get responsible 
    }
};

static HospitalServer hospitalA;

// Communicate with client
class Client {
    private:
    int sockfd, newsockfd, port_num;
    struct sockaddr_in remote_addr, local_addr;
    socklen_t remote_sockaddr_length, local_sockaddr_length;

    void Error(char *msg) {
        perror(msg);
        exit(0);
    }
    
    void HandleRequest (int sock) {

        char buffer[256];
        char copy[256];
        bzero(buffer,256);
        bzero(copy,256);
        int res = read(sock, buffer, 255);
        if (res < 0) {
            Error((char*)"ERROR reading from socket");
        }
        strcpy(copy, buffer);
        bzero(buffer,256);
        strcpy(buffer, copy);
        res = write(sock, buffer, 256);
        if (res < 0) {
            Error((char*)"ERROR writing to socket");
        }
        fprintf(stderr, "Client has sent Message<%s> to Scheduler using TCP\n", buffer);
    }

    public:
    void connectClient() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            Error((char*)"ERROR opening socket");
        }

        local_sockaddr_length = sizeof(local_addr);
        bzero((char *) &local_addr, local_sockaddr_length);

        port_num = TCP_PORT_NUM;
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
        local_addr.sin_port = htons(port_num);

        local_sockaddr_length = sizeof(local_addr);
        int res = bind(sockfd, (struct sockaddr *) &local_addr, local_sockaddr_length);
        if (res < 0) {
            Error((char*)"ERROR on binding");
        }

        // start listening to client socket
        listen(sockfd, 5);

        remote_sockaddr_length = sizeof(remote_addr);
        
        newsockfd = accept(sockfd, (struct sockaddr *) &remote_addr, &remote_sockaddr_length);
        if (newsockfd < 0) {
            Error((char*)"ERROR on accept");
        }

        while(1) HandleRequest(newsockfd);
        
        exit(0);
    }
};

int main() {
    fprintf(stderr, "The Scheduler is up and running.\n");
    
    hospitalA.connnect(SERVERA_PORT_NUM);
    fprintf(stderr, "The Scheduler is connecting to HospitalA.\n");
    // serverA.MessageToHospitalMap();
    
    Client client;
    client.connectClient();
}
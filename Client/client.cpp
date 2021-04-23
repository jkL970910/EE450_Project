#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <iostream>
using namespace std;

#define TCP_PORT_NUM 34864
#define HOSTNAME "127.0.0.1"

// Referenced TCP model at "Beej's Guide to Network Programming Using Internet Sockets"
class SchedulerMain {
    private:
    int sockfd, port_num; // sockfd == sock file descriptor
    struct sockaddr_in remote_addr;
    socklen_t remote_sockaddr_length;

    char buffer[256];
    int client_id;

    // print error message relating to socket usage
    void Error(char *msg) {
        perror(msg);
        exit(0);
    }

    public:
    void connectServer() {
        // set remote port number
        port_num = TCP_PORT_NUM;

        // create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            Error((char*)"Error opening socket");
        }

        // set schedulermain address
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port_num);
        remote_addr.sin_addr.s_addr = inet_addr(HOSTNAME); 

        remote_sockaddr_length = sizeof(remote_addr);
        int res = connect(sockfd, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
        if(res < 0) {
            Error((char*)"ERROR connecting");
        }

        // on-screen message 1
        fprintf(stderr, "The client is up and running\n");
    }

    void queryHospital(string location) {
        string message;
        message += location;

        bzero(buffer,256);    
        strcpy(buffer, message.c_str());

        // send query to schedulermain
        int res = write(sockfd, buffer, 256);

        if (res < 0) {
            Error((char*)"ERROR writing to socket");
        }

        // on-screen message 2
        fprintf(stderr, "The client has sent query to Scheduler using TCP: client location %s\n", location.c_str());

        // get result from schedulermain
        bzero(buffer, 256);
        res = read(sockfd, buffer, 256);
        if (res < 0) {
            Error((char*)"ERROR reading from socket");
        }

        // on-screen message 3 / errors
        int result = atoi(buffer);
        switch(result) {
            case -2: 
	           fprintf(stderr, "The client has received results from the Scheduler: assigned to Hospital None\n"); 
	           fprintf(stderr, "Location %s not found\n", message.c_str()); break;
            case -1: 
	           fprintf(stderr, "The client has received results from the Scheduler: assigned to Hospital None\n"); 
	           fprintf(stderr, "Score = None, No assignment\n"); break;
            case 0: fprintf(stderr, "The client has received results from the Scheduler: assigned to Hospital A\n"); break;
            case 1: fprintf(stderr, "The client has received results from the Scheduler: assigned to Hospital B\n"); break;
            case 2: fprintf(stderr, "The client has received results from the Scheduler: assigned to Hospital C\n"); break;
        }
    }
};

int main(int argc, char* argv[]) {
    SchedulerMain schedulermain;
    if (argc < 2) {
        fprintf(stderr, "please enter the location\n");
        exit(0);
    } else if (argv[1] < 0) {
        fprintf(stderr, "please enter a valid location\n");
        exit(0);
    }
    string location = argv[1];

    // bind socket and connect to Scheduler
    schedulermain.connectServer();

    // start query for the input client location
    schedulermain.queryHospital(location);
}
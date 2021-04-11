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

#define TCP_PORT_NUM 34864
#define UDP_PORT_NUM 33864
#define HOSPITALA_PORT_NUM 30864
#define HOSPITALB_PORT_NUM 31864
#define HOSPITALC_PORT_NUM 32864
#define HOSTNAME "127.0.0.1"

// Communicate with hospitalA
class HospitalServer {
    private: 
    int sockfd, port_num;
    struct sockaddr_in remote_addr, local_addr;
    socklen_t sockaddr_in_length;
    char buffer[256];
    // using a 2-D array to store the basic info of each hospital
    // 0 ~ 1 stands for hospitalA ~ C, [][0] - capacity, [][1] - occupancy
    int hospitals[3][2]; 

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

    void connnect(int port_num, int count) {
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

        getInitHospital(count);
    }

    void getInitHospital(int count) {
        
        bzero(buffer, 256);
        fprintf(stderr, "scheduler is listening to hospitalA\n");
        receive(); // get responsible 
        
        int i = 0;
        int j = 0;
        string str;
        while (buffer[i] != '\0') {
            while (buffer[i] == ' ') {
                i++;
            }
            while (buffer[i] != ' ' && buffer[i] != '\0') {
                str = str + buffer[i++];
            }
            
            hospitals[count][j++] = atoi(str.c_str());
            str = "";
        }
        switch(count) {
            case 0: 
                fprintf(stderr, "the scheduler has initialized hospitalA using UDP through port: %d\n", HOSPITALA_PORT_NUM); 
                fprintf(stderr, "the capacity of HospitalA is: %d, the occupancy of HospitalA is: %d\n", hospitals[count][0], hospitals[count][1]);
                break;
            case 1: 
                fprintf(stderr, "the scheduler has initialized hospitalB using UDP through port: %d\n", HOSPITALB_PORT_NUM); 
                fprintf(stderr, "the capacity of HospitalA is: %d, the occupancy of HospitalA is: %d\n", hospitals[count][0], hospitals[count][1]);
                break;
            case 2: 
                fprintf(stderr, "the scheduler has initialized hospitalC using UDP through port: %d\n", HOSPITALC_PORT_NUM); 
                fprintf(stderr, "the capacity of HospitalA is: %d, the occupancy of HospitalA is: %d\n", hospitals[count][0], hospitals[count][1]);
                break;
        }
    }
};

static HospitalServer hospitalA;

// Communicate with client
class Client {
    private:
    int sockfd, newsockfd, port_num;
    struct sockaddr_in remote_addr, local_addr;
    socklen_t remote_sockaddr_length, local_sockaddr_length;
    char buffer[256];
    char copy[256];

    void Error(char *msg) {
        perror(msg);
        exit(0);
    }

    void readFromClient(int sock) {
        bzero(buffer,256);
        int res = read(sock, buffer, 256);
        if (res < 0) {
            Error((char*)"ERROR reading from socket");
        }
    }

    void sendToClient(int sock, string message) {
        bzero(buffer,256);
        strcpy(buffer, message.c_str());
        int res = write(sock, buffer, 256);
        if (res < 0) {
            Error((char*)"ERROR writing to socket");
        }
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
        fprintf(stderr, "scheduler is listening to client through TCP at port: %d\n", TCP_PORT_NUM);
        listen(sockfd, 5);

        remote_sockaddr_length = sizeof(remote_addr);
        
        newsockfd = accept(sockfd, (struct sockaddr *) &remote_addr, &remote_sockaddr_length);
        if (newsockfd < 0) {
            Error((char*)"ERROR on accept");
        }
        fprintf(stderr, "scheduler is connection to client througth TCP at port: %d\n", TCP_PORT_NUM);
    }

    int getClientLocation() {
        readFromClient(newsockfd);
        return atoi(buffer);
    }

    void send(string message) {
        sendToClient(newsockfd, message);
    }
};

static Client client;

int main() {
    fprintf(stderr, "The Scheduler is up and running.\n");
    
    hospitalA.connnect(HOSPITALA_PORT_NUM, 0);

    // connect to TCP and get reply from hospitals
    client.connectClient();
    
    while(1) {
        int location = client.getClientLocation();
        cout << location << endl;
        client.send(to_string(location));
    }
}
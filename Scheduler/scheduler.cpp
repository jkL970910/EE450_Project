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

static struct sockaddr_in local_addr;
static int sockfd;
static socklen_t sockaddr_in_length;
// using a 2-D array to store the basic info of each hospital
// 0 ~ 1 stands for hospitalA ~ C, [][0] - capacity, [][1] - occupancy
static int hospitals[3][2];

void Error(char* msg) {
    perror(msg);
    exit(0);
}

// set the local_UDP_addr of the scheduler
void bind() {
    // local address
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(UDP_PORT_NUM);
    local_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        Error((char*)"socket");
    }

    sockaddr_in_length = sizeof(struct sockaddr_in);
    bind(sockfd, (struct sockaddr*) & local_addr, sockaddr_in_length);
}



// Communicate with hospitals
class HospitalServer {
    private: 
    int port_num;
    struct sockaddr_in remote_addr;
    char buffer[256];

    public:
    // block the UDP send/receive function with the hospital
    void send() {
        int res = sendto(sockfd, buffer, 256, 0, (sockaddr*) &remote_addr, sockaddr_in_length);
        if (res < 0) {
            Error((char*)"sendto");
        }
    }

    void send(string message) {
        char hospital_message[message.length()];
        strcpy(hospital_message, message.c_str());

        int res = sendto(sockfd, hospital_message, 256, 0, (sockaddr*) &remote_addr, sockaddr_in_length);
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

    void connnectHospital(int port_num) {
        // remote address for each hospital servers
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port_num);
        remote_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
    }

    void getInitHospital(int port_num, int count) {
        connnectHospital(port_num);
        
        bzero(buffer, 256);
        receive();
        
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

    float getScore() {
        bzero(buffer, 256);
        receive();

        return atof(buffer);
    }
};

static HospitalServer hospitalA, hospitalB, hospitalC;

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
    void bindClient() {
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
    }

    void connectClient() {
        // start listening to client socket
        fprintf(stderr, "scheduler is listening to client through TCP at port: %d\n", TCP_PORT_NUM);
        listen(sockfd, 5);

        remote_sockaddr_length = sizeof(remote_addr);
        
        newsockfd = accept(sockfd, (struct sockaddr *) &remote_addr, &remote_sockaddr_length);
        if (newsockfd < 0) {
            Error((char*)"ERROR on accept");
        }
        fprintf(stderr, "scheduler is connecting to client througth TCP at port: %d\n", TCP_PORT_NUM);
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
    
    // build up UDP port
    bind();
    fprintf(stderr, "scheduler is listening to hospitals through UDP at port: %d\n", UDP_PORT_NUM);

    // get initial avaliability of hospitals
    hospitalA.getInitHospital(HOSPITALA_PORT_NUM, 0);
    hospitalB.getInitHospital(HOSPITALB_PORT_NUM, 1);
    hospitalC.getInitHospital(HOSPITALC_PORT_NUM, 2);
    
    // connect to TCP and receive the location from the client
    client.bindClient();
    
    while(1) {
        client.connectClient();
        int location = client.getClientLocation();
        fprintf(stderr, "the scheduler get the client location %d and start quering for the hospitals\n", location);

        // receive the score from the hospitals 
        hospitalA.send(to_string(location));
        float scoreA = hospitalA.getScore();
        fprintf(stderr, "the score of hospitalA is: %g\n", scoreA);
        
        hospitalB.send(to_string(location));
        float scoreB = hospitalB.getScore();
        fprintf(stderr, "the score of hospitalB is: %g\n", scoreB);
        
        hospitalC.send(to_string(location));
        float scoreC = hospitalC.getScore();
        fprintf(stderr, "the score of hospitalC is: %g\n", scoreC);

        // TODO: determine which hospital to be selected
        // int selectedLocation = selectHospital(scoreA, scoreB, scoreC);

        // TODO: send the update info to the hospitals
        
        // send the selected hospital to the client
        
        client.send(to_string(location));
    }
}
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
// using a 2-D array to store the initial info of each hospital
// 0 ~ 2 stands for hospitalA ~ C, [][0] - capacity, [][1] - occupancy
static int hospitals[3][2];

// using a 2-D array to store the score and distance of each hospital
// 0 ~ 2 stands for hospitalA ~ C, [][0] - score, [][1] - distance
static float hospitalsScore[3][2];

// using a flag to determine whether to communicate with hospital or not
static bool flagA;
static bool flagB;
static bool flagC;

void Error(char* msg) {
    perror(msg);
    exit(0);
}

// set the local_UDP_addr of the scheduler
// Referenced UDP model at "Beej's Guide to Network Programming Using Internet Sockets"
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

    // send message via UDP
    void send(string message) {
        char hospital_message[message.length()];
        strcpy(hospital_message, message.c_str());

        int res = sendto(sockfd, hospital_message, 256, 0, (sockaddr*) &remote_addr, sockaddr_in_length);
        if (res < 0) {
            Error((char*)"sendto");
        }
    }

    // receive message via UDP
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

        // on-screen message 2
        switch(count) {
            case 0: 
                fprintf(stderr, "The Scheduler has received information from Hospital A: total capacity is %d and initial occupancy is %d\n", hospitals[count][0], hospitals[count][1]);
                break;
            case 1: 
                fprintf(stderr, "The Scheduler has received information from Hospital B: total capacity is %d and initial occupancy is %d\n", hospitals[count][0], hospitals[count][1]);
                break;
            case 2: 
                fprintf(stderr, "The Scheduler has received information from Hospital C: total capacity is %d and initial occupancy is %d\n", hospitals[count][0], hospitals[count][1]);
                break;
        }
    }

    void getScore(int count) {
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
            
            hospitalsScore[count][j++] = atof(str.c_str());
            str = "";
        }
    }
};

static HospitalServer hospitalA, hospitalB, hospitalC;

// on-screen message 4/5
void getHospitalsScore(int location) {
    // first check the current availability of each hospital, then receive the score and distance for avaiable hospitals
    if (hospitals[0][0] > hospitals[0][1]) {
        flagA = true;
        hospitalA.send(to_string(location));
        fprintf(stderr, "The Scheduler has sent client location to Hospital A using UDP over port %d\n", HOSPITALA_PORT_NUM);
        hospitalA.getScore(0);
        if (hospitalsScore[0][0] > 0 && hospitalsScore[0][1] > 0) {
            fprintf(stderr, "The Scheduler has received map information from Hospital A, the score = %g and the distance = %g\n", hospitalsScore[0][0], hospitalsScore[0][1]);
        } else {
            fprintf(stderr, "The Scheduler has received map information from Hospital A, the score = None and the distance = None\n");
        }
    } else {
        flagA = false;
        hospitalsScore[0][0] = -1;
    }
    
    if (hospitals[1][0] > hospitals[1][1]) {
        flagB = true;
        hospitalB.send(to_string(location));
        fprintf(stderr, "The Scheduler has sent client location to Hospital B using UDP over port %d\n", HOSPITALB_PORT_NUM);
        hospitalB.getScore(1);
        if (hospitalsScore[1][0] > 0 && hospitalsScore[1][1] > 0) {
            fprintf(stderr, "The Scheduler has received map information from Hospital B, the score = %g and the distance = %g\n", hospitalsScore[1][0], hospitalsScore[1][1]); 
        } else {
            fprintf(stderr, "The Scheduler has received map information from Hospital B, the score = None and the distance = None\n");
        }
    } else {
        flagB = false;
        hospitalsScore[1][0] = -1;
    }
    
    if (hospitals[2][0] > hospitals[2][1]) {
        flagC = true;
        hospitalC.send(to_string(location));
        fprintf(stderr, "The Scheduler has sent client location to Hospital C using UDP over port %d\n", HOSPITALC_PORT_NUM);
        hospitalC.getScore(2);
        if (hospitalsScore[2][0] > 0 && hospitalsScore[2][1] > 0) {
            fprintf(stderr, "The Scheduler has received map information from Hospital C, the score = %g and the distance = %g\n", hospitalsScore[2][0], hospitalsScore[2][1]);
        } else {
            fprintf(stderr, "The Scheduler has received map information from Hospital C, the score = None and the distance = None\n");
        }
    } else {
        flagC = false;
        hospitalsScore[2][0] = -1;
    }
}

// selected the hospital with the highest score
int selectHospital() {
    float highScore = max(hospitalsScore[0][0], hospitalsScore[1][0]);
    highScore = max(highScore, hospitalsScore[2][0]);
    if (highScore == -1) return -1;
    
    int count = 0;
    int score = 0;
    for (int i = 0; i < 3; i++) {
        if (highScore == hospitalsScore[i][0]) count = count + 1;
    }
    // check if there are multiple tied scores
    if (count == 1) {
        if (highScore == hospitalsScore[0][0]) score = 0;
        if (highScore == hospitalsScore[1][0]) score = 1;
        if (highScore == hospitalsScore[2][0]) score = 2;
    } else {
        // 1 + 2 = 3, 1 + 3 = 4, 2 + 3 = 5, 1 + 2 + 3 = 6 --> all the multiple cases
        if (highScore == hospitalsScore[0][0]) score = score + 1;
        if (highScore == hospitalsScore[1][0]) score = score + 2;
        if (highScore == hospitalsScore[2][0]) score = score + 3;
    }
    return score;
}

// selected the hospital with the shortest distance
// 0 -> hospital A, 1 -> B, 2 -> C, 3 -> A & B, 4 -> A & C, 5 -> B & C, 6 -> A & B & C
int chooseHospital(int selectedLocation) {
    int result;
    switch(selectedLocation) {
        case -1: result = -1; break;
        case 0: result = 0; break;
        case 1: result = 1; break;
        case 2: result = 2; break;
        // compare the distances between those tied hospitals
        case 3: result = hospitalsScore[0][1] > hospitalsScore[1][1] ? 1 : 0; break;
        case 4: result = hospitalsScore[0][1] > hospitalsScore[2][1] ? 2 : 0; break;
        case 5: result = hospitalsScore[1][1] > hospitalsScore[2][1] ? 2 : 1; break;
        default: {
            float minDistance = min(hospitalsScore[0][1], hospitalsScore[1][1]);
            minDistance = min(minDistance, hospitalsScore[2][1]);
            // if there are tied distances, we choose the last compared one.
            if (minDistance == hospitalsScore[0][1]) result = 0;
            if (minDistance == hospitalsScore[1][1]) result = 1;
            if (minDistance == hospitalsScore[2][1]) result = 2;
        }
    }
    return result;
};

void messageConfirm(int selectedResult) {
    if (selectedResult == -1 || selectedResult == -2) fprintf(stderr, "The Scheduler has assigned None to the client\n");
    if (selectedResult == 0) fprintf(stderr, "The Scheduler has assigned Hospital A to the client\n");
    if (selectedResult == 1) fprintf(stderr, "The Scheduler has assigned Hospital B to the client\n");
    if (selectedResult == 2) fprintf(stderr, "The Scheduler has assigned Hospital C to the client\n");
};

void updateHospital(int selectedResult) {
    if (flagA) hospitalA.send(to_string(selectedResult));
    if (flagB) hospitalB.send(to_string(selectedResult));
    if (flagC) hospitalC.send(to_string(selectedResult));
    // on-screen message 8
    if (selectedResult == 0) fprintf(stderr, "The Scheduler has sent the result to Hospital A using UDP over port %d\n", UDP_PORT_NUM);
    if (selectedResult == 1) fprintf(stderr, "The Scheduler has sent the result to Hospital B using UDP over port %d\n", UDP_PORT_NUM);
    if (selectedResult == 2) fprintf(stderr, "The Scheduler has sent the result to Hospital C using UDP over port %d\n", UDP_PORT_NUM);
};

void updateOccupancy(int selectedResult) {
    hospitals[0][1] = selectedResult == 0 ? hospitals[0][1] + 1 : hospitals[0][1];
    hospitals[1][1] = selectedResult == 1 ? hospitals[1][1] + 1 : hospitals[1][1];
    hospitals[2][1] = selectedResult == 2 ? hospitals[2][1] + 1 : hospitals[2][1];
    updateHospital(selectedResult);
};

// Communicate with client
// Referenced TCP model at "Beej's Guide to Network Programming Using Internet Sockets"
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
        listen(sockfd, 5);

        remote_sockaddr_length = sizeof(remote_addr);
        
        newsockfd = accept(sockfd, (struct sockaddr *) &remote_addr, &remote_sockaddr_length);
        if (newsockfd < 0) {
            Error((char*)"ERROR on accept");
        }
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
    // on-screen message 1
    fprintf(stderr, "The Scheduler is up and running.\n");
    
    // build up UDP port
    bind();

    // get initial avaliability of hospitals
    hospitalA.getInitHospital(HOSPITALA_PORT_NUM, 0);
    hospitalB.getInitHospital(HOSPITALB_PORT_NUM, 1);
    hospitalC.getInitHospital(HOSPITALC_PORT_NUM, 2);
    
    // connect to TCP and receive the location from the client
    client.bindClient();
    
    while(1) {
        // connect to client and receive the location
        client.connectClient();
        int location = client.getClientLocation();
        // on-screen message 3
        fprintf(stderr, "The Scheduler has received client at location %d from the client using TCP over port %d\n", location, TCP_PORT_NUM);

        // receive the score from the hospitals
        getHospitalsScore(location);

        // determine which hospital to be selected
        int selectedResult;
        // if all the hospital distances are -1, the client location is not on the map 
        if (hospitalsScore[0][1] == -1 && hospitalsScore[1][1] == -1 && hospitalsScore[2][1] == -1) selectedResult = -2;
        // if one hospital distance is -1, the client is on that hospital, score = None
        else if (hospitalsScore[0][1] == -1 || hospitalsScore[1][1] == -1 || hospitalsScore[2][1] == -1) selectedResult = -1;
        else {
            int selectedLocation = selectHospital();
            selectedResult = chooseHospital(selectedLocation);
        }

        // on-screen message 7
        messageConfirm(selectedResult);

        // send the selected hospital to the client
        client.send(to_string(selectedResult));
        // on-screen message 7
        fprintf(stderr, "The Scheduler has sent the result to client using TCP over port %d\n", TCP_PORT_NUM);
        
        // update the selected hospital's info and send to the hospital
        updateOccupancy(selectedResult);
    }
}
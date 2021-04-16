#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <tr1/unordered_map>
#include <vector>
#include <set>
#include <unordered_set>
#include <queue>
#include <iostream>
using namespace std;

#define HOSPITALB_PORT_NUM 31864
#define UDP_PORT_NUM 33864
#define HOSTNAME "127.0.0.1"
#define FILENAME "map.txt"
#define FLT_MAX 3.402823466e+38F

static string client_location; // message of userid from Scheduler, backend server will give location based on this userid
static float hospitalScore[2]; // [0] stands for the score. [1] for the distances.

class Hospital {
    private:
    int location; // the location on the map
    int re_location;
    int capacity; // initial capacity of the hospital
    int occupancy; // inital occupancy of the hospital
    
    tr1::unordered_map<int, int> hospital_relocation_mapping; // store re_indexed_location, convert original location to re_indexed location. Eg. (78 => 0), (2 => 1).
    tr1::unordered_map<int, tr1::unordered_map<int, float> > matrix; // store the neighbors of each edges and their distances   

    public:
    // preparation: store and construct the map.txt
    void updateOccupancy() {
        this -> occupancy = this -> occupancy + 1;
        // on-screen message 9
        fprintf(stderr, "Hospital B has been assigned to a client, occupation is updated to %d, avaliability is updated to %g\n", this->occupancy, getAvailability());
    }

    void setInfo(int location, int capacity, char occupancy) {
        this -> location = location;
        this -> re_location = getRelocation(location);
        this -> capacity = capacity;
        this -> occupancy = occupancy;
        fprintf(stderr, "Hospital B has total capacity %d and initial occupancy %d\n", capacity, occupancy);
    }

    string getHospitalInfo() {
        return to_string(this->capacity) + " " + to_string(this->occupancy);
    }

    // set re_indexed_map
    void setReIndexMap(int location, int re_location) {
        hospital_relocation_mapping.insert(pair<int, int>(location, re_location));
    }

    // avoid duplicating reIndex
    bool hasReloNum(int location) {
        if (hospital_relocation_mapping.find(location) != hospital_relocation_mapping.end()) return true;
        return false;
    }

    // get reIndex location by client location
    int getRelocation(int location) {
        if (hospital_relocation_mapping.find(location) == hospital_relocation_mapping.end()) return -1; // locatioin not find
        else return hospital_relocation_mapping[location];
    }

    // get client location by reIndex location
    int getLocation(int re_location) {
        for (pair<int,int> p : hospital_relocation_mapping)
        {
            if (p.second == re_location) return p.first;
        }
        return re_location;
    }

    // set up the matrix with the re-indexs of locations
    void setAjacencyMatrixbyRow(int* single_row_location_info, float* single_row_location_distance) {
        int index_first = single_row_location_info[0];
        int index_second = single_row_location_info[1];
        float distance = single_row_location_distance[0];

        int re_index_first = getRelocation(index_first);
        int re_index_second = getRelocation(index_second);
        
        // map<int, map<int, float>>
        if (matrix[re_index_first].find(re_index_second) == matrix[re_index_first].end()) {
            matrix[re_index_first].insert(pair<int, float>(re_index_second, distance));
        }
        if (matrix[re_index_second].find(re_index_first) == matrix[re_index_second].end()) {
            matrix[re_index_second].insert(pair<int, float>(re_index_first, distance));
        }
    }

    void initialize(char* argv[]) {
        setInfo(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    }

    float getAvailability() {
        return (this->capacity - this->occupancy) / (float)this->capacity;
    }

    // BFS method: 
    float shortestPath(int reIndex) {
        queue<int> q;
        int relocation_size = hospital_relocation_mapping.size();
        bool visited[relocation_size] = {0};
        tr1::unordered_map<int, float> distance;
        for (int i = 0; i < relocation_size; i++) {
            distance[i] =  FLT_MAX;
        }
        distance[reIndex] = 0;
        q.push(reIndex);
        while (!q.empty()) {
            int size = q.size();
            for (int i = 0; i < size; i++) {
                int curIndex = q.front();
                visited[curIndex] = true;
                q.pop();
                for (pair<int, float> p : matrix[curIndex]) {
                    if (!visited[p.first] || distance[p.first] > distance[curIndex] + p.second) {
                        q.push(p.first);
                        distance[p.first] = min(distance[p.first], distance[curIndex] + p.second);
                    }
                }
            }
        }
        return distance[this->re_location];
    }

    void findLocationScore(int location) {
        float a = getAvailability();
        // on-screen message 5
        fprintf(stderr, "Hospital B has capacity = %d, occupation = %d, availability = %g\n", this->capacity, this->occupancy, getAvailability());
        float d;
        int reIndex = getRelocation(location);
        if (a < 0 || a > 1) a == -1;
        if (reIndex == -1 || location == this->location) d = -1;
        if (d == -1 || a = -1)  {
            // on-screen message 3
            if (d == -1) fprintf(stderr, "Hospital B does not have the location %d in map\n", location);
            hospitalScore[0] = -1;
            hospitalScore[1] = -1;
            return; 
        }
        hospitalScore[1] = shortestPath(reIndex);
        // on-screen message 6
        fprintf(stderr, "Hospital B has found the shortest path to client, distance = %g\n", hospitalScore[1]);
        hospitalScore[0] = 1 / (hospitalScore[1] * (1.1 - a));
        // on-screen message 7
        fprintf(stderr, "Hospital B has the score = %g\n", hospitalScore[0]);
    }
};

class File {
    private:
    int fsize; // file size
    FILE* fp;
    char cache[1024]; // max characters a txt file can hold in one line

    // set file size
    void setFileSize(FILE* fp) {
        // get the size of the file.
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
    }

    public:
    // Open map.txt and set file size
    void open() {
        this -> fp = fopen(FILENAME, "rt");
        if (fp == 0) {
            printf("ERROR, read file.");
            return;
        }

        setFileSize(fp);
    }

    // CLose map.txt
    void close() {
        fclose(fp);
    }
    
    void bufferToLocationNum(char* s, int* locationNum) {
        int i = 0;
        string str;

        for (int j = 0; j < 2; j++) {
            while (s[i] != ' ') {
                str = str + s[i];
                i++;
            }
            locationNum[j] = atoi(str.c_str());
        }
    }

    // re_index the two edges of each row and store in a map
    void reIndexHospitalLocation(Hospital& hospital) {
        int reNum = 0;
        while (fgets(cache,fsize,fp)) {
            if (cache[0] == ' ') {
                int i = 0;
                while (cache[i] == ' ') i++;
                strncpy(cache, cache + i, 1024);
            }

            int locationNum[2];
            bufferToLocationNum(cache, locationNum);
            if (hospital.hasReloNum(locationNum[0])) continue;
            else hospital.setReIndexMap(locationNum[0], reNum++);
            if (hospital.hasReloNum(locationNum[1])) continue;
            else hospital.setReIndexMap(locationNum[1], reNum++);
        }
        rewind(fp);
    }

    // read the file and contruct an array to store each row
    void bufferToLocationArray(char* s, int* single_row_location_info, float* single_row_location_distance) {
        // convert char[] to string
        string buffer = "";
        buffer += s;
        
        // add a white space to the original char
        buffer[buffer.length() - 1] = ' ';
        buffer[buffer.length()] = '\n';
        string str;
        int num = 0;
        int i = 0;

        while (buffer[i] != '\n') {
            if (buffer[i] == ' ') {
                if (str.length() != 0) { // the blank is at the end of a string
                    if (num < 2) single_row_location_info[num++] = atoi(str.c_str()); // first two as edges
                    else single_row_location_distance[0] = atof(str.c_str()); // the thrid as distance
                    str = "";
                }
            } else {
                str = str + buffer[i];
            }
            i++;
        }
    }
    
    // read the map.txt and store the nums into matrix
    void getMapMatrix(Hospital& hospital) {   
        while (fgets(cache, fsize, fp)) {
            int single_row_location_info[2];
            float single_row_location_distance[1] = {0};
            for (int i = 0; i < 2; i++) {
                single_row_location_info[i] = 0; // initialize each single_row in order to avoid unnecessary loop
            }
            bufferToLocationArray(cache, single_row_location_info, single_row_location_distance);
            hospital.setAjacencyMatrixbyRow(single_row_location_info, single_row_location_distance);
        }
        rewind(fp);
    }

    // the whole steps to read and initalize the map structure
    void construct(Hospital& hospital) {
        File file;
        file.open();
        file.reIndexHospitalLocation(hospital);
        file.getMapMatrix(hospital);
        file.close();
    }
};

// UDP connection to Scheduler
// Referenced UDP model at "Beej's Guide to Network Programming Using Internet Sockets"
class SchedulerMain {
    private:
    int sockfd;
    struct sockaddr_in remote_addr, local_addr;
    socklen_t remote_sockaddr_length, local_sockaddr_length;
    char buffer[1024];

    void Error(char *msg) {
        perror(msg);
        exit(0);
    }

    void receiveSchedulerMessages() {
        bzero(&buffer, 1024);
        int res = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&remote_addr, &remote_sockaddr_length);
        if (res < 0) {
            Error((char*)"recvfrom");
        }
    }

    void sendHospitalMessages(string message) {
        char server_message[message.length()];
        strcpy(server_message, message.c_str());

        int res = sendto(sockfd, server_message, message.length(), 0, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
        if (res < 0) {
            Error((char*)"sendto");
        }
    }

    public:
    void connectScheduler(Hospital& hospital) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            Error((char*)"Opening socket");
        }
        
        local_sockaddr_length = sizeof(local_addr);
        bzero(&local_addr, local_sockaddr_length);

        // remote address
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(UDP_PORT_NUM);
        remote_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        // local address
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(HOSPITALB_PORT_NUM);
        local_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        int res = bind(sockfd, (struct sockaddr *)&local_addr, local_sockaddr_length);
        if(res < 0) {
            Error((char*)"binding");
        }
        
        remote_sockaddr_length = sizeof(struct sockaddr_in);
        
        // initial the hospital info and send to scheduler
        string message = hospital.getHospitalInfo();
        sendHospitalMessages(message);
        
        while (1) {
            // receive client info from the scheduler
            receiveSchedulerMessages();
            int location = atoi(buffer);

            // using BFS to find the shortest location score
            // socre = -1 stands for score = None
            // on-screen message 2
            fprintf(stderr, "Hospital B has received input from client at location %d\n", location);
            hospital.findLocationScore(location);

            string returnMessage = to_string(hospitalScore[0]) + " " + to_string(hospitalScore[1]);

            // send the score to the scheduler
            sendHospitalMessages(returnMessage);
            // on-screen message 4/8
            if (hospitalScore[1] == -1) fprintf(stderr, "Hospital B has sent 'location not found' to the Scheduler\n");
            else fprintf(stderr, "Hospital B has sent score = %g and distance = %g to the Scheduler\n", hospitalScore[0], hospitalScore[1]);

            // receive the scheduler's response and update the map
            // 1 stands for the hospital has been selected, 0 for not
            receiveSchedulerMessages();
            int chosen = atoi(buffer);

            if (chosen == 1) {
                hospital.updateOccupancy();
            }
        }
    }
};

int main(int argc, char* argv[]) {
    File map;
    Hospital hospitalB;
    SchedulerMain schedulermain;

    if (argc < 4) {
        fprintf(stderr, "you mush input all the three of the hospital, as '<hospital location> <total capacity> <initial occupancy>'\n");
        exit(0);
    }

    // on-screen message 1
    fprintf(stderr, "Hospital B is up and running using UDP on port %d\n", HOSPITALB_PORT_NUM);

    // construct the map and store in hospitalB
    map.construct(hospitalB);
    
    // initialize the location, capacity & occupancy of the hospitalB
    hospitalB.initialize(argv);
    
    // connect to the scheduler using UDP
    schedulermain.connectScheduler(hospitalB);
}
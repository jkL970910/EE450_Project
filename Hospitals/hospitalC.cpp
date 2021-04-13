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

#define HOSPITALC_PORT_NUM 32864
#define UDP_PORT_NUM 33864
#define HOSTNAME "127.0.0.1"
#define FILENAME "map_simple.txt"
#define MAX_LOCATION_NUM 100
#define FLT_MAX 3.402823466e+38F

static string client_location; // message of userid from Scheduler, backend server will give location based on this userid

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
    void setInfo(int capacity, int occupancy) {
        this -> capacity = capacity;
        this -> occupancy = occupancy;
    }

    void setInfo(int location, int capacity, char occupancy) {
        this -> location = location;
        this -> re_location = getRelocation(location);
        this -> capacity = capacity;
        this -> occupancy = occupancy;
        cout << this->location << " " << this->capacity << " " << this->occupancy << endl;
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
    void setAjacencyMatrixbyRow(float* single_row_location_info) {
        int index_first = (int)single_row_location_info[0];
        int index_second = (int)single_row_location_info[1];
        float distance = single_row_location_info[2];

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

    float shortestPath(int reIndex) {
        if (reIndex == this->re_location) return 0;
        
        float shortest = FLT_MAX;
        float current = 0;
        bool visited[hospital_relocation_mapping.size()] = {0};
        visited[reIndex] = true;
        helper(shortest, current, visited, reIndex);
        cout << "shortest path is: " << shortest << endl;
        return shortest;
    }

    // helper function of DFS
    void helper(float& shortest, float& current, bool* visited, int index) {
        if (index == this->re_location) {
            shortest = min(shortest, current);
            return;
        }
        
        for (pair<int, float> p : matrix[index]) {
            if (!visited[p.first]) {
                visited[p.first] = true;
                float temp = current;
                current = current + p.second;
                helper(shortest, current, visited, p.first);
                current = temp;
                visited[p.first] = false;
            }
        }
    }

    float findLocationScore(int location) {
        float a = getAvailability();
        float d;
        a = (a < 0 || a > 1) ? -1 : a;
        int reIndex = getRelocation(location);
        if (reIndex == -1) d = -1;
        if (a == -1 || d == -1) {
            fprintf(stderr, "the client location is not in the map\n");
            return -1; 
        }
        d = shortestPath(reIndex);
        return 1 / (d * (1.1 - a));
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
    void bufferToLocationArray(char* s, float* location_info) {
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
                    if (num < 2) location_info[num++] = atoi(str.c_str()); // first two as edges
                    else {
                        location_info[num++] = atof(str.c_str()); // the thrid as distance
                    }
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
            float single_row_location_info[3];
            for (int i = 0; i < 3; i++) {
                single_row_location_info[i] = 0; // initialize each single_row in order to avoid unnecessary loop
            }
            bufferToLocationArray(cache, single_row_location_info);
            hospital.setAjacencyMatrixbyRow(single_row_location_info);
        }
        rewind(fp);
    }

    // the whole steps to read and initalize the map structure
    void construct(Hospital& hospital) {
        File file;
        file.open();

        file.reIndexHospitalLocation(hospital);
        file.getMapMatrix(hospital);
        fprintf(stderr, "The hospitalC has constructed the map\n");
        
        file.close();
    }
};

// UDP connection to Scheduler
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
        local_addr.sin_port = htons(HOSPITALC_PORT_NUM);
        local_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        int res = bind(sockfd, (struct sockaddr *)&local_addr, local_sockaddr_length);
        if(res < 0) {
            Error((char*)"binding");
        }
        
        remote_sockaddr_length = sizeof(struct sockaddr_in);

        fprintf(stderr, "The hospitalC is up and running using UDP on port <%d>\n", HOSPITALC_PORT_NUM);
        
        // initial the hospital info and send to scheduler
        string message = hospital.getHospitalInfo();
        sendHospitalMessages(message);
        fprintf(stderr, "The hospitalC has sent its inital info to the Scheduler\n");
        
        while (1) {
            // receive client info from the scheduler
            receiveSchedulerMessages();
            
            int location = atoi(buffer);

            // using DFS to find the shortest location score
            float score = hospital.findLocationScore(location);

            // send the score to the scheduler
            sendHospitalMessages(to_string(score));

            // receive the scheduler's response and update the map
            // receiveSchedulerMessages();
        }
    }
};

int main(int argc, char* argv[]) {
    File map;
    Hospital hospitalC;
    SchedulerMain schedulermain;

    if (argc < 4) {
        fprintf(stderr, "you mush input all the three of the hospital, as '<hospital location> <total capacity> <initial occupancy>'\n");
        exit(0);
    }

    // construct the map and store in hospitalC
    map.construct(hospitalC);

    // initialize the location, capacity & occupancy of the hospitalC
    hospitalC.initialize(argv);
    
    // connect to the scheduler using UDP
    schedulermain.connectScheduler(hospitalC);
}
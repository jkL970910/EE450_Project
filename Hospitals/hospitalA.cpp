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

#define SERVERA_PORT_NUM 30991
#define HOSTNAME "127.0.0.1"
#define FILENAME "map_simple.txt"
#define MAX_LOCATION_NUM 100

static string query_user_id; // message of userid from Scheduler, backend server will give location based on this userid

class Hospital {
    private:
    int location; // the location on the map
    int capacity; // initial capacity of the hospital
    int occupancy; // inital occupancy of the hospital
    
    tr1::unordered_map<int, int> hospital_relocation_mapping; // store re_indexed_location, convert original location to re_indexed location. Eg. (78 => 0), (2 => 1).
    tr1::unordered_map<int, tr1::unordered_map<int, float>> matrix; // store the neighbors of each edges and their distances
    set<int> avaliableHospital; // store the avaliable hospital locations on the map

    public:
    // preparation: store the map.txt
    void setLocation(int location) {
        this -> location = location;
    }

    void setCapacity(int capacity) {
        this -> capacity = capacity;
    }

    void setOccupancy(int occupancy) {
        this -> occupancy = occupancy;
    }

    string getHospitalInfo() {
        return to_string(this->location) + " " + to_string(this->capacity) + " " + to_string(this->occupancy);
    }

    // set re_indexed_map
    void setLocationMap(int location, int re_location) {
        hospital_relocation_mapping.insert(pair<int, int>(location, re_location));
    }

    bool hasReloNum(int location) {
        if (hospital_relocation_mapping.find(location) != hospital_relocation_mapping.end()) return true;
        return false;
    }

    int getRelocation(int location) {
        if (hospital_relocation_mapping.find(location) == hospital_relocation_mapping.end()) return -1; // locatioin not find
        else return hospital_relocation_mapping[location];
    }

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

    void initialize() {
        cout << "please enter the initial capacity and occupancy of HospitalA: " << endl;
        cout << "the format is: '<hospitalA location> <total capacity> <initial occupancy>': " << endl;
        int location, capacity, occupancy;
        cin >> location >> capacity >> occupancy;
        setLocation(location);
        setCapacity(capacity);
        setOccupancy(occupancy);
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
            else hospital.setLocationMap(locationNum[0], reNum++);
            if (hospital.hasReloNum(locationNum[1])) continue;
            else hospital.setLocationMap(locationNum[1], reNum++);
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
    void getAdjacencyMatrix(Hospital& hospital) {   
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

    void ready(Hospital& hospital) {
        File file;
        file.open();

        file.reIndexHospitalLocation(hospital);
        file.getAdjacencyMatrix(hospital);
        fprintf(stderr, "The server A has constructed the map\n");
        hospital.initialize();
        
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
    bool send_lock;

    void Error(char *msg) {
        perror(msg);
        exit(0);
    }

    void sendResponsibleHospitals(string message) {
        char server_message[message.length()];
        strcpy(server_message, message.c_str());

        int res = sendto(sockfd, server_message, message.length(), 0, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
        if (res < 0) {
            Error((char*)"sendto");
        }

        fprintf(stderr, "The hospitalA has sent its initial info to Scheduler\n");
    }

    public:
    void connectScheduler(Hospital& hospital) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            Error((char*)"Opening socket");
        }
        
        local_sockaddr_length = sizeof(local_addr);
        bzero(&local_addr, local_sockaddr_length);

        // local address
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(SERVERA_PORT_NUM);
        local_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        int res = bind(sockfd, (struct sockaddr *)&local_addr, local_sockaddr_length);
        if(res < 0) {
            Error((char*)"binding");
        }
        
        remote_sockaddr_length = sizeof(struct sockaddr_in);

        fprintf(stderr, "The server A is up and running using UDP on port <%d>\n", SERVERA_PORT_NUM);
        while (1) {
            string message;
            bzero(&buffer, 1024);
            // this sentence has no real function, only because UDP server has to boot up first and freeze in while(1)
            res = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&remote_addr, &remote_sockaddr_length);
            if (res < 0) {
                Error((char*)"recvfrom");
            }
            
            if (send_lock == false) { // if it is the first time sending message to server main, first time send responsible hospitals
                message = hospital.getHospitalInfo();
                sendResponsibleHospitals(message);
                send_lock = true;
            }
            else { // non first time, send recommendation
                message = query_user_id;

                int res = sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
                if (res < 0) {
                    Error((char*)"sendto");
                }
                fprintf(stderr, "The server A has sent the result(s) to Scheduler\n");
            }
        }
    }
};

int main() {
    File file;
    Hospital A;
    SchedulerMain schedulermain;

    file.ready(A);

    schedulermain.connectScheduler(A);
}
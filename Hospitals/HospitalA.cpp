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
using namespace std;

#define SERVERA_PORT_NUM 30991
#define HOSTNAME "127.0.0.1"
#define FILENAME "data1.txt"
#define MAX_USER_NUM 100
#define MAX_COUNTRY_NUM 10

static string query_user_id; // message of userid from servermain, backend server will give recommendation based on this userid

class SchedulerMain
{
    private:
    int sockfd;
    struct sockaddr_in remote_addr, local_addr;
    socklen_t remote_sockaddr_length, local_sockaddr_length;
    char buffer[1024];
    bool send_lock;

    void Error(char *msg)
    {
        perror(msg);
        exit(0);
    }

    void sendResponsibleCountries()
    {
        string message = "first time to connect to scheduler";
        char server_message[message.length()];
        strcpy(server_message, message.c_str());

        int res = sendto(sockfd, server_message, message.length(), 0, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
        if (res < 0)
        {
            Error((char*)"sendto");
        }

        fprintf(stderr, "The server A has sent a connection confirm to Scheduler\n");
    }

    public:
    void connectScheduler()
    {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0)
        {
            Error((char*)"Opening socket");
        }
        
        local_sockaddr_length = sizeof(local_addr);
        bzero(&local_addr, local_sockaddr_length);

        // local address
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(SERVERA_PORT_NUM);
        local_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        int res = bind(sockfd, (struct sockaddr *)&local_addr, local_sockaddr_length);
        if(res < 0)
        {
            Error((char*)"binding");
        }
        
        remote_sockaddr_length = sizeof(struct sockaddr_in);

        fprintf(stderr, "The server A is up and running using UDP on port <%d>\n", SERVERA_PORT_NUM);
        while (1)
        {
            string message;
            bzero(&buffer, 1024);
            // this sentence has no real function, only because UDP server has to boot up first and freeze in while(1)
            res = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&remote_addr, &remote_sockaddr_length);
            if(res < 0)
            {
                Error((char*)"recvfrom");
            }
            
            if(send_lock == false) // if it is the first time sending message to server main, first time send responsible countries
            {
                sendResponsibleCountries();
                send_lock = true;
            }
            else // non first time, send recommendation
            {
                message = "second";

                int res = sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
                if (res < 0)
                {
                    Error((char*)"sendto");
                }
                fprintf(stderr, "The server A has sent the result(s) to Scheduler\n");
            }
            

        }
    }
};

int main()
{
    SchedulerMain schedulermain;

    schedulermain.connectScheduler();
}
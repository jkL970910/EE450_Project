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
#define TCP_PORT_NUM 33991
#define HOSTNAME "127.0.0.1"
using namespace std;

class SchedulerMain
{
    private:
    int sockfd, port_num; // sockfd == sock file descriptor
    struct sockaddr_in remote_addr;
    socklen_t remote_sockaddr_length;

    char buffer[256];
    int client_id;

    // print error message relating to socket usage
    void Error(char *msg)
    {
        perror(msg);
        exit(0);
    }

    public:
    void connectServer()
    {
        // set port number
        port_num = TCP_PORT_NUM;

        // create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            Error((char*)"Error opening socket");
        }

        // set schedulermain address
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port_num); // htons 将整型变量从主机字节顺序转变成网络字节顺序
        remote_addr.sin_addr.s_addr = inet_addr(HOSTNAME); // inet_addr 一个点分十进制的IP转换成一个长整数型数

        remote_sockaddr_length = sizeof(remote_addr);
        int res = connect(sockfd, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
        if(res < 0) 
        {
            Error((char*)"ERROR connecting");
        }

        fprintf(stderr, "Client is up and running\n");
    }

    void queryHospital()
    {
        string user_id, message;
        fprintf(stderr, "-----Start a new request-----\n");
        fprintf(stderr, "Please enter the User ID: ");
        cin >> user_id;

        message += user_id;

        bzero(buffer,256);    
        strcpy(buffer, message.c_str());

        // send query to schedulermain
        int res = write(sockfd, buffer, strlen(buffer));

        if (res < 0)
        {
            Error((char*)"ERROR writing to socket");
        }
        fprintf(stderr, "Client has sent Message<%s> to Scheduler using TCP\n", user_id.c_str());

        // get result from schedulermain
        bzero(buffer, strlen(buffer));
        res = read(sockfd, buffer, 256);
        if (res < 0)
        {
            Error((char*)"ERROR reading from socket");
        }

        // print the received result from Scheduler
        fprintf(stderr, "Client has received results from Scheduler: <%s> \n", buffer);
    }
};

int main()
{
    SchedulerMain schedulermain;
    schedulermain.connectServer();

    while(1)
    {
        schedulermain.queryHospital();
    }

}
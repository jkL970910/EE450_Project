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
#define TCP_SERVERMAIN_PORT_NUM 33991
#define HOSTNAME "127.0.0.1"
using namespace std;

class ServerMain
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
    void Connect()
    {
        // set port number
        port_num = TCP_SERVERMAIN_PORT_NUM;

        // create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            Error((char*)"Error opening socket");
        }

        // set servermain address
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port_num);
        remote_addr.sin_addr.s_addr = inet_addr(HOSTNAME);

        remote_sockaddr_length = sizeof(remote_addr);
        int res = connect(sockfd, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
        if(res < 0) 
        {
            Error((char*)"ERROR connecting");
        }

        // get client id from servermain
        bzero(buffer, strlen(buffer));
        res = read(sockfd, buffer, 256);
        if (res < 0)
        {
            Error((char*)"ERROR reading from socket");
        }

        client_id = buffer[0] - '0';

        fprintf(stderr, "Client%d is up and running\n", client_id);
    }

    void Query()
    {
        string user_id, country_name, message;
        fprintf(stderr, "-----Start a new request-----\n");
        fprintf(stderr, "Please enter the User ID: ");
        cin >> user_id;

        message += user_id;

        message += ",";

        fprintf(stderr, "Please enter the Country Name: ");
        cin >> country_name;

        message += country_name;

        bzero(buffer,256);    
        strcpy(buffer, message.c_str());

        // send query to servermain
        int res = write(sockfd, buffer, strlen(buffer));
        if (res < 0)
        {
            Error((char*)"ERROR writing to socket");
        }
        fprintf(stderr, "Client%d has sent User<%s> and <%s> to Main Server using TCP\n", client_id, user_id.c_str(), country_name.c_str());
        
        // get result from servermain
        bzero(buffer, strlen(buffer));
        res = read(sockfd, buffer, 256);
        if (res < 0)
        {
            Error((char*)"ERROR reading from socket");
        }

        // 4 cases:  
        if(buffer[0] == 'C') // buffer[0] == C // country not found
        {
            fprintf(stderr, "<%s> not found\n", country_name.c_str());
        }
        else if(buffer[0] == 'U') // buffer[0] == U // user not found 
        {
            fprintf(stderr, "User<%s> not found\n", user_id.c_str());
        }
        else if(buffer[0] == '-') // buffer[0] == - // already connected to all other users
        {
            fprintf(stderr, "User<%s> is already connected to all other users, no new recommendation\n", user_id.c_str());
        }
        else // recommendation
        {
            fprintf(stderr, "Client<%d> has received results from Main Server: User<%s> is possible friend of User<%s> in <%s>\n", client_id, buffer, user_id.c_str() ,country_name.c_str());
        }
    }
};

int main()
{
    ServerMain servermain;
    servermain.Connect();

    while(1)
    {
        servermain.Query();
    }

}
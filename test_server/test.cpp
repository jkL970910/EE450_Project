/* Creates a datagram server. The port number is passed as an argument. This server runs forever */
/* Referenced UDP model at https://vinodthebest.wordpress.com/category/c-programming/c-network-programming/ */

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

static std::tr1::unordered_map<string, int> country_backend_mapping; // convert country name to number. Eg. A => 0, Canada => 1.
static string country_list; // message to servermain clarify which countries this backend server responsible
static string query_user_id; // message of userid from servermain, backend server will give recommendation based on this userid
static string query_country_name; // // message of country name from servermain, backend server will give recommendation based on this country

// for sort the elements in priority queue
// two cases involved, 1. (userid, common_friend_num) 2. (userid, connection_num)
struct Connection
{
    int c_n1;
    int c_n2;

    // constructor
    Connection(int n1, int n2) : c_n1(n1), c_n2(n2)
    {
    }

    bool operator<(const struct Connection& other) const
    {
        if(c_n2 == other.c_n2) return c_n1 > other.c_n1; 
        return c_n2 < other.c_n2 ;
    }
};

class Country
{
    private:
    int population; // number of users in this country
    std::tr1::unordered_map<int, int> user_backend_mapping; // store re_indexed_userId, convert original userId to re_indexed userId. Eg. (78 => 0), (2 => 1).

    vector<set<int> > adjacency_matrix; // user connection adjacency matrix. PS: in essence it is a adjacency list Eg. A => 0, xYz => 1


    public:
    /*********************Preparation**********************/ // data.txt to local storage
    void SetPopulation(int population)
    {
        this -> population = population;
    }

    void SetUserMap(int user_id, int reindexed_user_id) // set user_backend_mapping
    {
        user_backend_mapping.insert(pair<int,int>(user_id, reindexed_user_id));
    }

    int GetReindexedUserId(int user_id) // same as GetUserBackendMapping
    {
        if(user_backend_mapping.find(user_id) == user_backend_mapping.end()) return -1; // user not found
        else return user_backend_mapping[user_id];
    }

    int GetOriginalUserId(int reindexed_user_id) // get the original userId by reindexed_userId Eg. Input: 0 => Output: 78
    {
        for(pair<int,int> p : user_backend_mapping)
        {
            if(p.second == reindexed_user_id) return p.first;
        }

        return reindexed_user_id;
    }

    void SetAdjacencyMatrixbyRow(int* single_row_user_info) // Eg. data1.txt.xyz.column1 Eg. (1,0,3) // in data1.txt => (0,1,2) //in local adjacency matrix
    {
        int reindexed_current_id = GetReindexedUserId(single_row_user_info[0]); // Eg. 1 => 0 
        set<int> row;
        for(int i = 0; i < MAX_USER_NUM && single_row_user_info[i] != -1; i++)
        {
            int reindexed_u_id = GetReindexedUserId(single_row_user_info[i]);
            if(reindexed_current_id != reindexed_u_id) // exclude someone friends himself
            {
                row.insert(reindexed_u_id);
            }
                    
        }
        
        adjacency_matrix.insert(adjacency_matrix.begin() + reindexed_current_id, row); // insert the whole row to "his" position
    }
    /*******************End-Preparation*********************/


    /******************Recommendation***********************/
    /// <summary>
    /// Get all friends of query user.
    /// </summary>
    /// <param name="query_user_id">query userid</param>
    /// <param name="friend_list">all friends of query user</param>
    /// <returns></returns>
    void GetFriends(int query_user_id, unordered_set<int>* friend_list)
    {
        for(int f : adjacency_matrix[query_user_id])
        {
            (*friend_list).insert(f);
        }
    }

    /// <summary>
    /// Get all strangers of query user. Stranger: Strangers are those who are not friend with query user
    /// Eg. in canada, if query user is 8, then his stranger is 11. [11][0] // PS: the second parameter is the number of common friends with query user
    /// Eg2. in xYz, if query user is 0, then his stranger is 3. [3][1] // PS: the second parameter is the number of common friends with query user
    /// </summary>
    /// <param name="query_user_id">query userid</param>
    /// <param name="friend_list">all friends of query user</param>
    /// <param name="stranger_list">all strangers of query user.</param>
    /// <returns></returns>
    void  GetStrangers(int query_user_id, unordered_set<int>& friend_list, priority_queue<Connection>* stranger_list)
    {
        
        for(int u = 0; u < population; u++)
        {
            // exclude query user and his friends from strangers list
            if(u == query_user_id || (friend_list.find(u) != friend_list.end())) continue;

            int common_friend_num = 0;

            // get the number of common friends a stranger has with query user
            for(int f : friend_list)
            {
                set<int>::iterator iter;
                if( (iter = adjacency_matrix[u].find(f)) != adjacency_matrix[u].end()) common_friend_num++;
            }

            (*stranger_list).push(Connection(u, common_friend_num));
        }
    }

    /// <summary>
    /// Get all strangers connection number
    /// Eg. in canada, if query user is 8, then his stranger is 11. [11][0] // 11 got 0 connection
    /// Eg2. in canada, if query user is 11, then his strangers are 3,2,8,78. [8][3] // 8 got 3 connections
    /// </summary>
    /// <param name="query_user_id">query userid</param>
    /// <param name="friend_list">all friends of query user</param>
    /// <param name="connection_num_map">connetion number of all strangers for certain query user</param>
    /// <returns></returns>
    void GetConnctionMap(int query_user_id, unordered_set<int>& friend_list, priority_queue<Connection>* connection_num_map)
    {
        int user_id = -1;

        for(set<int> s : adjacency_matrix)
        {
            user_id++;

            // exclude query user and his friends. == all query user`s strangers
            if(user_id == query_user_id || (friend_list.find(user_id) != friend_list.end())) continue;

            (*connection_num_map).push(Connection(user_id, s.size()));
        }
    }

    // recommend one possible friend for query user
    int RecommendFriend(int query_user_id)
    {
        std::unordered_set<int> friend_list; // store id of all friends to query user
        priority_queue<Connection> stranger_list; // store all users except query user and his friends and each stranger`s common friends number with query user.
        priority_queue<Connection> connection_num_map; // store all strangers connection number
        GetFriends(query_user_id, &friend_list);

        GetStrangers(query_user_id, friend_list, &stranger_list);

        GetConnctionMap(query_user_id, friend_list, &connection_num_map);


        if(stranger_list.size() == 0 || stranger_list.top().c_n2 == 0)
        {
            if(connection_num_map.size() == 0) return -1; // case 1
            else return connection_num_map.top().c_n1; // case 2 a
        }
        else return stranger_list.top().c_n1; // case 2 b

    }
    /******************End-Recommendation******************/
};

class File
{
    private:
    int fsize; // file size
    FILE* fp;
    char cache[1024]; // max characters a txt file can hold in one line

    // set file size
    void SetFileSize(FILE* fp)
    {
        // get the size of the file.
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
    }

    public:
    // Open dataX.txt and set file size
    void Open()
    {
        this -> fp = fopen(FILENAME, "rt");
        if(fp == 0)
        {
            printf("ERROR, read file.");
            return;
        }

        SetFileSize(fp);
    }

    // CLose dataX.txt
    void Close()
    {
        fclose(fp);
    }

    /// <summary>
    /// convert all user data in dataX.txt to array.
    /// </summary>
    /// <param name="s">complete contend in dataX.txt</param>
    /// <param name="user_info">put every number of each line in data.txt to this array</param>
    /// <returns></returns>
    void BufferToUserArray(char* s, int* user_info)
    {
        // convert char[] to string;
        string buffer = "";
        buffer += s;
        
        // add a white space to the original char. Eg. 0_1_2 => 0_1_2_
        buffer[buffer.length() - 1] = ' ';
        buffer[buffer.length()] = '\n';
        
        string str; // store every continuous number. Eg. str = 100 √ str = 10 12 × 
        int num = 0; // count number of people in buffer. 
        int i = 0; // position of buffer.
        
        /* here we store all userId in buffer to array.
        if it is not white space, we keep add number to str. make it continuous.
        Eg. store 100 to str => str = 1; str = 0; str = 0;
        if it is white space, put that number to array.*/
        while(buffer[i] != '\n')
        {
            if(buffer[i] == ' ' && str.length() != 0)
            {
                user_info[num++] = atoi(str.c_str());
                str = "";
            }
            else
            {
                str = str + buffer[i];
            }
            i++;
        }
        
    }

    /// <summary>
    /// convert all first element of user data in dataX.txt to array.
    /// </summary>
    /// <param name="s">complete contend in dataX.txt</param>
    /// <param name="user_info">put first userid of each line in data.txt to this array</param>
    /// <returns></returns>
    void BufferToUserId(char* s, int* user_info)
    {
        int i = 0;
        string str;

        while(s[i] != ' ')
        {

            str = str + s[i];
            i++;
        }
        user_info[0] = atoi(str.c_str());

    }

    /// <summary>
    /// Reindex all country name and user name, Fill country_backend_mapping and user_backend_mapping of every Country object
    /// Explanation1: Eg. country_backend_mapping["A"] == 0;  country_backend_mapping["xYz"] == 1;
    /// Explanation2: Eg. country[1].user_backend_mapping[1] == 0;  country[1].user_backend_mapping[0] == 1; country[1].user_backend_mapping[3] == 2;
    /// </summary>
    /// <param name="country">all countries data1.txt contains</param>
    /// <returns></returns>
    void ReIndexCountryAndUser(Country* country)
    {
        int country_num = -1, user_num = 0;
        while (fgets(cache,fsize,fp))
        {
            if(cache[0] == ' ') // if it starts with white space, remove head white speaces
            {
                int i = 0;
                while(cache[i] == ' ') i++;
                strncpy(cache, cache + i, 1024);
            }
            
            if(cache[0] >= '0' && cache[0] <= '9') // if this line starts with number => user line
            {
                int user_info[1]; //first user id of every row
                BufferToUserId(cache, user_info);
                country[country_num].SetUserMap(user_info[0], user_num++);
            }
            else // country name line
            {
                if(country_num != -1) country[country_num].SetPopulation(user_num);
                user_num = 0; // otherwise in the next country, id starts with 3. First example.

                cache[strlen(cache) - 1] = '\0'; // remove \\n
                country_list += cache;
                country_list += ",";  // Eg. A,xYz
                country_backend_mapping.insert(pair<string,int>(cache, ++country_num));
            }
        }

        if(country_num != -1) country[country_num].SetPopulation(user_num);

        rewind(fp);
    }

    /// <summary>
    /// Convert all user connections to an adjacentcy matrix for every country
    /// </summary>
    /// <param name="country">all countries data1.txt contains</param>
    /// <returns></returns>
    void GetAdjacencyMatrix(Country* country)
    {
        int country_num = 0;

        while(fgets(cache,fsize,fp))
        {
            if(cache[0] >= '0' && cache[0] <= '9')
            {
                int single_row_user_info[MAX_USER_NUM];
                for(int i = 0; i < MAX_USER_NUM; i++) // set all user to -1 first, so SetAdjacencyMatrixbyRow can precisely end the loop
                {
                    single_row_user_info[i] = -1;
                }
                BufferToUserArray(cache, single_row_user_info);

                country[country_num].SetAdjacencyMatrixbyRow(single_row_user_info);
            }
            else
            {
                cache[strlen(cache) - 1] = '\0'; // remove \\n
                country_num = country_backend_mapping[cache];
            }
        }
        rewind(fp);

    }

    void Ready(Country* country)
    {
        File file;

        file.Open();

        file.ReIndexCountryAndUser(country);
        file.GetAdjacencyMatrix(country);

        file.Close();
    }
};

class ServerMain
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

    void SendResponsibleCountries()
    {
        // string has to first convert to char[] for sending country_list to servermain.
        char country_list_ch[country_list.length()]; 
        strcpy(country_list_ch, country_list.c_str());

        int res = sendto(sockfd, country_list_ch, country_list.length(), 0, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
        if (res < 0)
        {
            Error((char*)"sendto");
        }

        fprintf(stderr, "The server A has sent a country list to Main Server\n");
    }

    string GetRecommendationMessage(Country* country, char* buffer)
    {
        char copy[1024];
        char* ptr = copy, *retptr;
        int query_id = 0, user_id, country_id, reindexed_user_id, recommend_user_id;
        string query[2];

        strcpy(copy, buffer);
        // convert 100,China to query[0] == 100, query[1] == China
        while((retptr = strtok(ptr, ",")) != NULL)
        {
            query[query_id++] = retptr;
            ptr = NULL;
        }

        query_country_name = query[1];
        country_id = country_backend_mapping[query_country_name];
        query_user_id = query[0];
        user_id = atoi(query[0].c_str());
        reindexed_user_id = country[country_id].GetReindexedUserId(user_id);
        fprintf(stderr, "The server A has received request for finding possible friends of User<%s> in <%s>\n", query_user_id.c_str(), query_country_name.c_str());
        if(reindexed_user_id == -1)
        {
            fprintf(stderr, "User<%d> does not show up in <%s>\n", user_id, query_country_name.c_str());
            return "User ID: Not found";
        } 
        
        fprintf(stderr, "The server A is searching possible friends for User<%s> …\n", query_user_id.c_str());
        recommend_user_id = country[country_id].RecommendFriend(reindexed_user_id);
        
        recommend_user_id = country[country_id].GetOriginalUserId(recommend_user_id);
        fprintf(stderr, "Here are the results: User<%d>...\n", recommend_user_id);
        return to_string(recommend_user_id);
    }

    public:
    void Connect(Country* country)
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
                SendResponsibleCountries();
                send_lock = true;
            }
            else // non first time, send recommendation
            {
                message = GetRecommendationMessage(country, buffer);
                

                int res = sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr *)&remote_addr, remote_sockaddr_length);
                if (res < 0)
                {
                    Error((char*)"sendto");
                }
                if(message[0] == 'U') fprintf(stderr, "The server A has sent “User<%s> not found” to Main Server\n", query_user_id.c_str()); 
                else fprintf(stderr, "The server A has sent the result(s) to Main Server\n");
            }
            

        }
    }
};


int main()
{
    File file;
    Country c[MAX_COUNTRY_NUM];
    ServerMain servermain;

    file.Ready(c);

    servermain.Connect(c);
}
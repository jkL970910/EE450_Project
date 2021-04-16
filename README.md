# USC-EE450-Project Description
# Author
Full Name: Jiekun Liu
USCID: 8614237864


# What I have done in the assignment
1. I built up a TCP connection between the Client and the Scheduler, three UDP connections between the Scheduler and the three hospitals
2. Read the map.txt (in my code I named the file as map_hard.txt), convert map's informatioin into data structure and stored in the hospitals
3. Implemented the shortest path algorithm, calculate hospitals' scores based on the given location and assign hospital to client in multiple cases.


# What my code files are and what each one of them does
In hospitalA.cpp:
1. the hospitalX.cpp first read and convert data in map.txt into local storage, re-index the locations (in tr1::unordered_map<int, int> hospital_relocation_mapping).
2. build up the map data structure, store the neighbors of each edges and their distances (in tr1::unordered_map<int, tr1::unordered_map<int, float> > matrix).
3. build up a UDP connection with the Scheduler, tell him the initial capacity and occupancy of the hospital.
4. calculate the shortest path for the given client's location using BFS, and return the score to the Scheduler.
5. receive the update message from the scheduler and to update the hospital's availability if necessary.
PS: the hospitalB.cpp and hospitalC.cpp are almost the same code as hospitalA.cpp, the only difference between them is the assigned UDP port number and their initial input availability.

In scheduler.cpp:
1. build up UDP connections with the three hospitals, receive and store their initial availabilities in local storage (in int hospitals[3][2]).
2. build up TCP connection with the client, receive the location of the client via TCP and send query to hospitals vid UDP.
3. receive the scores return from hospitals and determine which hospital should be selected (in float hospitalsScore[3][2]).
4. inform the Client via TCP with the selected hospital or None hospital or client location not found, then inform the selected hospital to update its availability if necessary.

In client.cpp:
1. build up TCP connection with the Scheduler, send its location to the scheduler via TCP.
2. receive the assignment result from the Scheduler and print it.


# The format of all the messages exchanged
1. in client.cpp:
client to scheduler: fields of class 'SchedulerMain'
    client sends <location> to scheduler
    client receives <hospital/None> from scheduler

2. in scheduler.cpp:
scheduler to hospitals: fields of class 'HospitalServer', and some functions in the scheduler field
    scheduler receives <total capacity> & <inital occupancy> from hospitalA, hospitalB and hospitalC
    scheduler sends <location> to available hospitals
    scheduler receives <score> from available hospitals
    scheduler sends <update information> to selected hospitals

scheduler to client: fields of class 'Client'
    scheduler receives <location> from client
    scheduler sends <selected hospital / None> to client

3. in hospitalA.cpp: (hospitalB.cpp and hospitalC.cpp are the same)
hospital reads map.txt: fields of class 'File'
    hospitalA read the map.txt and construct a map

hospital to scheduler: fields of class 'SchedulerMain'
    hospitalA sends its inital <capacity> and <occupancy> to scheduler
    hospitalA receives <location> from scheduler
    hospitalA sends <score> or <location not found message> to scheduler
    hospitalA receives <update information> from scheduler if it has been selected

hospital inside: fields of class "hospital"
    construct and store the map informtaion in local storage
    find and calculate the shortest path of the given location

# Any idiosyncrasy of my project
This project meets all the reqirements of the assignment.
PS: New client/server process is unable to use the same port unless zombie process is killed


# Reused Code
I have used the sample TCP and UDP code from "Beej's Guide to Network Programming"
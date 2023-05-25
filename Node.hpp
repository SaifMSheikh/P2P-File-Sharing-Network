#include "RequestManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <errno.h>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

using namespace std;

// Connection Data Structure
struct Connection
{
    int* sock;      // File Descriptor Endpoint
    bool isOpen;    // Is The Socket Being Used

    struct sockaddr_in conAddr; // Address Of Connected Node
    struct sockaddr_in myAddr;  // Address Of This Node

    socklen_t myAddrLen;  // Size Of MyAddress
    socklen_t conAddrLen; // Size Of ConAddress

    Connection(unsigned long int IP, sockaddr_in myAddr);
    Connection(int *sock, sockaddr_in conAddr);
    ~Connection();

    void Send(char *packet);
    void Receive(char *packet);
};

// Single Network Unit
class Node
{
private:
    // Peer-Exclusive Entry Function
    void Broadcast(sockaddr_in myAddr);

protected:
    // Request Handling Interface
    RequestManager* requestManager;
    // File Handling Interface
    FileManager* fileManager;
    char fileName[512];
    // Connection Data
    vector<Connection*> connection;
    Connection* next;
    Connection* prev;
    // Address Data
    sockaddr_in address;
    socklen_t addressLen;

public:
    // Singleton Constructor
    // Node(const Node &) = delete;
    Node(char* IP, char* name);
    // Protocol
    bool Startup();
    void Run();
    void Search();
    void SetUpTerminate();
    void Terminate();
    atomic<bool> quit;
    // Connection Handling
    Connection *GetConnection(unsigned long int IP); // Searches Connection Vector For Connections To Given IP
    Connection *Connect(unsigned long int IP);       // Calls GetConnection(), but Creates A New Connection If None Exists
    Connection *Wait(unsigned long int IP);          // Wait For Connection From Specified IP
    void Disconnect(Connection*);                          // Terminate Connection
    // Request Transmission
    int  SendRequest(int request, char* context = nullptr, Connection* channel = nullptr);
    void SerializeRequest(Data*, char*);
    void ReceiveRequest(Connection*channel = nullptr);
    void DeserializeRequest(char*, Data*);
    // File Transmission
    void SendFile(char*, unsigned long int);
    void ReceiveFile(char*, int);

protected:
    // Peer-Exclusive Entry
    bool silence; // If Silence Becomes True, Instantiate Host
    // Request Access Permissions
    friend class RequestManager;
    friend class SendFile;
    friend class Test;
};

// Special Instance Of Node For Client Entry
class Host : public Node
{
public:
    Host(char *IP, char* name) : Node(IP, name) { Run(); }

    void Run();
    void Entry();
};
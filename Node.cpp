#include "Node.hpp"

using namespace std;

Connection::Connection(unsigned long int IP, struct sockaddr_in address) : 
    // sock(make_unique<int>()),  
    myAddr(address),
    isOpen(false)
{
    // Use Connections Port
    myAddr.sin_port = htons(PORT_LINK);
    // Supply Connection Information
    conAddr.sin_family = AF_INET;
    conAddr.sin_port = htons(PORT_LINK);
    conAddr.sin_addr.s_addr = IP;
    conAddrLen = sizeof(conAddr);
    // Initialize Socket
    sock = new int;
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0) 
        throw runtime_error("Connection : Failed To Initialize Socket");
    isOpen = true;
    // Set Socket Options
    int optVal = 1;
    setsockopt(*sock, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal));    // Allows IP / Port Combination To Be Reused
    // if (bind(*sock, (sockaddr*) &myAddr, sizeof(myAddr)) < 0)
    //     throw runtime_error("Connection : Failed To Bind Socket");
    // Connect Socket
    int status = connect(*sock, (struct sockaddr*) &conAddr, conAddrLen);
    if (status < 0) {
        close(*sock);
        isOpen = false;
        throw runtime_error("Connection : Failed To Connect Socket");
    }
}

Connection::Connection(int* _sock, sockaddr_in address) : 
    // sock(make_unique<int>(*_sock)),
    sock(_sock),
    conAddr(address),
    isOpen(true)
{
    // Collect Connection Information
    conAddrLen = sizeof(conAddr);
    if (getsockname(*sock, (struct sockaddr*) &myAddr, &myAddrLen) < 0) 
        throw runtime_error("Connection : Failed To Retrieve Address Information");
    myAddrLen = sizeof(myAddr);
}

Connection::~Connection() {if (isOpen) close(*sock);}

void Connection::Send(char* packet) {
    if (send(*sock, packet, FILE_SIZE, 0) < 0)
        throw runtime_error("Send : Failed");
}

void Connection::Receive(char* packet) {
    // Receive Serialized Data
    if (recv(*sock, packet, FILE_SIZE, 0) < 0)
        throw runtime_error("Receive : Failed");
    // Set Timeout
    // struct timeval timeout = { 10, 0 };    // 10 Seconds
    // fd_set bitSet;                         // Set Of Socket File Descriptors
    // FD_ZERO(&bitSet);                      // Empty Bit Set
    // FD_SET(*sock, &bitSet);                // Add Sock To Bit Set
    // // Wait For Timeout
    // return select(*sock + 1, &bitSet, nullptr, nullptr, nullptr/*&timeout*/);
}

Node::Node(char* IP, char* name) : next(nullptr), prev(nullptr) {
    // Collect Local Address Information
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT_LINK);
    inet_pton(AF_INET, IP, &address.sin_addr);
    addressLen = sizeof(address);   
    // File To Be Requested
    strcpy(fileName, name);
    // Request & File Manager Startup
    requestManager = new RequestManager(*this);
    fileManager = new FileManager("Files");
    // Wait To Die
    SetUpTerminate();
}

Connection* Node::GetConnection(unsigned long int IP) {
    // Search Connections List For Appropriate IP
    for (int i = 0; i < connection.size(); i++) {
        if (connection[i]->myAddr.sin_addr.s_addr == IP)
            return connection[i];
    }
    // If None Found
    return nullptr;
}

Connection* Node::Connect(unsigned long int IP) {
    // Search For Pre-Existing Connections
    Connection* myConnection = Node::GetConnection(IP);
    // If No Connections Found
    if (myConnection == nullptr) {
        // Create New Connection
        try {myConnection = new Connection(IP, address);}
        catch (const runtime_error &x) {
            delete myConnection;
            cerr << x.what() << endl;
            return nullptr;
        }
        // Pass New Connection To Connections Vector
        connection.push_back(myConnection);
    } 
    // Repair Chain
    if (next == nullptr) next = myConnection;
    return myConnection;
}

Connection* Node::Wait(unsigned long int IP) {
    // Connection Information
    struct sockaddr_in conAddr;
    conAddr.sin_family = AF_INET;
    conAddr.sin_port = htons(PORT_LINK);
    socklen_t conAddrLen = sizeof(conAddr);
    // Initialize Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        close(sock);
        throw runtime_error("Wait : Failed To Initialize Socket");
    }
    // Set Socket Options
    int optVal = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal));    // Allows IP / Port Combination To Be Reused
    fcntl(sock, F_SETFL, O_NONBLOCK);                                       // Allows Non-Blocking Connection
    // Bind Socket To IP / Port Combination
    if (bind(sock, (struct sockaddr*) &address, addressLen) < 0) {
        close(sock);
        throw runtime_error("Wait : Failed To Bind Connection Socket");
    }
    int newSock;
    // Wait For Connection
    if (listen(sock, 1) < 0) {
        close(sock);
        throw runtime_error("Wait : No Incoming Connections");
    }
    // Wait For Reply Until Timeout
    struct timeval timeout = { 10, 0 };  // 100 Seconds
    fd_set bitSet;                        // Set Of Socket File Descriptors
    FD_ZERO(&bitSet);                     // Empty Bit Set
    FD_SET(sock, &bitSet);                // Add Sock To Bit Set
    int retcode = select(sock + 1, &bitSet, nullptr, nullptr, &timeout);
    if (retcode < 0) {
        cout << errno << endl;
        close(sock);
        throw runtime_error("Wait : Failed To Get Reply");
    } else if (retcode == 0) {
        // If Select() Times Out  
        close(sock);
        throw runtime_error("Wait : Timed Out"); 
    }
    // Accept Connection
    newSock = accept(sock, (sockaddr*) &conAddr, &conAddrLen);
    // Verify Connection
    if (newSock < 0) {
        close(newSock);
        cout << "Wait : Connection Failed";
    }
    // Check Address Info
    else if (conAddr.sin_addr.s_addr != IP) {
        close(newSock);
        close(sock);
        throw runtime_error("Wait : Wrong Connection");
    }
    // Compile Connection Object
    Connection* newConnection;
    try {newConnection = new Connection(&newSock, conAddr);}
    catch (const runtime_error &x) 
        {cerr << x.what() << endl; return newConnection;}
    // Pass Connection To Connections Vector
    connection.push_back(newConnection);
    // Repair Chain
    if (prev == nullptr) prev = newConnection;
    return newConnection;
}   

void Node::Disconnect(Connection* con) {
    // If Forward Link To Be Terminated, Update Pointer
    if (next == con) next = nullptr;
    // Search Connecitons Vector For Match
    for (int i = 0; i < connection.size(); i++) {
        // If Match Found
        if (connection[i] == con) {
            // Delete Connection
            delete connection[i];
            // Format Vector
        }
    }
}

bool Node::Startup() {
    try {Search();}
    catch (const runtime_error &x) {
        cerr << x.what() << endl;
    }
    return !silence;
}

void Node::SetUpTerminate() {
    // Set Up Termination Input Thread
    thread terminalThread(&Node::Terminate, this);
    terminalThread.detach();
}

void Node::Run() {
    cout << "Beginning Regular Function" << endl;
    // Set Up Background Operations
    thread receive(&Node::ReceiveRequest, this, nullptr);        // Reception Thread
    thread serve(&RequestManager::ResolveRequests, requestManager);  // Request Resolution Thread
    // Wait For Connection
    while (next == nullptr || prev == nullptr) {}
    cout << "Connection Complete" << endl;
    // Request File
    int fileRequestID = SendRequest(REQUEST_SEND_FILE, fileName, next);
    cout << "File Requested : " << fileName << endl;
    // Wait For File
    if (fileRequestID > 0) {
        char file[1024];
        ReceiveFile(file, fileRequestID);
        cout << file << endl;
        while (!quit) {
            // Do Stuff
        }
    }
    // Wait For Background Processes
    receive.join();
    serve.join();
    cout << "Shutting Down" << endl;
}

void Node::Terminate() {
    string input;
    while (getline(cin, input)) {
        if (input == "q" || input == "quit") 
            break;
    }
    cout << "Terminating Processes" << endl;
    quit = true;
}

int Node::SendRequest(int request, char* context, Connection* channel) {
    // Default Implementation
    if (channel == nullptr) channel = next;
    // Wait For Connection
    while (channel == nullptr) {if (quit) return -1;}
    cout << "Sending Request" << endl;
    // Compile Data
    Data* data = new Data(request, context);
    // ID Request
    requestManager->GenerateRequest(data);
    // Serialize For Transmission
    char packet[FILE_SIZE];
    SerializeRequest(data, packet);
    // Send Over Channel
    channel->Send(packet);
    cout << "Request Sent" << endl;
    // Return Request ID
    return data->ID;
}

void Node::ReceiveRequest(Connection* channel) {
    // Default Implementation
    if (channel == nullptr) channel = prev;
    // Pause Function Until Connected
    while (channel == nullptr) {if (quit) return;}
    cout << "[Receive] Initiated" << endl;
    // Reception Loop
    while (!quit) {
        // Initialize storage
        Data* data = new Data;
        char packet[FILE_SIZE];
        // Receive From Channel
        channel->Receive(packet);
        // Terminal Condition
        if (quit) break;
        // Extract Request
        DeserializeRequest(packet, data);
        requestManager->DecodeRequest(data);
    }
    cout << "[Receive] Terminated" << endl;
}

void Node::SerializeRequest(struct Data* data, char* packet) {
    // Test
    // cout << data->ID << endl
    //      << data->source << endl
    //      << data->request << endl
    //      << data->context << endl
    //      << data->timeout << endl;
    // Serialize Numerical Values
    int* num = (int*) packet;
    *num = data->ID;      num++;
    *num = data->timeout; num++;
    *num = data->request; num++;
    // Serialize Context
    char* txt = (char*) num;
    for (int i = 0; i < 512; i++, txt++)
        *txt = data->context[i];
    // Serialize Source IP
    long int* addr = (long int*) txt;
    *addr = data->source;
}

void Node::DeserializeRequest(char* packet, struct Data* data) {
    // Deserialize Numerical Values
    int* num = (int*) packet;
    data->ID = *num;      num++;
    data->timeout = *num; num++;
    data->request = *num; num++;
    // Deserialize Context
    char* txt = (char*) num;
    for (int i = 0; i < 512; i++, txt++) 
        *(data->context + i) = *txt;
    // Deserialize Source IP
    long int* addr = (long int*) txt;
    data->source = *addr;
}

void Node::SendFile(char* file, unsigned long int IP) {
    // Supply Connection Information
    struct sockaddr_in conAddr;
    conAddr.sin_family = AF_INET;
    conAddr.sin_port = htons(PORT_TRANSFER);
    conAddr.sin_addr.s_addr = IP;
    // Initialize Socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        close(sock);
        throw runtime_error("SendFile : Failed To Bind Socket");
    }
    // // Set Socket Options
    // int optVal = 1;
    // if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal)) < 0) {
    //     close(sock);
    //     throw runtime_error("SendFile : Failed To Set Socket Options");
    // }
    // Send File
    if (sendto(sock, file, FILE_SIZE, 0, (struct sockaddr*) &conAddr, sizeof(conAddr)) < 0) {
        close(sock);
        throw runtime_error("SendFile : Failed To Send File ");
    }
    // Close Socket
    close(sock);
}

void Node::ReceiveFile(char* file, int requestID) {
    cout << "[FileWait] Initiated" << endl; 
    try {
    while (!quit || requestManager->GetRequestIndex(requestID) != nullopt) {
            // Supply Connection Information
            struct sockaddr_in conAddr;
            conAddr.sin_family = AF_INET;
            conAddr.sin_port = htons(PORT_TRANSFER);
            socklen_t conAddrLen = sizeof(conAddr);
            // Initialize Socket
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) {
                close(sock);
                throw runtime_error("SendFile : Failed To Bind Socket");
            }
            // Set Socket Options
            int optVal = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal)) < 0) {
                close(sock);
                throw runtime_error("SendFile : Failed To Set Socket Options");
            }
            // Bind Socket
            struct sockaddr_in tempAddr = address;
            tempAddr.sin_port = htons(PORT_TRANSFER);
            if (bind(sock, (struct sockaddr*) &tempAddr, sizeof(tempAddr))) {
                close(sock);
                throw runtime_error("ReceiveFile : Failed To Bind Socket");
            }
            // Set Timer To Check Termination Condition
            struct timeval checkTime = {2, 0};    // 2 Seconds
            fd_set bitSet;                        // Set Of Socket File Descriptors
            FD_ZERO(&bitSet);                     // Empty Bit Set
            FD_SET(sock, &bitSet);                // Add Sock To Bit Set
            int retcode = select(sock + 1, &bitSet, nullptr, nullptr, &checkTime);
            // Handle Exceptions
            switch (retcode) {
                case 0:
                    // Timeout
                    continue;
                case -1:
                    throw runtime_error("ReceiveFile : Select Failed");
                    break;
                default:
                    // Receive File
                    if (recvfrom(sock, file, FILE_SIZE, 0, (struct sockaddr*) &conAddr, (socklen_t*) &conAddrLen) < 0) {
                        close(sock);
                        throw runtime_error("ReceiveFile : Failed To Receive File");
                    }
            }
        }
        // Remove Request From Pending Vector
        requestManager->RemoveRequest(requestID);
    }
    catch(const runtime_error &x) 
        {cerr << "[FileWait] " << x.what() << endl;}
    cout << "[FileWait] Terminated" << endl;
}

void Node::Broadcast(struct sockaddr_in myAddr) {
    cout << "[Broadcast] Initiated" << endl;
    // Supply Search Parameters
    struct sockaddr_in conAddr;
    conAddr.sin_family = AF_INET;
    conAddr.sin_port = htons(PORT_ENTRY);
    myAddr.sin_port  = htons(PORT_ENTRY);
    // Initialize Broadcast Socket
    int broadcast = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcast < 0) {
        close(broadcast);
        throw runtime_error("Search : Failed To Initialize Broadcast Socket");
    }
    // Set Socket Options
    int optValue = 1;
    if (setsockopt(broadcast, SOL_SOCKET, SO_BROADCAST, &optValue, sizeof(optValue)) < 0) {      // Allows Socket To Broadcast Periodic Messages To Subnet 
        close(broadcast);
        throw runtime_error("Search : Failed To Set Broadcast Option");
    }
    // Broadcast Join Request
    inet_pton(AF_INET, "10.7.153.255", &conAddr.sin_addr);   // Broadcast To All
    char conIP[256] = {};   // Our IP Address
    inet_ntop(AF_INET, (struct addr_in*) &myAddr.sin_addr, conIP, sizeof(conIP));  // Supplies IP Information
    int check;
    while (!silence && !quit) 
        {check = sendto(broadcast, conIP, strlen(conIP) + 1, 0, (struct sockaddr*) &conAddr, sizeof(conAddr));}   // Send To All Machines On Subnet
    close(broadcast);
    if (check < 0) throw runtime_error("Search : Broadcast Failed");
    cout << "[Broadcast] Terminated" << endl;
}

void Node::Search() {
    // Broadcast Entry Request
    thread broadcastThread(&Node::Broadcast, this, address);
    broadcastThread.detach();
    // MUST BE STATIC FUNCITON TO THREAD FOLLOW UP ON THIS
    // Initialize Connection Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        close(sock);
        throw runtime_error("Search : Failed To Initialize Connection Socket");
    }
    // Set Socket Options
    int optVal = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal));    // Allows IP / Port Combination To Be Reused
    fcntl(sock, F_SETFL, O_NONBLOCK);                                       // Allows Non-Blocking Connection
    // Bind Socket To IP / Port Combination
    if (bind(sock, (struct sockaddr*) &address, addressLen) < 0) {
        close(sock);
        throw runtime_error("Search : Failed To Bind Connection Socket");
    }
    // Test For Reply
    cout << "Searching For Entry Point" << endl;
    int temp = listen(sock, 1);
    if (temp < 0) {
        close(sock);
        throw runtime_error("Search : Failed To Get Reply");
    }
    // Wait For Reply Until Timeout
    struct timeval timeout = { 5, 0 };  // 10 Seconds
    fd_set bitSet;                       // Set Of Socket File Descriptors
    FD_ZERO(&bitSet);                    // Empty Bit Set
    FD_SET(sock, &bitSet);               // Add Sock To Bit Set
    int retcode = select(sock + 1, &bitSet, nullptr, nullptr, &timeout);
    if (retcode < 0) {
        cout << errno << endl;
        close(sock);
        throw runtime_error("Search : Failed To Get Reply");
    } else if (retcode == 0) {
        // If Select() Times Out  
        close(sock);
        silence = true;
        throw runtime_error("Search : Timed Out"); 
    }
    cout << "Found Host" << endl 
         << "Waiting For Connection Request" << endl;
    // Accept Connection
    struct sockaddr_in conAddr;
    conAddr.sin_family = AF_INET;
    conAddr.sin_port = htons(PORT_ENTRY);
    conAddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t conAddrLen = sizeof(conAddr);
    int newSock = accept(sock, (struct sockaddr*) &conAddr, &conAddrLen);
    if (newSock < 0) {
        close(newSock);
        close(sock);
        throw runtime_error("Search : Connection Failed");
    }
    cout << "Connected" << endl;
    silence = true;
    sleep(1);
    silence = false;
    // Compile Connection Object
    Connection* newConnection; 
    try {newConnection = new Connection(&newSock, conAddr);}
    catch (const runtime_error &x) 
        {cerr << "Search : " << x.what() << endl; return;}
    // Pass Connection To Connections Vector
    prev = newConnection;
    connection.push_back(prev);
}

void Host::Run() {
    thread entry(&Host::Entry, this);
    Node::Run();
    entry.join();
}

void Host::Entry() {
    cout << "[Entry] Initiated" << endl;
    // Supply Search Parameters
    struct sockaddr_in conAddr;
    conAddr.sin_family = AF_INET;
    conAddr.sin_port = htons(PORT_ENTRY);
    socklen_t conAddrLen = sizeof(conAddr);
    // Initialize Socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        close(sock);
        throw runtime_error("Entry : Failed To Initialize Socket");
    }
    // Set Socket Options
    int optValue = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optValue, sizeof(optValue));       // Allows IP / Port Combination To Be Reused
    // Bind Socket To IP / Port Combination
    struct sockaddr_in tempAddr;
    tempAddr.sin_family = AF_INET;
    tempAddr.sin_port = htons(PORT_ENTRY);
    tempAddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t tempAddrLen = sizeof(tempAddr);
    if (bind(sock, (struct sockaddr*) &tempAddr, tempAddrLen) < 0) {
        close(sock);
        throw runtime_error("Entry : Failed To Bind Socket");
    }
    // Wait For Connections
    char conIP[256];
    cout << "[Entry] Waiting For Entry Requests" << endl;
    while (!quit) {
        // Listen For Entry Request Broadcast
        int retcode = recvfrom(sock, conIP, sizeof(conIP) - 1, MSG_DONTWAIT, (struct sockaddr*) &conAddr, &conAddrLen);
        if (retcode <= 0 && !quit) {continue;}
        // Check For Pre-Existing Connection
        bool match = false;
        for (int i = 0 ; i < connection.size(); i++) {
            // Match Found
            if (inet_addr(conIP) == connection[i]->conAddr.sin_addr.s_addr) 
                match = true;       
        }
        // If Connection Already Exists, Ignore Request
        if (match) continue;
        // Otherwise Move On
        cout << "[Entry] Received Request From " << conIP << endl
             << "[Entry] Attempting Connection " << endl;
        // Establish TCP Connection
        Connection* newConnection;
        try {newConnection = Connect(inet_addr(conIP));}
        catch (const runtime_error &x) {
            delete newConnection;
            close(sock);
            cerr << "[Entry] " << x.what() << endl;
        }
        cout << "[Entry] Connected To " << conIP << endl;
        // Repair Chain
        if (prev == nullptr) {
            try {
                char myIP[256];
                inet_ntop(AF_INET, (struct addr_in*) &address.sin_addr, myIP, sizeof(myIP));
                SendRequest(REQUEST_CONNECT_TO, myIP, newConnection);
                Wait(newConnection->conAddr.sin_addr.s_addr);
            } catch(const runtime_error &x) {
                cerr << "[Entry] " << x.what() << endl; 
                // close(sock);
                quit = true;
            }
        }
    }
    cout << "[Entry] Terminated" << endl;
}
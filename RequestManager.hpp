#include "FileManager.cpp"

#include <deque>
#include <vector>

using namespace std;

// Data Format To Be Sent
struct Data {
    // Identification Info
    int ID;
    int timeout;
    unsigned long int source;       // IP of Sender
    // Content
    int   request;                  // Enumerated Request
    char* context;                  // Contextual Information

    Data(int _request = REQUEST_NULL, char* _context = nullptr);
};

// Forward Declaration
class Node;

// Request Base Class
class Request {
    protected:
        Data data;
    public:
        Request(Data _data) : data(_data) {}
       ~Request() = default;
        
        virtual void Serve(Node&) = 0;      // Execute Requested Actions
        virtual void Forward(Node&) = 0;    // Forward To Next Node

        int ID() {return data.ID;};
        unsigned long int Source() {return data.source;}
};

// Node Request Handler
class RequestManager {
    private:
        // Parent Node
        Node& node;
        // Request Data
        vector<int> pending;
        deque<Request*> handling;
    public:
        RequestManager(Node& _node) : node(_node) {} 
        // Request Generation & Resolution
        void GenerateRequest(Data*);
        void DecodeRequest(Data*);
        // Request Processing
        void ResolveRequests();
        void AddNewRequest(Request*);   // Adds Request To Handling Deque
        void RemoveRequest(int);        // Removes Request From Pending Vector
optional<int>GetRequestIndex(int);      // Returns Index Of Pending Request, Given ID
};
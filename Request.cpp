#include "Request.hpp"

Data::Data(int _request, char* _context) : 
    request(_request),
    timeout(11) 
{
    if (_context == nullptr) context = (char*) malloc(FILE_SIZE * sizeof(char));
    else context = _context;
}

void RequestManager::ResolveRequests() {
    cout << "[Resolve] Initiated" << endl;
    while (!node.quit) {
        // If Unresolved Requests Exist
        if (!handling.empty()) {
            // If Request Originated Here
            if (handling.front()->Source() == node.address.sin_addr.s_addr) {
                // Check If Pending
                for (int i = 0; i < pending.size(); i++) {
                    // If Match Found, Discard And Move On
                    if (handling.front()->ID() == pending[i]) {
                        RemoveRequest(handling.front()->ID());
                        continue;
                    }
                }
            }
            // Resolve Request
            try {handling.front()->Serve(node);}
            catch (const runtime_error &x) 
                {cerr << "[Resolve] " << x.what() << endl;}
            handling.pop_front();
        }
    }
    cout << "[Resolve] Terminated" << endl;
}

void RequestManager::GenerateRequest(struct Data* data) {
    // Supply Data ID Info
    data->ID = 0;
    for (int i = 0; i < pending.size(); i++) {
        if (pending[i] > data->ID)
            data->ID = pending[i];
    }
    data->ID++;
    // Sign Data
    data->source = node.address.sin_addr.s_addr;
}

void RequestManager::DecodeRequest(struct Data* data) {
    // Extract Instructions
    switch(data->request) {
        case REQUEST_NULL: break;
        case REQUEST_TEST: {
            Test* newRequest = new Test(*data);
            handling.push_back(newRequest);
            break;
        }
        case REQUEST_CONNECT_TO: {
            ConnectTo* newRequest = new ConnectTo(*data);
            handling.push_back(newRequest);
            break;
        }
        case REQUEST_WAIT_FOR: {
            WaitFor* newRequest = new WaitFor(*data);
            handling.push_back(newRequest);
            break;
        }
        case REQUEST_SEND_FILE: {
            SendFile* newRequest = new SendFile(*data);
            handling.push_back(newRequest);
            break;
        }
        default:
            throw runtime_error("Decode : Invalid Request Type");
    }
}

void RequestManager::AddNewRequest(Request* newRequest) {
        if (newRequest != nullptr)
            handling.push_back(newRequest);
}

void RequestManager::RemoveRequest(int ID) {
    // Search Pending Requests Vector For Match
    for (int i = 0; i < pending.size(); i++) {
        // If Found
        if (pending[i] == ID) {
            // Erase
            pending.erase(pending.begin() + i);
            break;
        }
    }
}

optional<int> RequestManager::GetRequestIndex(int ID) {
    // Search Pending Requests Vector For Match
    for (int i = 0; i < pending.size(); i++) {
        // If Found, Return Index
        if (pending[i] == ID) return i;
    }
    // If No Match
    return nullopt;
}

void Test::Serve(Node& node) {cout << "[Test] " << data.context << endl;}

void ConnectTo::Serve(Node& node) {
    // Convert Context To Address
    unsigned long int IP;                   // Typedef In_Addr
    inet_pton(AF_INET, data.context, &IP);  // Converts Standard Text Format IP To Binary
    // Attempt Connection To Address
    node.Connect(data.source);
}

void WaitFor::Serve(Node& node) {
    // Convert Context To Address
    unsigned long int IP;                   // Typedef In_Addr
    inet_pton(AF_INET, data.context, &IP);  // Converts Standard Text Format IP To Binary
    // Wait For Connection Request From Address
    node.Wait(IP);
}

void SendFile::Serve(Node& node) {
    // If Node Sent This Request
    if (data.source == node.address.sin_addr.s_addr) {
        // Remove From Pending
        node.requestManager->RemoveRequest(data.ID);
        return;
    }
    // Otherwise, Check For Requested File
    char file[FILE_SIZE];
    node.fileManager->GetFile(data.context, file, (size_t) FILE_SIZE);
    // If File Not Found
    if (file == nullptr) {
        // Forward To Next Node
        Forward(node);
        return;
    }
    // Otherwise, Serve Request
    try {node.SendFile(file, data.source);}
    catch (const runtime_error &x) 
        {cerr << x.what() << endl;}
}

void Test::Forward(Node& node)      {node.SendRequest(REQUEST_TEST, data.context);}
void ConnectTo::Forward(Node& node) {node.SendRequest(REQUEST_CONNECT_TO, data.context);}
void WaitFor::Forward(Node& node)   {node.SendRequest(REQUEST_WAIT_FOR, data.context);}
void SendFile::Forward(Node& node)  {node.SendRequest(REQUEST_SEND_FILE, data.context);}
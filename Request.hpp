#include "Node.cpp"

using namespace std;

class Test : public Request {
    private:
    public:
        Test(Data data) : Request(data) {}
        virtual void Serve(Node&);
        virtual void Forward(Node&);
};

class ConnectTo : public Request {
    private:
    public:
        ConnectTo(Data data) : Request(data) {}
        virtual void Serve(Node&);
        virtual void Forward(Node&);
};

class WaitFor : public Request {
    private:
    public:
        WaitFor(Data data) : Request(data) {}
        virtual void Serve(Node&);
        virtual void Forward(Node&);
};

class SendFile : public Request {
    private:
    public:
        SendFile(Data data) : Request(data) {}
        virtual void Serve(Node&);
        virtual void Forward(Node&);
};
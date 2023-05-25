#include "Request.cpp"

using namespace std;

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) throw runtime_error("Invalid Arguments");
    
        Node* peer = new Node(argv[1], argv[2]);
        if (!peer->Startup()) {
            delete peer;
            cout << "Hosting " << endl;
            sleep(3);
            Host* host = new Host(argv[1], argv[2]);
        } else {peer->Run();}
    } catch(runtime_error &x) {
        cout << endl << x.what() << endl;
    }

    return 0;
}
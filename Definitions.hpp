// Ports
#define PORT_LINK     6666
#define PORT_ENTRY    5555
#define PORT_TRANSFER 4444
// File Information
#define FILE_SIZE 1024
// Request Codes
enum REQUEST {
    REQUEST_NULL,
    REQUEST_TEST,
    REQUEST_CONNECT_TO,
    REQUEST_WAIT_FOR,
    REQUEST_SEND_FILE
};
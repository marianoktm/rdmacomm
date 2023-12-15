//
// Created by Mariano Aponte on 11/12/23.
//

#include <iostream>
#include "../../include/RdmaCommunicator.h"

#define IP "192.168.4.99"
#define PORT "9988"

int main() {
    try {
        // Create a communicator
        auto comm = new RdmaCommunicator(IP, PORT);

        // Try to connect to the server
        comm->Connect();

        // RDMA Write
        char * msg = "hello from client!";
        comm->Write(msg, strlen(msg));

        // RDMA Read
        char * recvmsg = static_cast<char *>(calloc(40, sizeof(char)));
        comm->Read(recvmsg, 40);
        std::cout << "Message from the server: " << recvmsg << std::endl;
    }
    catch (std::string & exc) {
        std::cerr << exc << std::endl;
    }
    catch (const char * exc) {
        std::cerr << exc << std::endl;
    }
}
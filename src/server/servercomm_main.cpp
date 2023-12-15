//
// Created by Mariano Aponte on 11/12/23.
//

#include <iostream>
#include "../../include/RdmaCommunicator.h"

#define IP "192.168.4.99"
#define PORT "9988"

int main() {
    try {
        // Create the communicator
        auto comm = new RdmaCommunicator(IP, PORT);

        // Set the communicator as a server and put it in a listen state
        comm->Serve();

        // Accept an incoming connection and create a new communicator to handle it
        auto cli = const_cast<Communicator *>(comm->Accept());

        // RDMA Read: as a server it actually registers the buffer as rdma-write ready
        char * recvmsg = static_cast<char *>(calloc(40, sizeof(char)));
        cli->Read(recvmsg, 40);
        std::cout << "Message from the client: " << recvmsg << std::endl;

        // RDMA Write: as a server it actually registers the buffer as rdma-read ready
        char * msg = "hello from server!";
        cli->Write(msg, strlen(msg));
    }
    catch (std::string & exc) {
        std::cerr << exc << std::endl;
    }
    catch (const char * exc) {
        std::cerr << exc << std::endl;
    }
}
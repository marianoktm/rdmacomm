//
// Created by Mariano Aponte on 07/12/23.
//

#ifndef RDMACM_RDMACOMMUNICATOR_H
#define RDMACM_RDMACOMMUNICATOR_H

#include "Communicator.h"
#include "ktmrdma.h"
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <string>
#include <netdb.h>

#define BACKLOG 8
#define BUF_SIZE 1024
#define DEBUG

class RdmaCommunicator : public Communicator {
private:
    // Useful data for communication and initialization
    rdma_cm_id * rdmaCmId;
    rdma_cm_id * rdmaCmListenId;

    char * hostname;
    char * port;

    //ibv_mr * memoryRegion;

    bool isServing = false;

public:
    RdmaCommunicator() = default;
    RdmaCommunicator(char * hostname, char * port);
    RdmaCommunicator(rdma_cm_id * rdmaCmId, bool isServing = true);

    void Serve();
    const Communicator *const Accept() const;

    void Connect();

    size_t Read(char * buffer, size_t size);
    size_t Write(const char * buffer, size_t size);

    void Sync();
    void Close();

    std::string to_string() { return "rdmacommunicator"; }
};


#endif //RDMACM_RDMACOMMUNICATOR_H

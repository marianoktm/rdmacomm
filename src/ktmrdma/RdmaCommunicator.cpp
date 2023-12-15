//
// Created by Mariano Aponte on 07/12/23.
//

#include <iostream>
#include <sstream>
#include "../../include/RdmaCommunicator.h"
#include "../../include/utils.h"

RdmaCommunicator::RdmaCommunicator(char * hostname, char * port) {
    if (port == nullptr or std::string(port).empty()) {
        throw "RdmaCommunicator: Port not specified...";
    }

    hostent *ent = gethostbyname(hostname);
    if (ent == NULL) {
        throw "RdmaCommunicator: Can't resolve hostname \"" + std::string(hostname) + "\"...";
    }

    this->hostname = hostname;
    this->port = port;
}

RdmaCommunicator::RdmaCommunicator(rdma_cm_id *rdmaCmId, bool isServing) {
    this->rdmaCmId = rdmaCmId;
    this->isServing = isServing;
}

void RdmaCommunicator::Serve() {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Serve(): called." << std::endl;
#endif

    // Setup address info
    rdma_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags = RAI_PASSIVE;

    rdma_addrinfo * rdmaAddrinfo;
    ktm_rdma_getaddrinfo(nullptr, port, &hints, &rdmaAddrinfo);

    // Create communication manager id with queue pair attributes
    ibv_qp_init_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(qpInitAttr));

    qpInitAttr.cap.max_send_wr = 20;
    qpInitAttr.cap.max_recv_wr = 20;
    qpInitAttr.cap.max_send_sge = 20;
    qpInitAttr.cap.max_recv_sge = 20;
    qpInitAttr.sq_sig_all = 1;

    ktm_rdma_create_ep(&rdmaCmListenId, rdmaAddrinfo, nullptr, &qpInitAttr);
    rdma_freeaddrinfo(rdmaAddrinfo);

    // Listen for connections
    ktm_rdma_listen(rdmaCmListenId, BACKLOG);

    isServing = true;
}

const Communicator *const RdmaCommunicator::Accept() const {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Accept(): called." << std::endl;
#endif
    rdma_cm_id * clientRdmaCmId;
    ktm_rdma_get_request(rdmaCmListenId, &clientRdmaCmId);
    ktm_rdma_accept(clientRdmaCmId, nullptr);
    return new RdmaCommunicator(clientRdmaCmId);
}

void RdmaCommunicator::Connect() {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Connect(): called." << std::endl;
#endif
    // Setup address info
    rdma_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;

    rdma_addrinfo * rdmaAddrinfo;
    ktm_rdma_getaddrinfo(hostname, port, &hints, &rdmaAddrinfo);

    // Create communication manager id with queue pair attributes
    ibv_qp_init_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(qpInitAttr));

    qpInitAttr.cap.max_send_wr = 20;
    qpInitAttr.cap.max_recv_wr = 20;
    qpInitAttr.cap.max_send_sge = 20;
    qpInitAttr.cap.max_recv_sge = 20;
    qpInitAttr.sq_sig_all = 1;

    ktm_rdma_create_ep(&rdmaCmId, rdmaAddrinfo, nullptr, &qpInitAttr);
    rdma_freeaddrinfo(rdmaAddrinfo);

    ktm_rdma_connect(rdmaCmId, nullptr);
}

/*
size_t RdmaCommunicator::Read(char *buffer, size_t size) {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Read(): called." << std::endl;
#endif
    // If I'm client, I want to read into buffer from server
    // If I'm server, I want to set buffer to the client to write

    char * rdmaExcBuffer = static_cast<char *>(malloc(BUF_SIZE));
    ibv_mr * rdmaExcMr = ktm_rdma_reg_msgs(rdmaCmId, rdmaExcBuffer, BUF_SIZE);

    uintptr_t peer_address = 0;
    uint32_t peer_rkey = 0;

    ibv_mr * memoryRegion;

#ifdef DEBUG
    std::cout << tab << "EXCHANGING RDMA INFORMATIONS..." << std::endl;
#endif
    if (isServing) {
        // If I'm server, I want to set buffer to the client to write
        memoryRegion = ktm_rdma_reg_write(rdmaCmId, buffer, size);

        ktm_server_exchange_rdma_info(rdmaCmId, rdmaExcBuffer, BUF_SIZE, rdmaExcMr, &peer_address, &peer_rkey, memoryRegion);
#ifdef DEBUG
        std::cout << doubletab << "server: " << "using peer address: " << peer_address << "(" << (void * ) peer_address << ") and peer rkey: " << peer_rkey << std::endl;
        std::cout << doubletab << "the buffer is now rdma-write ready." << std::endl;
#endif
    }
    else {
        // If I'm client, I want to read into buffer from server
        memoryRegion = ktm_rdma_reg_msgs(rdmaCmId, buffer, size);

        ktm_client_exchange_rdma_info(rdmaCmId, rdmaExcBuffer, BUF_SIZE, rdmaExcMr, &peer_address, &peer_rkey, memoryRegion);
#ifdef DEBUG
        std::cout << doubletab << "client: " << "using peer address: " << "(" << (void * ) peer_address << ") and peer rkey: " << peer_rkey << std::endl;
        std::cout << doubletab << "the buffer is now ready to read from server." << std::endl;
#endif
    }
#ifdef DEBUG
    std::cout << tab << "DONE! (exchanging rdma informations)" << std::endl;
#endif

#ifdef DEBUG
    std::cout << tab <<  "self address: " << (uintptr_t) memoryRegion->addr << "(" << memoryRegion->addr << "), self rkey: " << memoryRegion->rkey << std::endl;
#endif

    rdma_dereg_mr(rdmaExcMr);
    free(rdmaExcBuffer);

    int num_wc;
    ibv_wc workCompletion;
    memset(&workCompletion, 0, sizeof(workCompletion));

    char * completionMsg = static_cast<char *>(calloc(1, sizeof(char)));
    ibv_mr * completionMr = ktm_rdma_reg_msgs(rdmaCmId, completionMsg, 1);

#ifdef DEBUG
    std::cout << tab << "PERFORMING READ..." << std::endl;
#endif
    if (isServing) {
        // If I'm server, I signal I'm ready for RDMA write, then wait for client's RDMA write completion
        ktm_rdma_post_send(rdmaCmId, nullptr, completionMsg, 1, completionMr, 0);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

        ktm_rdma_post_recv(rdmaCmId, nullptr, completionMsg, 1, completionMr);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);
    }
    else {
        // If I'm client, I wait the server signal, then I RDMA read to server and then send a completion
        ktm_rdma_post_recv(rdmaCmId, nullptr, completionMsg, 1, completionMr);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);

        ktm_rdma_post_read(rdmaCmId, nullptr, buffer, size, memoryRegion, IBV_WR_RDMA_READ, peer_address, peer_rkey);

        ktm_rdma_post_send(rdmaCmId, nullptr, completionMsg, 1, completionMr, 0);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);
    }
#ifdef DEBUG
    std::cout << tab << "DONE! (performing read)" << std::endl;
#endif

    rdma_dereg_mr(memoryRegion);
    rdma_dereg_mr(completionMr);

    return 1;
}

size_t RdmaCommunicator::Write(const char *buffer, size_t size) {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Write(): called." << std::endl;
#endif
    // If I'm client, I want to write buffer to the server
    // If I'm server, I want to set buffer to the client to read

    char * rdmaExcBuffer = static_cast<char *>(malloc(BUF_SIZE));
    ibv_mr * rdmaExcMr = ktm_rdma_reg_msgs(rdmaCmId, rdmaExcBuffer, BUF_SIZE);

    char * actualBuffer = static_cast<char *>(calloc(size, sizeof(char)));
    memcpy(actualBuffer, buffer, size);

    uintptr_t peer_address = 0;
    uint32_t peer_rkey = 0;

    ibv_mr * memoryRegion;

#ifdef DEBUG
    std::cout << tab << "EXCHANGING RDMA INFORMATIONS..." << std::endl;
#endif
    if (isServing) {
        // If I'm server, I want to set buffer to the client to read
        memoryRegion = ktm_rdma_reg_read(rdmaCmId, (void *) actualBuffer, size);

        ktm_server_exchange_rdma_info(rdmaCmId, rdmaExcBuffer, BUF_SIZE, rdmaExcMr, &peer_address, &peer_rkey, memoryRegion);
#ifdef DEBUG
        std::cout << doubletab << "server: " << "using peer address: " << peer_address << " (" << (void *) peer_address << ") and peer rkey: " << peer_rkey << std::endl;
        std::cout << doubletab << "the buffer is now rdma-read ready." << std::endl;
        std::cout << doubletab << "buffer: <<<" << actualBuffer << ">>>" << std::endl;
#endif
    }
    else {
        // If I'm client, I want to write buffer to the server
        memoryRegion = ktm_rdma_reg_msgs(rdmaCmId, (void *) actualBuffer, size);

        ktm_client_exchange_rdma_info(rdmaCmId, rdmaExcBuffer, BUF_SIZE, rdmaExcMr, &peer_address, &peer_rkey, memoryRegion);
#ifdef DEBUG
        std::cout << doubletab << "client: " << "using peer address: " << peer_address << " (" << (void * ) peer_address << ") and peer rkey: " << peer_rkey << std::endl;
        std::cout << doubletab << "the buffer is now ready to be written." << std::endl;
        std::cout << doubletab << "buffer: <<<" << actualBuffer << ">>>" << std::endl;
#endif
    }
#ifdef DEBUG
    std::cout << tab << "DONE! (exchanging rdma informations)" << std::endl;
#endif

#ifdef DEBUG
    std::cout << tab << "self address: " << (uintptr_t) memoryRegion->addr << "(" << memoryRegion->addr << "), self rkey: " << memoryRegion->rkey << std::endl;
    std::cout << tab << "actualBuffer address: " << (void * ) actualBuffer << " (" << (uintptr_t) actualBuffer << ")" << std::endl;
#endif

    rdma_dereg_mr(rdmaExcMr);
    free(rdmaExcBuffer);

    int num_wc;
    ibv_wc workCompletion;
    memset(&workCompletion, 0, sizeof(workCompletion));

    char * completionMsg = static_cast<char *>(calloc(1, sizeof(char)));
    ibv_mr * completionMr = ktm_rdma_reg_msgs(rdmaCmId, completionMsg, 1);

#ifdef DEBUG
    std::cout << tab << "PERFORMING WRITE..." << std::endl;
#endif
    if (isServing) {
        // If I'm server, I signal I'm ready for RDMA read, then wait for client's RDMA read completion
        ktm_rdma_post_send(rdmaCmId, nullptr, completionMsg, 1, completionMr, 0);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

        ktm_rdma_post_recv(rdmaCmId, nullptr, completionMsg, 1, completionMr);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);
    }
    else {
        // If I'm client, I wait the server signal, then I RDMA write to server and then send a completion
        ktm_rdma_post_recv(rdmaCmId, nullptr, completionMsg, 1, completionMr);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);

        ktm_rdma_post_write(rdmaCmId, nullptr, actualBuffer, size, memoryRegion, IBV_WR_RDMA_WRITE, peer_address, peer_rkey);

        ktm_rdma_post_send(rdmaCmId, nullptr, completionMsg, 1, completionMr, 0);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);
    }
#ifdef DEBUG
    std::cout << tab << "DONE! (performing write)" << std::endl;
#endif

    rdma_dereg_mr(memoryRegion);
    rdma_dereg_mr(completionMr);

    return 1;
}
*/

size_t RdmaCommunicator::Read(char *buffer, size_t size) {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Read(): called." << std::endl;
#endif

    // registering the buffer to write state
    ibv_mr * memoryRegion = ktm_rdma_reg_write(rdmaCmId, buffer, size);

    int num_wc;
    ibv_wc workCompletion;
    memset(&workCompletion, 0, sizeof(workCompletion));

    // SENDING WRITE MEMORY INFO TO THE PEER
    char * rdmaExcBuffer = static_cast<char *>(malloc(BUF_SIZE));
    ibv_mr * rdmaExcMr = ktm_rdma_reg_msgs(rdmaCmId, rdmaExcBuffer, BUF_SIZE);

    uintptr_t self_address = (uintptr_t) memoryRegion->addr;
    uint32_t self_rkey = memoryRegion->rkey;

#ifdef DEBUG
    std::cout << "using self address: " << self_address << " and self rkey: " << self_rkey << std::endl;
#endif

    // sending address info
    memcpy(rdmaExcBuffer, &self_address, sizeof(self_address));
    ktm_rdma_post_send(rdmaCmId, nullptr, rdmaExcBuffer, sizeof(self_address), rdmaExcMr, IBV_SEND_INLINE);
    num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

    // sending rkey info
    memcpy(rdmaExcBuffer, &self_rkey, sizeof(self_rkey));
    ktm_rdma_post_send(rdmaCmId, nullptr, rdmaExcBuffer, sizeof(self_rkey), rdmaExcMr, IBV_SEND_INLINE);
    num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

    // some cleanup
    rdma_dereg_mr(rdmaExcMr);
    free(rdmaExcBuffer);

    // message for signaling
    char * completionMsg = static_cast<char *>(calloc(1, sizeof(char)));
    ibv_mr * completionMr = ktm_rdma_reg_msgs(rdmaCmId, completionMsg, 1);

    // signal ready to be written
    ktm_rdma_post_send(rdmaCmId, nullptr, completionMsg, 1, completionMr, IBV_SEND_INLINE);
    num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

    // (writing happens now)

    // wait peer to end writing
    ktm_rdma_post_recv(rdmaCmId, nullptr, completionMsg, 1, completionMr);
    num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);

    // some cleanup
    rdma_dereg_mr(memoryRegion);
    rdma_dereg_mr(completionMr);
    free(completionMsg);

    return 1;
}

size_t RdmaCommunicator::Write(const char *buffer, size_t size) {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Write(): called." << std::endl;
#endif

    char * actualBuffer = (char *) malloc(size);
    memcpy(actualBuffer, buffer, size);
    ibv_mr * memoryRegion = ktm_rdma_reg_msgs(rdmaCmId, actualBuffer, size);

    int num_wc;
    ibv_wc workCompletion;
    memset(&workCompletion, 0, sizeof(workCompletion));

    // GETTING WRITE MEMORY INFO FROM THE PEER
    char * rdmaExcBuffer = static_cast<char *>(malloc(BUF_SIZE));
    ibv_mr * rdmaExcMr = ktm_rdma_reg_msgs(rdmaCmId, rdmaExcBuffer, BUF_SIZE);

    uintptr_t peer_address = -1;
    uint32_t peer_rkey = -1;

    // getting address
    ktm_rdma_post_recv(rdmaCmId, nullptr, rdmaExcBuffer, sizeof(peer_address), rdmaExcMr);
    num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);
    memcpy(&peer_address, rdmaExcBuffer, sizeof(peer_address));

    // getting rkey
    ktm_rdma_post_recv(rdmaCmId, nullptr, rdmaExcBuffer, sizeof(peer_rkey), rdmaExcMr);
    num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);
    memcpy(&peer_rkey, rdmaExcBuffer, sizeof(peer_rkey));

#ifdef DEBUG
    std::cout << "using peer address: " << peer_address << " and peer rkey: " << peer_rkey << std::endl;
#endif

    // some cleanup
    rdma_dereg_mr(rdmaExcMr);
    free(rdmaExcBuffer);

    // message for signaling
    char * completionMsg = static_cast<char *>(calloc(1, sizeof(char)));
    ibv_mr * completionMr = ktm_rdma_reg_msgs(rdmaCmId, completionMsg, 1);

    // wait signal to write
    ktm_rdma_post_recv(rdmaCmId, nullptr, completionMsg, 1, completionMr);
    num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);

    // write
    ktm_rdma_post_write(rdmaCmId, nullptr, actualBuffer, size, memoryRegion, IBV_WR_RDMA_WRITE, peer_address, peer_rkey);
    num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

    // signal write ended
    ktm_rdma_post_send(rdmaCmId, nullptr, completionMsg, 1, completionMr, IBV_SEND_INLINE);
    num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

    // some cleanup
    rdma_dereg_mr(memoryRegion);
    rdma_dereg_mr(completionMr);
    free(actualBuffer);
    free(completionMsg);

    return 1;
}

void RdmaCommunicator::Sync() {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Sync(): called." << std::endl;
#endif
}

void RdmaCommunicator::Close() {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Close(): called." << std::endl;
    rdma_disconnect(rdmaCmId);
    rdma_destroy_id(rdmaCmId);
#endif
}


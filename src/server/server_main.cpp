//
// Created by Mariano Aponte on 30/11/23.
//

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#include <thread>
#include <chrono>

#include <iostream>

#include "../../include/ktmrdma.h"
#include "../../include/utils.h"

#define PORT "9995"

#define BUF_SIZE 1024
#define BACKLOG 8

class main_server_app {
public:
    void run() {
        /// (1) Setup address info
        rdma_addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_port_space = RDMA_PS_TCP;
        hints.ai_flags = RAI_PASSIVE;

        rdma_addrinfo * rdmaAddrinfo;
        ktm_rdma_getaddrinfo(nullptr, PORT, &hints, &rdmaAddrinfo);

        /// (2) Create communication manager id with queue pair attributes
        ibv_qp_init_attr qpInitAttr;
        memset(&qpInitAttr, 0, sizeof(qpInitAttr));

        qpInitAttr.cap.max_send_wr = 1;
        qpInitAttr.cap.max_recv_wr = 1;
        qpInitAttr.cap.max_send_sge = 1;
        qpInitAttr.cap.max_recv_sge = 1;
        qpInitAttr.sq_sig_all = 1;

        rdma_cm_id * rdmaCmListenId;

        ktm_rdma_create_ep(&rdmaCmListenId, rdmaAddrinfo, nullptr, &qpInitAttr);
        rdma_freeaddrinfo(rdmaAddrinfo);

        /// (3) Listen for connections
        ktm_rdma_listen(rdmaCmListenId, BACKLOG);

        /// (4) Get the next pending connection request event
        rdma_cm_id * rdmaCmId;
        ktm_rdma_get_request(rdmaCmListenId, &rdmaCmId);

        /// (5) Prepare memory region for messaging
        void * buffer = malloc(BUF_SIZE);
        ibv_mr * memoryRegion;
        memoryRegion = ktm_rdma_reg_msgs(rdmaCmId, buffer, BUF_SIZE);

        /// (6) Communication
        ibv_wc workCompletion;
        int num_wc;

        /*
        ktm_rdma_post_recv(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion);
        ktm_rdma_accept(rdmaCmId, nullptr);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);
        ktm_rdma_post_recv(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion);
        ktm_rdma_post_send(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion, 0);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);
         */

        ktm_rdma_accept(rdmaCmId, nullptr);
        std::cout << "accepted..." << std::endl;

        ktm_rdma_post_recv(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);

        char * recvmsg = (char *) malloc(strlen((char *)buffer) + 1);
        memcpy(recvmsg, buffer, strlen((char *)buffer));

        std::cout << recvmsg << std::endl;

        char * msg = "hello from server!";
        memcpy(buffer, msg, strlen(msg));

        ktm_rdma_post_send(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion, 0);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

        /// exchanging data for rdma operations
        void * rdmaBuffer = malloc(BUF_SIZE);
        ibv_mr * rdmaMemoryRegion;
        rdmaMemoryRegion = ktm_rdma_reg_write(rdmaCmId, rdmaBuffer, BUF_SIZE);

        uintptr_t peer_address = 0;
        uint32_t peer_rkey = 0;

        ktm_server_exchange_rdma_info(rdmaCmId, rdmaBuffer, BUF_SIZE, rdmaMemoryRegion, &peer_address, &peer_rkey);

        ktm_rdma_post_recv(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);

        rdma_dereg_mr(rdmaMemoryRegion);
        rdmaMemoryRegion = ktm_rdma_reg_read(rdmaCmId, rdmaBuffer, BUF_SIZE);

        ktm_server_exchange_rdma_info(rdmaCmId, rdmaBuffer, BUF_SIZE, rdmaMemoryRegion, &peer_address, &peer_rkey);

        while (1) {
            std::cout << "rdmaBuffer:" << std::endl;
            std::cout << (char *) rdmaBuffer << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

int main() {
    try {
        auto app = new main_server_app();
        app->run();
    }
    catch (std::string & exc) {
        exc_die(exc.c_str());
    }
    catch (const char * & exc) {
        exc_die(exc);
    }

    return 0;
}
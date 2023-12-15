//
// Created by Mariano Aponte on 30/11/23.
//

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#include <iostream>
#include <thread>

#include "../../include/ktmrdma.h"
#include "../../include/utils.h"

#define IP "192.168.4.99"
#define PORT "9995"

#define BUF_SIZE 1024

class main_client_app {
public:
    void run() {
        /// (1) Setup address info
        rdma_addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_port_space = RDMA_PS_TCP;

        rdma_addrinfo * rdmaAddrinfo;
        ktm_rdma_getaddrinfo(IP, PORT, &hints, &rdmaAddrinfo);

        /// (2) Create communication manager id with queue pair attributes
        ibv_qp_init_attr qpInitAttr;
        memset(&qpInitAttr, 0, sizeof(qpInitAttr));

        qpInitAttr.cap.max_send_wr = 1;
        qpInitAttr.cap.max_recv_wr = 1;
        qpInitAttr.cap.max_send_sge = 1;
        qpInitAttr.cap.max_recv_sge = 1;
        qpInitAttr.sq_sig_all = 1;

        rdma_cm_id * rdmaCmId;

        ktm_rdma_create_ep(&rdmaCmId, rdmaAddrinfo, nullptr, &qpInitAttr);
        rdma_freeaddrinfo(rdmaAddrinfo);

        /// (3) Prepare memory regions for messaging
        void * buffer = malloc(BUF_SIZE);
        ibv_mr * memoryRegion;
        memoryRegion = ktm_rdma_reg_msgs(rdmaCmId, buffer, BUF_SIZE);

        /// (4) Communication
        ibv_wc workCompletion;
        int num_wc;

        ktm_rdma_connect(rdmaCmId, nullptr);

        std::cout << "connected..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        char * msg = "hello from client!";
        memcpy(buffer, msg, strlen(msg) + 1);

        ktm_rdma_post_send(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion, 0);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

        ktm_rdma_post_recv(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion);
        num_wc = ktm_rdma_get_recv_comp(rdmaCmId, &workCompletion);

        char * recvmsg = (char *) malloc(strlen((char *)buffer) + 1);
        memcpy(recvmsg, buffer, strlen((char *)buffer));

        std::cout << recvmsg << std::endl;


        /// exchanging data for rdma operations

        void * rdmaBuffer = malloc(BUF_SIZE);
        ibv_mr * rdmaMemoryRegion;
        rdmaMemoryRegion = ktm_rdma_reg_msgs(rdmaCmId, rdmaBuffer, BUF_SIZE);

        uintptr_t peer_address = 0;
        uint32_t peer_rkey = 0;
        ktm_client_exchange_rdma_info(rdmaCmId, rdmaBuffer, BUF_SIZE, rdmaMemoryRegion, &peer_address, &peer_rkey);

        /// PERFORMING WRITE TO SERVER
        char * to_write = "Ah ha! I wrote this!";
        memcpy(buffer, to_write, strlen(to_write) + 1);

        ktm_rdma_post_write(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion, IBV_WR_RDMA_WRITE, (uint64_t) peer_address, peer_rkey);
        num_wc = ktm_rdma_get_send_comp(rdmaCmId, &workCompletion);

        /// PERFORMING READ TO SERVER
        ktm_client_exchange_rdma_info(rdmaCmId, rdmaBuffer, BUF_SIZE, rdmaMemoryRegion, &peer_address, &peer_rkey);

        ktm_rdma_post_read(rdmaCmId, nullptr, buffer, BUF_SIZE, memoryRegion, IBV_WR_RDMA_READ, (uint64_t) peer_address, peer_rkey);

        char * read = (char *) malloc(strlen((char *)buffer) + 1);
        memcpy(read, buffer, strlen((char *)buffer));

        std::cout << "read: " << read << std::endl;
    }
};

int main() {
    try {
        auto app = new main_client_app();
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

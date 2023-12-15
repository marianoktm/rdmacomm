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

/**
 * @brief RdmaCommunicator represents a communication interface using RDMA (Remote Direct Memory Access).
 */
class RdmaCommunicator : public Communicator {
private:
    /// @brief Represents an RDMA communication manager ID for the client.
    rdma_cm_id * rdmaCmId;
    /// @brief Represents an RDMA communication manager ID for the server when in listening mode.
    rdma_cm_id * rdmaCmListenId;

    /// @brief A C-style string representing the hostname for communication.
    char * hostname;
    /// @brief A C-style string representing the port for communication.
    char * port;

    /// @brief A boolean indicating whether the RDMA communicator is in server mode.
    bool isServing = false;

public:
    RdmaCommunicator() = default;

    /**
    * @brief Constructs an RdmaCommunicator object with the specified hostname and port.
    *
    * This constructor resolves the given hostname to an IP address using gethostbyname,
    * checks if the port is specified, and initializes the RdmaCommunicator with the provided
            * information.
    *
    * @param hostname A C-style string representing the hostname.
    * @param port A C-style string representing the port number.
    * @throws An exception if the port is not specified or if the hostname resolution fails.
    */
    RdmaCommunicator(char * hostname, char * port);

    /**
    * @brief Constructs an RdmaCommunicator object with the given rdma_cm_id and serving status.
    *
    * This constructor is used when an RdmaCommunicator is created from an existing rdma_cm_id,
    * and the serving status is specified.
    *
    * @param rdmaCmId A pointer to the rdma_cm_id that will be used to create the communicator.
    * @param isServing A boolean indicating whether the RdmaCommunicator is in server mode.
    */
    RdmaCommunicator(rdma_cm_id * rdmaCmId, bool isServing = true);

    /**
    * @brief Initiates the RDMAcommunicator as a server.
    *
    * This method sets up the RDMA address information, creates a communication manager ID with
            * specified queue pair attributes, and listens for incoming connections.
    *
    * @note The server becomes active and starts listening for connections after calling this method.
    *
    * @throws An exception in case of errors during address resolution, communication manager ID creation,
    *         or listening for connections.
    */
    void Serve();

    /**
    * @brief Accepts an incoming connection and returns a new RdmaCommunicator for communication.
    *
    * This method is used in the server to accept incoming connections. It retrieves the RDMA CM ID
    * for the client, performs the acceptance, and creates a new RdmaCommunicator object for further
            * communication with the client.
    *
    * @note The method uses the ktm_rdma_get_request and ktm_rdma_accept functions.
    * @note The caller is responsible for managing the memory of the returned RdmaCommunicator object.
    *       It is recommended to use smart pointers or manually handle the object's lifecycle.
    *
    * @return A pointer to a new RdmaCommunicator representing the accepted connection.
    * @throws An exception in case of errors during the request retrieval, acceptance, or object creation.
    */
    const Communicator *const Accept() const;

    /**
    * @brief Initiates a connection to the server.
    *
    * This method sets up the RDMA address information, creates a communication manager ID with
            * specified queue pair attributes, and establishes a connection to the specified server.
    *
    * @note The client becomes connected to the server after calling this method.
    *
    * @throws An exception in case of errors during address resolution, communication manager ID creation,
    *         or connection establishment.
    */
    void Connect();

    /**
    * @brief Reads data from the remote peer using RDMA.
    *
    * This method initiates a read operation by registering the buffer for writing state, sending
    * memory information to the peer, signaling readiness, waiting for the peer to finish writing,
    * and performing necessary cleanup.
    *
    * @param buffer A pointer to the buffer where the received data will be written.
    * @param size The size of the data to read.
    * @return The number of bytes successfully read (always 1 in this implementation, to fix).
    */
    size_t Read(char * buffer, size_t size);

    /**
    * @brief Writes data to the remote peer using RDMA.
    *
    * This method initiates a write operation by copying the input buffer, registering it for
    * messaging state, exchanging memory information with the peer, waiting for the signal to write,
            * performing the write operation, signaling the end of writing, and cleaning up resources.
    *
    * @param buffer A pointer to the buffer containing the data to be written.
    * @param size The size of the data to write.
    * @return The number of bytes successfully written (always 1 in this implementation, to fix).
    */
    size_t Write(const char * buffer, size_t size);

    /**
    * @brief This method is a placeholder for future implementation.
    *
    * @note This method is not yet implemented and will print an error message to stderr.
    */
    void Sync();

    /**
    * @brief Closes the RDMA communication and releases associated resources.
    *
    * This method disconnects the RDMA communication and destroys the communication manager ID.
    * It should be called when the communication session is complete or needs to be terminated.
    */
    void Close();

    /**
    * @brief Converts the RdmaCommunicator object to a string representation.
    *
    * @return A string representation of the RdmaCommunicator object.
    */
    std::string to_string() { return "rdmacommunicator"; }
};


#endif //RDMACM_RDMACOMMUNICATOR_H

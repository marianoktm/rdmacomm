//
// Created by Mariano Aponte on 07/12/23.
//

#ifndef RDMACM_COMMUNICATOR_H
#define RDMACM_COMMUNICATOR_H

#include <cstddef>
#include <string>

class Communicator {
public:

    virtual ~Communicator() = default;


    virtual void Serve() = 0;

    virtual const Communicator *const Accept() const = 0;

    virtual void Connect() = 0;

    virtual size_t Read(char *buffer, size_t size) = 0;
    virtual size_t Write(const char *buffer, size_t size) = 0;
    virtual void Sync() = 0;

    virtual void Close() = 0;

    virtual std::string to_string() { return "communicator"; }

    virtual void run(){};

private:
};

#endif //RDMACM_COMMUNICATOR_H

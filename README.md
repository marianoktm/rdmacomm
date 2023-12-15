# rdmacomm: A general purpose tcp-style RDMA client-server

**rdmacomm** is an rdma-enabled tcp-style infiniband client-server, made to meet the requirements of the `Communicator` abstract
class of [GVirtuS: A GPGPU Transparent Virtualization Component for High Performance Computing Clouds](https://github.com/gvirtus/GVirtuS).

It's main purpose is to enable GVirtuS to _RDMA over Infiniband_ communications, but it can be used as
a general purpose RDMA client-server implementation, due to it's ease of use.

The repository also contains `ktmrdma`, a wrapper library which purpose is to enable the original rdma-cm to an 
exception-style error handling.

This project has been made at **"Parthenope University" in Naples**, during my internship for my bachelor
degree in Computer Science.

## How does rdmacomm works

The core of **rdmacomm** is the `RDMACommunicator` class, which is an implementation of the `Communicator` abstract class of **GVirtuS**.

The methods of `RDMACommunicator` are:
* `Serve()`: sets the communicator as a server and puts it in a listen state;
* `Accept()`: as a server, accepts an incoming connection request and returns 
  an RDMACommunicator object to handle that connection;
* `Connect()`: as a client, connects to a server;
* `Close()`: closes the connection;
* `Read()`: mimics the socket read: it actually register the buffer as RDMA-write enabled, sends it's Memory Region infos to the peer and waits for a write;
* `Write()`: mimics the socket write: it actually receives the peer's Memory Region infos and writes the buffer directly into the peer memory;
* `Sync()`: _Not yet implemented._

### How to use the methods

The classic implementation of an **rdmacomm** client-server is the following:

#### Server

This code is taken from the `servercomm_main.cpp` file:

```
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

cli->Close();
```

The server basically performs the following operations:
* Create a new Communicator
* Set the Communicator as a Server using `Serve()` method;
* Accept an incoming communication using `Accept()` method;
* Use `Read()` and `Write()` methods to communicate;
* Close the connection using `Close()` method.

#### Client

This code is taken from the `clientcomm_main.cpp` file:

```
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
```

The client basically performs the following operations:
* Create a new Communicator;
* Connect to the server using `Connect()` method;
* Use `Read()` and `Write()` methods to communicate;

## Compiling and testing rdmacomm

Make sure to have `CMake 3.19` or above.

`ibverbs` and `rdma-cm` are obviously required.

The compilation is trivial:
1) Clone the repo with `git clone`;
2) Create a new `build` directory into the root of the project;
3) Move into this new directory;
4) Perform `cmake ..`;
5) Perform `make`;

To test the client-server, simply run `./servercomm` and `./clientcomm`.

To change the ip and port, simply edit `servercomm_main.cpp` and `clientcomm_main.cpp` files.

## Further developments

**rdmacomm** is a **WORK IN PROGRESS** project, even if already working.

The main developments are:
* Finding the best communication optimizations;
* Adding the option to compile **rdmacomm** as a static and/or shared library;
* Enabling GVirtuS to use **rdmacomm** as a communicator;
* Finding and fixing bugs.

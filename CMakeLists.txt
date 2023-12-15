cmake_minimum_required(VERSION 3.19)
project(rdmacm)

set(CMAKE_CXX_STANDARD 17)

add_executable(clientcomm
        src/clientcomm_main.cpp
        include/ktmrdma.h
        src/ktmrdma.cpp
        src/client/rdma_client.cpp
        include/rdma_client.h
        include/utils.h
        include/Communicator.h
        src/RdmaCommunicator.cpp
        include/RdmaCommunicator.h
        )

add_executable(servercomm
        src/servercomm_main.cpp
        include/ktmrdma.h
        src/ktmrdma.cpp
        src/server/rdma_server.cpp
        include/rdma_server.h
        include/utils.h
        include/Communicator.h
        src/RdmaCommunicator.cpp
        include/RdmaCommunicator.h
        )

target_link_libraries(clientcomm PRIVATE ibverbs)
target_link_libraries(servercomm PRIVATE ibverbs)

target_link_libraries(clientcomm PRIVATE rdmacm)
target_link_libraries(servercomm PRIVATE rdmacm)

target_include_directories(clientcomm PRIVATE /usr/include/infiniband)
target_include_directories(servercomm PRIVATE /usr/include/infiniband)

target_include_directories(clientcomm PRIVATE /usr/include/rdma)
target_include_directories(servercomm PRIVATE /usr/include/rdma)
//
// Created by Mariano Aponte on 01/12/23.
//

#ifndef RDMACM_UTILS_H
#define RDMACM_UTILS_H

#include <sstream>

std::string tab("    ");
std::string doubletab("        ");

void exc_die(const char * exc) {
    std::cerr << exc << std::endl;
    exit(EXIT_FAILURE);
}


#endif //RDMACM_UTILS_H

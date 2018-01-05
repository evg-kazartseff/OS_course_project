//
// Created by evgenii on 15.11.17.
//
#include <iostream>
#include "server.h"

int main() {
    Server::Net* Network = new Server::Net("127.0.0.1", 61530);
    Server::Core* Core = new Server::Core(Network);
    Core->Run();
}
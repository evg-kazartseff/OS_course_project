//
// Created by evgenii on 29.11.17.
//
#include "client.h"

int main(int argc, char** argv) {
    std::cout << "Enter you name: ";
    std::string login;
    std::cin >> login;
    Client::Net* Network = new Client::Net("127.0.0.1", 61530);
    Network->SetUserName(login);
    std::cout << "Wait for connection" << std::endl;
    while(!Network->Connect()) sleep(1);
    Client::Graphic* Graphic = new Client::Graphic();
    Graphic->SetUserName(login);
    Client::Core* Core = new Client::Core(Network, Graphic);
    Core->Run();
    delete Core;
    return 0;
}
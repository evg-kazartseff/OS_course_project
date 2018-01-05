//
// Created by evgenii on 15.11.17.
//

#ifndef OS_COURE_PROJECT_SERVER_H
#define OS_COURE_PROJECT_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <zconf.h>

#include <iostream>
#include <cstdint>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <condition_variable>
#include <set>
#include "../../lib/json.h"

#define MaxConcurrentConnections 4
#define ID_CLIENT_LENGHT 256
#define MAX_MES_LEN 4096


namespace Server {
    using json = nlohmann::json;

    class Messange;
    class Net;
    class User;
    class Game;

    class Messange {
    public:
        std::string Name;
        std::string Messange;
        bool empty() {
            return (this->Name.empty() || this->Messange.empty());
        }
    };

    class Net {
        private:
            bool __runing;
            std::mutex __runing_mutex;
            int __listener;
            struct sockaddr_in __addr;
            unsigned long __max_lenght_mesange;

            std::thread __open_connection;
            std::thread __send_messange;
            std::vector<std::thread> __recv_messange_handler;

            std::unordered_map<std::string, int> __connect_clients;
            std::mutex __connection_clients_mutex;

            std::queue<Messange> __queue_send;
            std::mutex __queue_send_mutex;
            std::condition_variable __queue_send_cond_var;

            std::queue<Messange> __queue_answer;
            std::mutex __queue_answer_mutex;
            std::condition_variable __queue_answer_cond_var;

            std::queue<std::string> __connect_clients_queue;
            std::mutex __connect_clients_queue_mutex;
            std::condition_variable __connect_clients_queue_cond_var;
            void OpenConnection();
            void CloseConnection(std::string Name);
            void SendMessage();
            void ReadMessage(std::string name);

    public:
        Net(const char* IP, uint16_t port);
        ~Net();
        void Send(std::string Name, std::string messange);

        Messange Recv();
        std::string GetNewClientName();
    };

    class Game {
    private:
        std::string name;

        Server::User* Creator;
        Server::User* Connection;
    public:
        Game(Server::User* Creator);
        void SetName(std::string name);
        std::string GetName();
        void Connect(Server::User* User);
        Server::User* GetPartner(Server::User* user);
    };

    class User {
    private:
        std::string UserName;
        Server::Game* Game;
    public:
        User(std::string Name);
        ~User();
        std::string GetName();
        Server::Game* GetGame();
        void SetGame(Server::Game* game);
        void CreateGame();
        std::string GetGameName();
    };

    typedef std::__detail::_Node_iterator<std::pair<const std::string, User>, 0, 1> Client_t;

    typedef std::unordered_map<std::string, Game*> Games_t;

    typedef std::unordered_map<std::string, User> Clients_t;

    class Core {
    private:
        int runing;
        std::mutex runing_mutex;
        Server::Net* Network;

        Games_t Games;

        Clients_t Clients;
        std::mutex Clients_mutex;

        std::thread AddClients;
        void FAddClients();

        void SendListGames(Client_t client);
        void CreateGame(Client_t client, std::string GameName);
        void ConnectToGame(Client_t client, std::string GameName);
        void MakeStep(Client_t client, std::string Step);
        void SendMessange(Client_t client, std::string Messange);
    public:
        Core(Server::Net* Network);
        int Run();
    };
}


#endif //OS_COURE_PROJECT_SERVER_H

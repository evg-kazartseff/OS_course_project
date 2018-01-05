//
// Created by evgenii on 15.11.17.
//

#include "server.h"

Server::Net::Net(const char* IP, uint16_t port) : __addr(),
                                                  __queue_send() {
    this->__runing = true;
    this->__max_lenght_mesange = MAX_MES_LEN;
    this->__listener = socket(AF_INET, SOCK_STREAM, 0);
    if (this->__listener < 0) {
        std::cout << "Server: Не удалось создать сокет входящих подключений" << std::endl;
    }
    int on = 1;
    setsockopt(this->__listener, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on));
    this->__addr.sin_family = AF_INET;
    this->__addr.sin_port = htons(port);
    this->__addr.sin_addr.s_addr = inet_addr(IP);
    if(bind(this->__listener, (struct sockaddr *)&this->__addr, sizeof(this->__addr)) < 0) {
        std::cout << "Server: Could not bind socket to ip address" << std::endl;
    }
    listen(this->__listener, MaxConcurrentConnections);
    this->__open_connection = std::thread(&Net::OpenConnection, this);
    this->__open_connection.detach();
    this->__send_messange = std::thread(&Net::SendMessage, this);
    this->__send_messange.detach();
}

void Server::Net::OpenConnection() {
    int descriptor;
    this->__runing_mutex.lock();
    while (this->__runing) {
        this->__runing_mutex.unlock();
        descriptor = accept(this->__listener, nullptr, nullptr);
        if (descriptor < 0) {
            std::cout << "Server: Connection error!" << std::endl;
        }
        else {
            std::vector<char> NameClient(ID_CLIENT_LENGHT);
            recv(descriptor, NameClient.data(), ID_CLIENT_LENGHT, 0);
            this->__connection_clients_mutex.lock();
            auto Client = this->__connect_clients.find(NameClient.data());
            if (Client == this->__connect_clients.end()) {
                this->__connect_clients.insert({NameClient.data(), descriptor});
                this->__connection_clients_mutex.unlock();
                this->__connect_clients_queue_mutex.lock();
                this->__connect_clients_queue.push(NameClient.data());
                this->__connect_clients_queue_mutex.unlock();
                this->__connect_clients_queue_cond_var.notify_all();
                this->__recv_messange_handler.push_back(std::thread(&Server::Net::ReadMessage, this, NameClient.data()));
                this->__recv_messange_handler.back().detach();
                json messange;
                messange["connect"] = "ok";
                this->Send(NameClient.data(), messange.dump().data());
            }
            else {
                this->__connection_clients_mutex.unlock();
                json messange;
                messange["connect"] = "error";
                std::string error = std::string("Client with name \"");
                error += NameClient.data();
                error += "\" already is login";
                messange["error"] = error.data();
                this->Send(NameClient.data(), messange.dump().data());
                this->CloseConnection(NameClient.data());
            }
        }
        this->__runing_mutex.lock();
    }
    this->__runing_mutex.unlock();
}

void Server::Net::SendMessage() {
    this->__runing_mutex.lock();
    while (this->__runing) {
        this->__runing_mutex.unlock();
        std::unique_lock<std::mutex> lock(this->__queue_send_mutex);
        this->__queue_send_cond_var.wait(lock, [this]{ return !this->__queue_send.empty();});
        while (!this->__queue_send.empty()) {
            Messange messange = this->__queue_send.front();
            this->__queue_send.pop();
            lock.unlock();
            this->__connection_clients_mutex.lock();
            auto client = this->__connect_clients.find(messange.Name);
            if (client != this->__connect_clients.end()) {
                send(client->second, messange.Messange.c_str(),
                     messange.Messange.length(),
                     0);
                std::cout << "Send: [" << messange.Name << "] ["
                          << messange.Messange << "]" <<  std::endl;
            }
            this->__connection_clients_mutex.unlock();
            lock.lock();
        }
        lock.unlock();
        this->__runing_mutex.lock();
    }
    this->__runing_mutex.unlock();
}

void Server::Net::CloseConnection(std::string Name) {
    this->__connection_clients_mutex.lock();
    auto devise = this->__connect_clients.find(Name);
    while (devise != this->__connect_clients.end()) {
        close(devise->second);
        this->__connect_clients.erase(devise);
        devise = this->__connect_clients.find(Name);
    }
    this->__connection_clients_mutex.unlock();
    std::string LogMes = "Server: Close connection: [";
    LogMes += Name;
    LogMes += "]";
    std::cout << LogMes << std::endl;
}

void Server::Net::ReadMessage(std::string name) {
    this->__runing_mutex.lock();
    while (this->__runing) {
        this->__runing_mutex.unlock();
        this->__connection_clients_mutex.lock();
        auto client = this->__connect_clients.find(name);
        if (client != this->__connect_clients.end()) {
            this->__connection_clients_mutex.unlock();
            std::vector<char> Data(this->__max_lenght_mesange);
            ssize_t n = recv(client->second, Data.data(), this->__max_lenght_mesange, 0);
            if (n == 0) {
                this->CloseConnection(name);
            }
            std::cout << "Recv: [" << client->first << "] [" << Data.data() << "]" << std::endl;
            this->__queue_answer_mutex.lock();
            this->__queue_answer.push({client->first, Data.data()});
            this->__queue_answer_mutex.unlock();
            this->__queue_answer_cond_var.notify_all();
        }
        else {
            this->__connection_clients_mutex.unlock();
            return;
        }
        this->__runing_mutex.lock();
    }
    this->__runing_mutex.unlock();
}

void Server::Net::Send(std::string Name, std::string messange) {
    this->__queue_send_mutex.lock();
    this->__queue_send.push({Name, messange});
    this->__queue_send_mutex.unlock();
    this->__queue_send_cond_var.notify_all();
}

Server::Messange Server::Net::Recv() {
    Messange messange = {};
    this->__runing_mutex.lock();
    if (this->__runing) {
        this->__runing_mutex.unlock();
        std::unique_lock<std::mutex> lock(this->__queue_answer_mutex);
        this->__queue_answer_cond_var.wait(lock, [this]{ return !this->__queue_answer.empty();});
        if (!this->__queue_answer.empty()) {
            messange = this->__queue_answer.front();
            this->__queue_answer.pop();
        }
        lock.unlock();
    }
    this->__runing_mutex.unlock();
    return messange;
}

std::string Server::Net::GetNewClientName() {
    std::string Name;
    this->__runing_mutex.lock();
    if (this->__runing) {
        this->__runing_mutex.unlock();
        std::unique_lock<std::mutex> lock(this->__connect_clients_queue_mutex);
        this->__connect_clients_queue_cond_var.wait(lock,
                                                    [this] { return !this->__connect_clients_queue.empty(); });
        if (!this->__connect_clients_queue.empty()) {
            Name = this->__connect_clients_queue.front();
            this->__connect_clients_queue.pop();
        }
        lock.unlock();
    }
    this->__runing_mutex.unlock();
    return Name;
}

Server::Net::~Net() {
    for (auto i : this->__connect_clients) {
        this->CloseConnection(i.first);
    }
    close(this->__listener);
}


Server::User::User(std::string Name) {
    this->UserName = Name;
}

void Server::User::CreateGame() {
    this->Game = new Server::Game(this);
}

Server::Game* Server::User::GetGame() {
    return this->Game;
}

Server::User::~User() {
    delete this->Game;
}

std::string Server::User::GetName() {
     return this->UserName;
}

std::string Server::User::GetGameName() {
    return this->GetGame()->GetName();
}

void Server::User::SetGame(Server::Game* game) {
    this->Game = game;
}

Server::Game::Game(Server::User* Creator) {
    this->Creator = Creator;
}

void Server::Game::Connect(Server::User* User) {
    this->Connection = User;
}

void Server::Game::SetName(std::string name) {
    this->name = name;
}

std::string Server::Game::GetName() {
    return this->name;
}

Server::User* Server::Game::GetPartner(Server::User* user) {
    Server::User* ptr;
    (this->Creator == user) ? ptr = this->Connection : ptr = this->Creator;
    return ptr;
}

int Server::Core::Run() {
    this->runing_mutex.lock();
    this->runing = true;
    this->runing_mutex.unlock();
    this->AddClients = std::thread(&Server::Core::FAddClients, this);
    this->AddClients.detach();

    this->runing_mutex.lock();
    while (this->runing) {
        this->runing_mutex.unlock();
        Server::Messange messange = this->Network->Recv();
        std::unordered_map<std::string, Server::User>::iterator client;
        this->Clients_mutex.lock();
        if ((client = this->Clients.find(messange.Name)) != this->Clients.end()) {
            this->Clients_mutex.unlock();
            if (!messange.Messange.empty()) {
                try {
                    auto parse_messange = json::parse(messange.Messange);
                    if (parse_messange.find("action") != parse_messange.end()) {
                        if (parse_messange["action"] == "get list game") {
                            this->SendListGames(client);
                        } else if (parse_messange["action"] == "create game") {
                            this->CreateGame(client, parse_messange["name"]);
                        } else if (parse_messange["action"] == "connect to game") {
                            this->ConnectToGame(client, parse_messange["name"]);
                        } else if (parse_messange["action"] == "delete game") {
                            std::string name = parse_messange["name"];
                            this->Games.erase(name);
                        } else if (parse_messange["action"] == "test partner") {
                            json answ;
                            if (client->second.GetGame()->GetPartner(&client->second)) {
                                answ["test partner"] = "ok";
                            } else {
                                answ["test partner"] = "err";
                            }
                            this->Network->Send(client->first, answ.dump());
                        }
                    }
                    else if (parse_messange.find("Step") != parse_messange.end()) {
                        this->MakeStep(client, parse_messange.dump());
                    }
                    else if (parse_messange.find("messange") != parse_messange.end()) {
                        this->SendMessange(client, parse_messange.dump());
                    }
                } catch (std::exception& exception) {
                    std::cout << exception.what() << std::endl;
                }
            }
        }
        this->Clients_mutex.unlock();
        this->runing_mutex.lock();
    }
    this->runing_mutex.unlock();
}

void Server::Core::FAddClients() {
    this->runing_mutex.lock();
    while (this->runing) {
        this->runing_mutex.unlock();
        std::string NewClientName = this->Network->GetNewClientName();
        Server::User NewClient(NewClientName);
        this->Clients_mutex.lock();
        this->Clients.insert({NewClientName, NewClient});
        this->Clients_mutex.unlock();
    }
}

Server::Core::Core(Server::Net* Network) {
    this->Network = Network;
}

void Server::Core::SendListGames(std::unordered_map<std::string, Server::User>::iterator client) {
    json Answer;
    json::array_t array;
    for (auto i : this->Games) {
        array.push_back(i.second->GetName().data());
    }
    Answer["list game"] = array;
    this->Network->Send(client->first, Answer.dump());
}

void Server::Core::CreateGame(std::unordered_map<std::string, Server::User>::iterator client, std::string GameName) {
    client->second.CreateGame();
    client->second.GetGame()->SetName(GameName);
    this->Games.insert({client->second.GetGameName(), client->second.GetGame()});
}

void Server::Core::ConnectToGame(std::unordered_map<std::string, Server::User>::iterator client, std::string GameName) {
    auto game = this->Games.find(GameName);
    if (game != this->Games.end()) {
        game->second->Connect(&client->second);
        client->second.SetGame(game->second);
    }
}

void Server::Core::MakeStep(std::unordered_map<std::string, Server::User>::iterator client, std::string Step) {
    this->Network->Send(client->second.GetGame()->GetPartner(&client->second)->GetName(), Step);
}

void Server::Core::SendMessange(std::unordered_map<std::string, Server::User>::iterator client, std::string Messange) {
    this->Network->Send(client->second.GetGame()->GetPartner(&client->second)->GetName(), Messange);
}

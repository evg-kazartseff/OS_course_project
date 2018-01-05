//
// Created by evgenii on 29.11.17.
//


#include "client.h"

Client::Net::Net(const char* IP, uint16_t port) : __soc_descriptor(), __addr() {
    this->__runing = true;
    this->__addr.sin_family = AF_INET;
    this->__addr.sin_addr.s_addr = inet_addr(IP);
    this->__addr.sin_port = htons(port);
    this->__max_mes_len = MAX_MES_LEN;
    this->create_socket();
}

void Client::Net::create_socket() {
    this->__soc_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(this->__soc_descriptor < 0)
    {
        std::cerr << "Client: Socket initialization error" << std::endl;
    } else {
        int on = 1;
        setsockopt(this->__soc_descriptor, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on));
    }
}

int Client::Net::Connect() {
    int sd = connect(this->__soc_descriptor, (struct sockaddr *) &this->__addr, sizeof(this->__addr));
    if (sd == -1) {
        std::cerr << "Client: Can't connect to the server" << std::endl;
        return 0;
    }
    ssize_t st;
    do {st = send(this->get_sd(), this->GetUserName().data(), this->GetUserName().length(), 0);} while (st == EAGAIN);
    std::vector<char> Answer(ID_CLIENT_LENGHT);
    recv(this->get_sd(), Answer.data(), ID_CLIENT_LENGHT, 0);
    auto Json = json::parse(Answer.data());
    json::iterator status;
    if ((status = Json.find("connect")) != Json.end()) {
        if (*status == "ok") {
            this->__connected_mutex.lock();
            this->__connected_mutex.unlock();
            this->__recv_messange_handler = std::thread(&Client::Net::RecvMessange, this);
            this->__recv_messange_handler.detach();
            std::cout << "Client: Connect to the server" << std::endl;
            return 1;
        } else if (*status == "error") {
            if ((status = Json.find("error")) != Json.end()) {
                std::cerr << status.operator*() << std::endl;
                return 0;
            }
        }
    }
    return 0;
}

void Client::Net::SetUserName(std::string UserName) {
    this->__username = UserName;
}

std::string Client::Net::GetUserName() {
    return this->__username;
}

void Client::Net::Send(std::string messange) {
    send(this->get_sd(), messange.data(), messange.length(), 0);
}

int Client::Net::get_sd() {
    return this->__soc_descriptor;
}

void Client::Net::RecvMessange() {
    this->__runing_mutex.lock();
    while (this->__runing) {
        this->__runing_mutex.unlock();
        std::vector<char> Data(this->__max_mes_len);
        ssize_t n = recv(this->get_sd(), Data.data(), this->__max_mes_len, 0);
        if (n == 0) {
            this->CloseConnection();
        }
        else {
            this->__queue_answer_mutex.lock();
            this->__queue_answer.push(Data.data());
            this->__queue_answer_mutex.unlock();
            this->__queue_answer_cond_var.notify_all();
        }
        Data.clear();
        this->__runing_mutex.lock();
    }
    this->__runing_mutex.unlock();
}

void Client::Net::CloseConnection() {
    close(this->get_sd());
    this->__connected_mutex.lock();
    this->__connected_mutex.unlock();
}

std::string Client::Net::Recv() {
    std::string messange;
    this->__runing_mutex.lock();
    if (this->__runing) {
        this->__runing_mutex.unlock();
        std::unique_lock<std::mutex> lock(this->__queue_answer_mutex);
        this->__queue_answer_cond_var.wait(lock, [this] { return !this->__queue_answer.empty(); });
        if (!this->__queue_answer.empty()) {
            messange = this->__queue_answer.front();
            this->__queue_answer.pop();
        }
        lock.unlock();
    }
    this->__runing_mutex.unlock();
    return messange;
}


Client::Game::Game(int dimension) {
    this->Grid = new int*[dimension];
    for (int i = 0; i < dimension; ++i) {
        this->Grid[i] = new int[dimension];
    }
    this->CleanGrid(dimension);
}

void Client::Game::CleanGrid(int dimension) {
    for (int i = 0; i < dimension; ++i) {
        for (int j = 0; j < dimension; ++j) {
            this->Grid[i][j] = 0;
        }
    }
}

bool Client::Game::MakeStep(int row, int col, int val) {
    if (this->Grid[row][col] == 0) {
        this->Grid[row][col] = val;
        return true;
    }
    return false;
}

int**Client::Game::GetGrid() {
    return this->Grid;
}

int Client::Game::CheckWin() {
    int XHOne = 0, XHTwo = 0, XHThree = 0;
    int XVOne = 0, XVTwo = 0, XVThree = 0;
    int XDOne = 0, XDTwo = 0;

    int ZHOne = 0, ZHTwo = 0, ZHThree = 0;
    int ZVOne = 0, ZVTwo = 0, ZVThree = 0;
    int ZDOne = 0, ZDTwo = 0;
    int all_no_zero = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (this->Grid[i][j] == 1) {
                if (i == 0) {
                    XHOne++;
                }
                else if (i == 1) {
                    XHTwo++;
                }
                else if (i == 2) {
                    XHThree++;
                }

                if (i == j) {
                    XDOne++;
                }
                if ((i==0)&&(j==2)||(i==1)&&(j==1)||(i==2)&&(j==0)) {
                    XDTwo++;
                }

                if (j == 0) {
                    XVOne++;
                } else if (j == 1) {
                    XVTwo++;
                } else if (j == 2) {
                    XVThree++;
                }
                all_no_zero++;
            }
            else if (this->Grid[i][j] == -1) {
                if (i == 0) {
                    ZHOne++;
                }
                else if (i == 1) {
                    ZHTwo++;
                }
                else if (i == 2) {
                    ZHThree++;
                }

                if (i == j) {
                    ZDOne++;
                }
                if ((i==0)&&(j==2)||(i==1)&&(j==1)||(i==2)&&(j==0)) {
                    ZDTwo++;
                }

                if (j == 0) {
                    ZVOne++;
                } else if (j == 1) {
                    ZVTwo++;
                } else if (j == 2) {
                    ZVThree++;
                }
                all_no_zero++;
            }
        }
    }
    if (XHOne == 3 || XHTwo == 3 || XHThree == 3 || XVOne == 3 || XVTwo == 3 || XVThree == 3 || XDOne == 3 || XDTwo == 3)
        return 1;
    if (ZHOne == 3 || ZHTwo == 3 || ZHThree == 3 || ZVOne == 3 || ZVTwo == 3 || ZVThree == 3 || ZDOne == 3 || ZDTwo == 3)
        return -1;
    if ((XHOne == 3 || XHTwo == 3 || XHThree == 3 || XVOne == 3 || XVTwo == 3 || XVThree == 3 || XDOne == 3 || XDTwo == 3) && (ZHOne == 3 || ZHTwo == 3 || ZHThree == 3 || ZVOne == 3 || ZVTwo == 3 || ZVThree == 3 || ZDOne == 3 || ZDTwo == 3))
        return 10;
    if (all_no_zero == 9)
        return 10;
    return 0;
}

void Client::Game::SetGrid(int i, int j, int val) {
    this->Grid[i][j] = val;
}

Client::Graphic::Graphic() {
    this->Main = initscr();
    this->grid = new int*[3];
    for (int i = 0; i < 3; ++i) {
        this->grid[i] = new int[3];
        for (int j = 0; j < 3; ++j) {
            this->grid[i][j] = 0;
        }
    }
}

Client::Graphic::~Graphic() {
    for (int i = 0; i < 3; ++i) {
        delete this->grid[i];
    }
    delete this->grid;
    clear();
    endwin();
}

void Client::Graphic::UpdateMainWindow() {
    clear();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_CYAN);
    wbkgd(this->Main, COLOR_PAIR(1));
    wrefresh(this->Main);
}

void Client::Graphic::StartGraphic(std::string name_game) {
    UpdateMainWindow();
    CreateGameWindow(name_game);
    UpdateGrid();
    UpdateRightWindow();
    UpdateTextEditWindow();
}

void Client::Graphic::UpdateCell(int row, int col, int val, int beg_x, int beg_y, bool active) {
    if(this->GridWindow) {
        if (this->Call[row][col])
            delwin(this->Call[row][col]);
        if (this->WriteCall[row][col])
            delwin(this->WriteCall[row][col]);
        this->Call[row][col] = derwin(this->GridWindow, 7, 7, beg_y + 1, beg_x + 1);
        if (active) {
            init_pair (10, COLOR_RED, COLOR_CYAN);
            wattron(this->Call[row][col],COLOR_PAIR(10));
            box(this->Call[row][col], ACS_VLINE, ACS_HLINE);
            wattroff(this->Call[row][col],COLOR_PAIR(10));
        } else {
            box(this->Call[row][col], ACS_VLINE, ACS_HLINE);
        }
        this->WriteCall[row][col] = derwin(this->Call[row][col], 5, 5, 1, 1);
        box(this->WriteCall[row][col], ACS_VLINE, ACS_HLINE);
        if (val == 1) {
           mvwprintw(this->WriteCall[row][col], 0, 0, X);
        }
        else if (val == -1) {
            mvwprintw(this->WriteCall[row][col], 0, 0, ZERO);
        }
        else if (val == 0) {
             mvwprintw(this->WriteCall[row][col], 0, 0, CLEAN);
        }
        wrefresh(this->Call[row][col]);
        wrefresh(this->WriteCall[row][col]);
    }
}

void Client::Graphic::UpdateGrid() {
    if (this->GridWindow)
        delwin(this->GridWindow);
    this->GridWindow = subwin(this->Game, 23, 23, 2, 6);
    box(this->GridWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(this->GridWindow);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            this->UpdateCell(i, j, this->grid[i][j], i * 7, j * 7, ((i == this->CurActive.i) && (j == this->CurActive.j)));
        }
    }
}

void Client::Graphic::CreateGameWindow(std::string NameGame) {
    this->Game = subwin(this->Main, 30, 36, 0, 0);
    box(this->Game, ACS_VLINE, ACS_HLINE);
    std::string text = "Game: ";
    text += NameGame;
    mvwprintw(this->Game, 1, 2, text.data());
    mvwprintw(this->Game, 25, 1, "TAB - change window");
    mvwprintw(this->Game, 26, 1, "F10 - exit");
    wrefresh(this->Game);
}

std::string Client::Graphic::ChoiceCreateGame(std::set<std::string> Games) {
    UpdateMainWindow();
    this->CreateChoiceGameWindow = derwin(this->Main, this->Main->_maxy, this->Main->_maxx, 0, 0);
    box(this->CreateChoiceGameWindow, ACS_VLINE, ACS_HLINE);
    mvwprintw(this->Main, 1, this->Main->_maxx / 2 - 6, "Choice game");
    mvwprintw(this->Main, this->Main->_maxy - 4, 1, "Press f1 for create game or enter the game number: ");
    wrefresh(this->CreateChoiceGameWindow);
    WINDOW* GamesWindow = derwin(this->CreateChoiceGameWindow, this->CreateChoiceGameWindow->_maxy - 5, this->CreateChoiceGameWindow->_maxx - 1, 2, 1);
    box(GamesWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(GamesWindow);
    int iter = 0;
    for (std::set<std::string>::iterator iterator = Games.begin(); iterator != Games.end(); iterator++) {
        std::string name;
        char iter_str[10];
        std::sprintf(iter_str,"%d", iter);
        name += iter_str;
        name += " ";
        name += *iterator;
        mvwprintw(this->Main, iter + 3, 2, name.data());
        iter++;
    }
    refresh();
    keypad(this->Main, true);
    int c;
    char name_buff[80];
    int i = 0;
    move(this->Main->_maxy - 3, 1);
    while((c = getch()) != 10) {
        name_buff[i++] = static_cast<char>(c);
    }
    name_buff[i++] = '\000';
    if (name_buff[0] >= '0' && name_buff[0] <= '9') {
        unsigned long idx = static_cast<unsigned long>(strtol(name_buff, nullptr, 10));
        std::set<std::string>::iterator it = Games.begin();
        std::advance(it, idx);
        return *it;
    }
    std::string name = name_buff;
    return name;
}

void Client::Graphic::SetUserName(std::string username) {
    this->username = username;
}

void Client::Graphic::UpdateRightWindow() {
    this->RightWindow = subwin(this->Main, 30, 60, 0, 36);
    box(this->RightWindow, ACS_VLINE, ACS_HLINE);
    mvwprintw(this->RightWindow, 1, 28, "CHAT");
    wrefresh(this->RightWindow);
    UpdateChatWindow();
}

void Client::Graphic::UpdateChatWindow() {
    this->ChatWindow = derwin(this->RightWindow, 27, 58, 2, 1);
    box(this->ChatWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(this->ChatWindow);
}

WINDOW* Client::Graphic::GetMainWindow() {
    return this->Main;
}

Client::cur_active Client::Graphic::GetActiveCall() {
    return this->CurActive;
}

void Client::Graphic::SetActiveCall(cur_active Cur) {
    this->CurActive = Cur;
}

void Client::Graphic::UpdateTextEditWindow() {
    this->TextEditWindow = derwin(this->ChatWindow, 5, 56, 21, 1);
    wclear(this->TextEditWindow);
    box(this->TextEditWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(this->TextEditWindow);
}

void Client::Graphic::SetGrid(int** griid) {
    this->grid = griid;
}

void Client::Graphic::UpdateWinLoseWindow(int val) {
    if (val != 0) {
        this->WinLoseWindow = newwin(10, 30, 10, 30);
        box(this->WinLoseWindow, ACS_VLINE, ACS_HLINE);
        if (val == 1)
            mvwprintw(this->WinLoseWindow, this->WinLoseWindow->_maxy / 2, this->WinLoseWindow->_maxx / 2 - 5,
                      "You Win!!!");
        else if (val == -1)
            mvwprintw(this->WinLoseWindow, this->WinLoseWindow->_maxy / 2, this->WinLoseWindow->_maxx / 2 - 5,
                      "You Lose!!!");
        else if (val == 10)
            mvwprintw(this->WinLoseWindow, this->WinLoseWindow->_maxy / 2, this->WinLoseWindow->_maxx / 2 - 5, "Draw!!!");
        wrefresh(this->WinLoseWindow);
    }
}

void Client::Graphic::PrintMessange(std::string Messange) {
    mvwprintw(this->ChatWindow, this->count_of_mes + 1, 1, Messange.data());
    this->count_of_mes++;
    wrefresh(this->ChatWindow);
}

std::string Client::Graphic::GetMesange() {
    move(24, 39);
    char name_buff[80];
    int i = 0;
    int c;
    echo();
    while((c = getch()) != 10) {
        name_buff[i++] = static_cast<char>(c);
    }
    noecho();
    name_buff[i++] = '\000';
    std::string ret = name_buff;
    return ret;
}

std::string Client::Graphic::GetUserName() {
    return this->username;
}

Client::Core::Core(Client::Net* Network, Client::Graphic* Graphic) {
    this->Network = Network;
    this->Graphic = Graphic;
    this->Game = new Client::Game(3);
}

Client::Core::~Core() {
    delete this->Network;
    delete this->Graphic;
    delete this->Game;
}

int Client::Core::Run() {
    std::set<std::string> list_game = this->RequestGameList();
    std::string name_game = this->Graphic->ChoiceCreateGame(list_game);
    json Json;
    if (list_game.find(name_game) == list_game.end()) {
        Json["action"] = "create game";
        Creator = true;
        my_step = true;
    } else {
        Json["action"] = "connect to game";
        Creator = false;
        my_step = false;
    }
    Json["name"] = name_game.data();
    this->Network->Send(Json.dump());
    clear();
    this->Graphic->StartGraphic(name_game);
    this->RecvThread = std::thread(&Client::Core::RecvHendler, this);
    this->RecvThread.detach();
    int flg;
    keypad(this->Graphic->GetMainWindow(), true);
    noecho();
    do {
        if (active_win == 1) {
            json test;
            test["action"] = "test partner";
            this->Network->Send(test.dump());
            this->test_partner_mutex.lock();
            this->test_partner_mutex.lock();
            this->test_partner_mutex.unlock();
            std::string text = this->Graphic->GetMesange();
            json mes;
            mes["messange"] = text.data();
            mes["name"] = this->Graphic->GetUserName();
            std::string print = this->Graphic->GetUserName();
            print += ": ";
            print += text;
            this->Graphic->PrintMessange(print);
            this->Network->Send(mes.dump());
            this->Graphic->UpdateTextEditWindow();
            active_win ^= 1;
        }
        this->Graphic->SetGrid(this->Game->GetGrid());
        this->Graphic->UpdateGrid();
        flg = KeyHendler();
        if (flg == EXIT) {
            if (this->Creator) {
                json del_game;
                del_game["action"] = "delete game";
                del_game["name"] = name_game;
                this->Network->Send(del_game.dump());
            }
            this->runing = false;
            return 0;
        }
        if (flg == SELECT_ACTIVE) {
            this->active_win ^= 1;
        }
    } while (this->runing);
    return 0;
}

int Client::Core::KeyHendler() {
    int action = 0;
    switch (getch()) {
        case KEY_F(10):
            action = EXIT;
            break;
        case 9:
            action = SELECT_ACTIVE;
            break;
        case 10:
            if (active_win == 0) {
                cur_active cur = this->Graphic->GetActiveCall();
                if (my_step) {
                    json test;
                    test["action"] = "test partner";
                    this->Network->Send(test.dump());
                    this->test_partner_mutex.lock();
                    this->test_partner_mutex.lock();
                    this->test_partner_mutex.unlock();
                    if (this->Game->MakeStep(cur.i, cur.j, this->Creator ? 1 : -1)) {
                        this->Graphic->SetGrid(this->Game->GetGrid());
                        this->Graphic->UpdateGrid();

                        json::array_t array1, array2, array3;
                        for (int j = 0; j < 3; ++j) {
                            array1.push_back(this->Game->GetGrid()[0][j]);
                            array2.push_back(this->Game->GetGrid()[1][j]);
                            array3.push_back(this->Game->GetGrid()[2][j]);
                        }
                        json::array_t matrix;
                        matrix.push_back(array1);
                        matrix.push_back(array2);
                        matrix.push_back(array3);
                        json Step;
                        Step["Step"] = matrix;
                        this->Network->Send(Step.dump());
                    }
                    int status = this->Game->CheckWin();
                    if ((status == 1) && this->Creator) {
                        this->win = 1;
                    } else if ((status == -1) && (this->Creator)) {
                        this->win = -1;
                    } else if (status == 1 && !this->Creator) {
                        this->win = -1;
                    } else if (status == -1 && !this->Creator) {
                        this->win = 1;
                    } else if (status == 0) {
                        this->win = 0;
                    } else if (status == 10) {
                        this->win = 10;
                    }
                    this->Graphic->UpdateWinLoseWindow(this->win);
                    my_step ^= 1;
                }
            }
            action = RUN;
            break;
        case KEY_UP:
            if (active_win == 0) {
                if (this->Graphic->GetActiveCall().j > 0) {
                    cur_active cur = this->Graphic->GetActiveCall();
                    cur.j--;
                    this->Graphic->SetActiveCall(cur);
                }
                action = RUN;
                this->Graphic->UpdateGrid();
            }
            else if (active_win == 1) {

            }
            break;
        case KEY_DOWN:
            if (active_win == 0) {
                if (this->Graphic->GetActiveCall().j < 2) {
                    cur_active cur = this->Graphic->GetActiveCall();
                    cur.j++;
                    this->Graphic->SetActiveCall(cur);
                }
                action = RUN;
                this->Graphic->UpdateGrid();
            }
            else if (active_win == 1) {

            }
            break;
        case KEY_LEFT:
            if (active_win == 0) {
                if (this->Graphic->GetActiveCall().i > 0) {
                    cur_active cur = this->Graphic->GetActiveCall();
                    cur.i--;
                    this->Graphic->SetActiveCall(cur);
                }
                action = RUN;
                this->Graphic->UpdateGrid();
            }
            else if (active_win == 1) {

            }
            break;
        case KEY_RIGHT:
            if (active_win == 0) {
                if (this->Graphic->GetActiveCall().i < 2) {
                    cur_active cur = this->Graphic->GetActiveCall();
                    cur.i++;
                    this->Graphic->SetActiveCall(cur);
                }
                action = RUN;
                this->Graphic->UpdateGrid();
            }
            else if (active_win == 1) {

            }
            break;
        default:
            action = RUN;
            break;
    }
    return action;
}

std::set<std::string> Client::Core::RequestGameList() {
    json Json;
    Json["action"] = "get list game";
    this->Network->Send(Json.dump());
    Json.clear();
    Json = json::parse(this->Network->Recv());
    std::set<std::string> list_game;
    if (Json.find("list game") != Json.end()) {
        std::vector<std::string> t = Json["list game"];
        for (unsigned long i = 0; i < t.size(); i++) {
            list_game.insert(t.at(i));
        }
        Json.clear();
    }
    return list_game;
}

void Client::Core::RecvHendler() {
    while (runing) {
        json mes = json::parse(this->Network->Recv());
        if (mes.find("Step") != mes.end()) {
            std::vector<std::vector<int>> t = mes["Step"];
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    this->Game->SetGrid(i, j, t.at(i).at(j));
                }
            }
            this->Graphic->SetGrid(this->Game->GetGrid());
            this->Graphic->UpdateGrid();
            int status = this->Game->CheckWin();
            if ((status == 1) && this->Creator) {
                this->win = 1;
            } else if ((status == -1) && (this->Creator)) {
                this->win = -1;
            } else if (status == 1 && !this->Creator) {
                this->win = -1;
            } else if (status == -1 && !this->Creator) {
                this->win = 1;
            } else if (status == 0) {
                this->win = 0;
            } else if (status == 10) {
                this->win = 10;
            }
            this->Graphic->UpdateWinLoseWindow(this->win);
            this->my_step ^= 1;
        } else if (mes.find("messange") != mes.end()) {
            std::string mesange = mes["name"];
            mesange += ": ";
            mesange += mes["messange"];
            this->Graphic->PrintMessange(mesange);
        } else if (mes.find("test partner") != mes.end()) {
            if (mes["test partner"] == "ok")
                this->test_partner_mutex.unlock();
        }
    }
}

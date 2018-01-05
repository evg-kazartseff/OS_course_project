//
// Created by evgenii on 29.11.17.
//

#ifndef OS_COURE_PROJECT_CLIENT_H
#define OS_COURE_PROJECT_CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include "../../lib/json.h"
#include <ncurses.h>
#include <set>
#include <unordered_set>


#define ID_CLIENT_LENGHT 256
#define MAX_MES_LEN 4096

using json = nlohmann::json;

#define EXIT 0
#define RUN 1
#define SELECT_ACTIVE 2

namespace Client {
    class Net {
    private:
        int __soc_descriptor;
        struct sockaddr_in __addr;
        size_t __max_mes_len;

        std::mutex __connected_mutex;

        bool __runing;
        std::mutex __runing_mutex;

        std::string __username;

        std::queue<std::string> __queue_answer;
        std::mutex __queue_answer_mutex;
        std::condition_variable __queue_answer_cond_var;

        std::thread __recv_messange_handler;
        void RecvMessange();

        void create_socket();
        int get_sd();
        void CloseConnection();
    protected:
    public:
        Net(const char* IP, uint16_t port);
        int Connect();

        void SetUserName(std::string UserName);
        std::string GetUserName();


        void Send(std::string messange);
        std::string Recv();
    };

    typedef struct  {
        int i = 0;
        int j = 0;
    } cur_active;

    class Graphic {
    private:
        cur_active CurActive;
        const char* X = "x   x x x   x   x x x   x";
        const char* ZERO = "000000   00   00   000000";
        const char* CLEAN = "                         ";
        std::string username;
        int** grid;
        WINDOW* Main;
        WINDOW* Game;
        WINDOW* GridWindow;
        WINDOW* Call[3][3];
        WINDOW* WriteCall[3][3];
        WINDOW* CreateChoiceGameWindow;
        WINDOW* RightWindow;
        WINDOW* ChatWindow;
        WINDOW* TextEditWindow;
        WINDOW* WinLoseWindow;
        int count_of_mes = 0;
        void UpdateMainWindow();
        void CreateGameWindow(std::string NameGame);
        void UpdateCell(int row, int col, int val, int beg_x, int beg_y, bool active);
        void UpdateRightWindow();
        void UpdateChatWindow();
    public:
        void UpdateGrid();
        void SetGrid(int** griid);
        Graphic();
        ~Graphic();
        void UpdateTextEditWindow();
        void PrintMessange(std::string Messange);
        std::string GetMesange();
        void UpdateWinLoseWindow(int val);
        void StartGraphic(std::string name_game);
        std::string ChoiceCreateGame(std::set<std::string> Games);
        void SetUserName(std::string username);
        std::string GetUserName();
        WINDOW* GetMainWindow();
        cur_active GetActiveCall();
        void SetActiveCall(cur_active Cur);
    };

    class Game {
    private:
        int **Grid;
        void CleanGrid(int dimension);
    public:
        Game(int dimension);
        bool MakeStep(int row, int col, int val);
        int CheckWin();
        int** GetGrid();
        void SetGrid(int i, int j, int val);
    };

    class Core {
    private:
        Client::Net* Network;
        Client::Graphic* Graphic;
        Client::Game* Game;
        bool Creator;
        bool my_step;
        int active_win = 0;
        int win;
        bool runing = true;
        std::set<std::string> RequestGameList();
        int KeyHendler();
        std::thread RecvThread;
        void RecvHendler();
        std::mutex test_partner_mutex;
    public:
        Core(Client::Net* Network, Client::Graphic* Graphic);
        ~Core();
        int Run();
    };
}

#endif //OS_COURE_PROJECT_CLIENT_H

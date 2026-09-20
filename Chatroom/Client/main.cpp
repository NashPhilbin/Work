#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include "curses.h"
#include <iterator>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define closesocket close
#endif

std::vector<std::string> processList(std::string list, char delim);
void recieveFromServer(int clientSocket);
std::mutex messageMutex;
std::mutex msgupdMutex;
std::mutex roomMutex;
std::vector<std::string> roomList = {"Lobby"};
std::vector<std::string> messageList = {};
bool roomUpdated = true;
bool msgUpdated = true;

int main(int argc, char *argv[])
{
    std::string username = "-1";
    std::string serverIP = "127.0.0.1";
    int serverPort = 54000;
    if (argc >= 2)
        serverIP = argv[1];
    if (argc >= 3)
        serverPort = std::stoi(argv[2]);
    if (argc >= 4)
        username = argv[3];

#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        std::cerr << "Failed to create socket\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address\n";
        closesocket(clientSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (connect(clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Connection failed\n";
        closesocket(clientSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    send(clientSocket, username.c_str(), username.size(), 0);

    int width = -1;
    int length = -1;
    int scroll = 0;
    bool scrollUpdated = true;
    int currentRoom = 0;
    int ch;
    std::string chBuffer;

    initscr(); // Start curses mode
    timeout(150);
    start_color();
    keypad(stdscr, TRUE);
    noecho();
    getmaxyx(stdscr, length, width);
    int messageSpaces = (length * .85) - 4;
    // height, width, top left y, x
    WINDOW *mainScr = newwin(length, width, 0, 0);                                    // encompasses whole screen
    WINDOW *messages = newwin((length * .85) - 2, (width * .75) - 1, 2, 1);           // top left majority of screen
    WINDOW *navBar = newwin((length * .85) - 2, (width * .25) - 1, 2, (width * .75)); // top right covers side of screen
    WINDOW *input = newwin((length * .15), width - 2, (length * .85), 1);             // bottom left covers bottom of screen
    std::vector<WINDOW *> windows = {mainScr, messages, navBar, input};               // pointers to all windows for updating via loop
    mvwprintw(mainScr, 1, 1, "Advanced C++ Chat Client");
    box(mainScr, 0, 0);
    refresh();
    wrefresh(mainScr);

    std::thread recieveThread(recieveFromServer, clientSocket);
    recieveThread.detach();

    while (true)
    {
        werase(input);
        mvwprintw(input, 1, 1, "Enter message here:");
        mvwprintw(input, 2, 1, ("> " + chBuffer).c_str());
        wnoutrefresh(input);

        bool localMsgUpdated;
        {
            std::lock_guard<std::mutex> msggupdLock(msgupdMutex);
            localMsgUpdated = msgUpdated;
            msgUpdated = false;
        }

        if (localMsgUpdated || scrollUpdated)
        {
            werase(messages);
            box(messages, 0, 0);
            std::lock_guard<std::mutex> messageLock(messageMutex);
            for (int i = scroll; i < messageSpaces + scroll && i < messageList.size(); i++)
            {
                mvwprintw(messages, i - scroll + 1, 1, messageList[i].c_str());
            }
            wnoutrefresh(messages);
            scrollUpdated = false;
        }
        if (roomUpdated)
        {
            wclear(navBar);
            box(navBar, 0, 0);
            for (int i = 0; i < roomList.size(); i++)
            {
                if (i == currentRoom)
                {
                    wattron(navBar, A_STANDOUT);
                    mvwprintw(navBar, i + 1, 20, "***");
                }
                mvwprintw(navBar, i + 1, 1, roomList[i].c_str());
                wattroff(navBar, A_STANDOUT);
            }
            wnoutrefresh(navBar);
            roomUpdated = false;
        }

        doupdate();
        ch = getch(); // Wait for user input

        switch (ch)
        {
        case ERR:
            break;
        case KEY_UP:
            if (scroll != 0)
            {
                scroll--;
                scrollUpdated = true;
            }
            break;
        case KEY_DOWN:
            scroll++;
            scrollUpdated = true;
            break;
        case KEY_LEFT:
            if (currentRoom != 0)
            {
                currentRoom--;
                roomUpdated = true;
                std::string move = ".JOIN_ROOM " + roomList[currentRoom];
                send(clientSocket, move.c_str(), move.size(), 0);
            }
            break;
        case KEY_RIGHT:
            if (currentRoom < roomList.size() - 1)
            {
                currentRoom++;
                roomUpdated = true;
                std::string move = ".JOIN_ROOM " + roomList[currentRoom];
                send(clientSocket, move.c_str(), move.size(), 0);
            }
            break;
        case KEY_BACKSPACE:
        case '\b':
        case 127:
            if (chBuffer.size() != 0)
                chBuffer = chBuffer.substr(0, chBuffer.size() - 1);
            break;
        case KEY_ENTER:
        case '\n':
            if (!chBuffer.empty())
            {
                std::lock_guard<std::mutex> messageLock(messageMutex);
                if (chBuffer.starts_with(".JOIN_ROOM"))
                    messageList = {};
                if (chBuffer.starts_with(".EXIT"))
                    messageList = {};
                send(clientSocket, chBuffer.c_str(), chBuffer.size(), 0);
                chBuffer = "";
                std::lock_guard<std::mutex> msggupdLock(msgupdMutex);
                msgUpdated = true;
            }
            break;
        default:
            chBuffer += ch;
        }
    }
    closesocket(clientSocket);
    endwin(); // End curses mode
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

void recieveFromServer(int clientSocket)
{
    char buffer[4096];
    while (true)
    {
        std::memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        std::lock_guard<std::mutex> messageLock(messageMutex);
        std::lock_guard<std::mutex> msggupdLock(msgupdMutex);
        if (bytesReceived > 0)
        {
            std::string msg = std::string(buffer, bytesReceived);
            if(msg.contains('\x1F') && msg.contains('\x1E'))
            {
                std::lock_guard<std::mutex> roomLock(roomMutex);
                roomList = processList(msg.substr(0,msg.find('\x1E')),'\x1F');
                messageList = processList(msg.substr(msg.find('\x1E')),'\x1E');
                roomUpdated = true;
                msgUpdated = true;
            }
            else if (msg[0] == '\x1F')
            {
                std::lock_guard<std::mutex> roomLock(roomMutex);
                roomList = processList(msg, '\x1F');
                roomUpdated = true;
            }
            else if(msg[0] == '\x1E'){
                messageList = processList(msg, '\x1E');
                msgUpdated = true;
            }
            else
            {
                messageList.push_back(msg);
                msgUpdated = true;
            }
        }
        else if (bytesReceived == 0)
        {
            messageList.push_back("Server disconnected");
            msgUpdated = true;
            break;
        }
        else
        {
            std::cerr << "Receive failed\n";
            break;
        }
    }
}

std::vector<std::string> processList(std::string list, char delim)
{
    std::vector<std::string> result;
    list = list.substr(1);
    while (list.contains(delim))
    {
        int location = list.find(delim);
        result.push_back(list.substr(0, location));
        list = list.substr(location + 1);
    }
    result.push_back(list);
    return result;
}
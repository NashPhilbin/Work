#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <mutex>
#include <iterator>
#include <algorithm>
#include "ChatServer.h"

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

void clientThread(sockaddr_in clientAddr, int clientSocket);
void broadcast(char buffer[4096], int bytesReceived, int clientSocket);
std::vector<int> socketList;
std::mutex socketMutex;
std::mutex serverMutex;
ChatServer serverData;

int main(int argc, char* argv[])
{
    std::string serverIP = "127.0.0.1";
    int serverPort = 54000;
    if(argc >= 2) serverIP = argv[1];
    if(argc == 3) serverPort = std::stoi(argv[2]);
    std::cout << "Using IP: " << serverIP << " and port: " << serverPort << std::endl;
    serverData.createRoom("Lobby");
    
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        std::cerr << "Cannot create socket\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind failed\n";
        closesocket(listenSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) < 0)
    {
        std::cerr << "Listen failed\n";
        closesocket(listenSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    while(true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientSize = sizeof(clientAddr);


        int clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket < 0)
        {
            std::cerr << "Accept failed\n";
            closesocket(listenSocket);
    #ifdef _WIN32
            WSACleanup();
    #endif
            return 1;
        }
        char buffer[4096]; //Username handshake
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        std::string clientName(buffer, bytesReceived);
        std::lock_guard<std::mutex> serverLock(serverMutex);
        serverData.joinRoom(clientSocket, clientName, "Lobby");
        std::lock_guard<std::mutex> sockLock(socketMutex);
        socketList.push_back(clientSocket);
        std::string listRooms = serverData.listRoomsInternal();
        send(clientSocket, listRooms.c_str(), listRooms.length(), 0);
        std::string log = serverData.getMessages("Lobby");
        send(clientSocket, log.c_str(), log.length(), 0);
        std::thread newThread(clientThread, clientAddr, clientSocket);
        newThread.detach();

    }

closesocket(listenSocket);


#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

void clientThread(sockaddr_in clientAddr, int clientSocket){
        char clientIp[INET_ADDRSTRLEN];
    std::memset(clientIp, 0, sizeof(clientIp));

    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
    std::cout << "Client connected from "
        << clientIp << ":" << ntohs(clientAddr.sin_port) << '\n';

    char buffer[4096];

    while (true)
    {
        std::memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0)
        {
            std::string message(buffer, bytesReceived);
            std::cout << "Client: " << message << '\n';
            std::lock_guard<std::mutex> serverLock(serverMutex);
            if(message.starts_with(".CREATE_ROOM") && 13 < message.length()){
                serverData.createRoom(message.substr(13,message.length()));
                std::string listRooms = serverData.listRoomsInternal();
                std::lock_guard<std::mutex> sockLock(socketMutex);
                for(int socket : socketList){
                    send(socket, listRooms.c_str(), listRooms.length(), 0);
                }
            }else if(message.starts_with(".JOIN_ROOM") && 11 < message.length()){
                serverData.joinRoom(clientSocket, message.substr(11,message.length()));
                std::string log = serverData.getMessages(message.substr(11,message.length()));
                send(clientSocket, log.c_str(), log.length(), 0);
            }else if(message.starts_with(".LIST_ROOMS")){
                std::string list = serverData.listRooms();
                send(clientSocket, list.c_str(), list.length(), 0);
            }else if(message.starts_with(".EXIT")){
                serverData.exit(clientSocket);
            }else broadcast(buffer, bytesReceived, clientSocket);
        }
        else if (bytesReceived == 0)
        {
            std::lock_guard<std::mutex> serverLock(serverMutex);
            std::lock_guard<std::mutex> sockLock(socketMutex);
            std::erase(socketList, clientSocket);
            serverData.exit(clientSocket);
            std::cout << "Client disconnected\n";
            break;
        }
        else
        {
            std::lock_guard<std::mutex> serverLock(serverMutex);
            std::lock_guard<std::mutex> sockLock(socketMutex);
            std::erase(socketList, clientSocket); 
            serverData.exit(clientSocket);
            std::cerr << "Receive failed\n";
            break;
        }
    }

    closesocket(clientSocket);
}

void broadcast(char buffer[4096], int bytesReceived, int clientSocket){
    std::string message(buffer, bytesReceived);
    std::pair<std::string, std::vector<int>> msgPair = serverData.sendMessage(clientSocket, message);
    std::vector<int> sockets = msgPair.second;
    for(int socket : sockets){
        if (send(socket, msgPair.first.c_str(), msgPair.first.length(), 0) < 0)
        {
            std::cerr << "Send failed for " << socket << "\n";
        }
    }
}


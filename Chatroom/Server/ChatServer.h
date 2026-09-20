#ifndef CHATSERVER_H
#define CHATSERVER_H
#include "Room.h"
#include "User.h"


class ChatServer{
    public:
    void createRoom(std::string name);
    void joinRoom(int socket, std::string roomName);
    void joinRoom(int socket, std::string username, std::string roomName);
    std::string listRooms();
    std::string listRoomsInternal();
    std::string exit(int socket);
    std::pair<std::string, std::vector<int>> sendMessage(int socket, std::string message);
    std::string getMessages(std::string name);
    private:
    std::vector<Room> rooms;
};

#endif
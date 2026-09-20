#ifndef ROOM_H
#define ROOM_H
#include <string>
#include <vector>
#include "User.h"
#include <utility>

class Room{
    public:
    Room(std::string name);
    void addUser(int socket, std::string name);
    void removeUser(int socket);
    std::vector<int> getSockets();
    std::vector<std::string> getMessages();
    std::string getName();
    std::pair<std::string, std::vector<int>> logMsgGetSockets(int socket, std::string message);
    std::string getUserName(int socket);
    std::vector<std::string> addUserGetMsgs(int socket, std::string name);
    bool hasSocket(int socket);
    bool hasName(std::string name);
    
    private:
    std::string addMessage(int socket, std::string message);
    std::string roomName;
    std::vector<User> members;
    std::vector<std::string> messages;
};

#endif
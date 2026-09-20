#include "Room.h"
#include <iterator>
Room::Room(std::string name){
    roomName = name;
    messages.push_back("Hello, welcome to " + name + "!");
    if(name == "Lobby"){
        messages.push_back("Use the UP and DOWN arrow keys to scroll messages.");
        messages.push_back("Use the LEFT and RIGHT keys to change rooms.");
        messages.push_back("Use \".CREATE_ROOM <room_name>\" to create a room.");
        messages.push_back("Use \".JOIN_ROOM <room_name>\" to join a room.");
        messages.push_back("Use \".LIST_ROOMS\" to get a list of rooms.");
        messages.push_back("Use \".EXIT\" to exit room.");
    }
}
void Room::addUser(int socket, std::string name){
    members.emplace_back(name, socket);
}
void Room::removeUser(int socket){
    std::vector<User>::iterator it = members.begin();
    while(it < members.end()){
        if((*it).getSock() == socket){
            members.erase(it);
            return;
        }
        it++;
    }
}
std::string Room::addMessage(int socket, std::string message){
    std::string newMessage = getUserName(socket) + ": ";
    newMessage += message;
    messages.push_back(newMessage);
    return newMessage;
}
std::vector<int> Room::getSockets(){
    std::vector<int> sockets;
    for(User u : members){
        sockets.push_back(u.getSock());
    }
    return sockets;
}
std::vector<std::string> Room::getMessages(){
    return messages;
}
bool Room::hasName(std::string name){
    if(name == "-1") return false;
    for(User u : members){
        if(u.getName() == name)
            return true;
    }
    return false;
}
std::string Room::getName(){
    return roomName;
}
std::pair<std::string, std::vector<int>> Room::logMsgGetSockets(int socket, std::string message){
    return {addMessage(socket, message), getSockets()};
}
std::string Room::getUserName(int socket){
    for(User u : members){
        if(u.getSock() == socket) return u.getName();
    }
    return "Invalid Socket";
}
std::vector<std::string> Room::addUserGetMsgs(int socket, std::string name){
    addUser(socket, name);
    return getMessages();
}
bool Room::hasSocket(int socket){
    for(User u : members){
        if(u.getSock() == socket)
            return true;
    }
    return false;
}
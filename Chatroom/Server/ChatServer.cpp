#include "ChatServer.h"
void ChatServer::createRoom(std::string name){
    rooms.emplace_back(name);
}
void ChatServer::joinRoom(int socket, std::string roomName){
    for(Room& r : rooms){
        if(r.getName() == roomName)
            r.addUser(socket, exit(socket));
    }
}
void ChatServer::joinRoom(int socket, std::string username, std::string roomName){
    for(Room& r : rooms){
        if(r.hasName(username)) username = "-1";
    }
    for(Room& r : rooms){
        if(r.getName() == roomName)
            r.addUser(socket, username);
    }
}
std::string ChatServer::listRooms(){
    std::string list = "List of rooms: ";
    for(Room r : rooms){
        list += r.getName() + ", ";
    }
    list.erase(list.length()-2);
    return list;
}
std::string ChatServer::listRoomsInternal(){ //For communicating between server/client, not to user
    std::string list = "";
    for(Room r: rooms){
        list += '\x1F' + r.getName();
    }
    return list;
}
std::string ChatServer::exit(int socket){
    std::string name;
    for(Room& r : rooms){
        name = r.getUserName(socket);
        if(name != "Invalid Socket"){
            r.removeUser(socket);
            return name;
        }
    }
    return "User Not Found";
}
std::pair<std::string, std::vector<int>> ChatServer::sendMessage(int socket, std::string message){
    for(Room& r : rooms){
        if(r.hasSocket(socket))
            return r.logMsgGetSockets(socket, message);
    }
    return {};
}
std::string ChatServer::getMessages(std::string name){
    std::string list = "";
    for(Room r: rooms){
        if(r.getName() == name){
            for(std::string str : r.getMessages()){
                list += '\x1E' + str;
            }
        }
    }
    return list;
}
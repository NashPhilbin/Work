#include "User.h"
User::User(int sock){
    username = generateName();
    socket = sock;
}
User::User(std::string name, int sock){
    if(name == "-1") username = generateName();
    else username = name;
    socket = sock;
}
int User::getSock(){
    return socket;
}
std::string User::getName(){
    return username;
}
int User::anonCount = 101;
std::string User::generateName(){
    std::string name = "Anon" + std::to_string(anonCount);
    anonCount++;
    return name;
}
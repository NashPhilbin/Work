#ifndef USER_H
#define USER_H

#include <string>

class User{
    public:
    User(int sock);
    User(std::string name, int sock);
    int getSock();
    std::string getName();
    private:
    static int anonCount;
    std::string generateName();
    std::string username;
    int socket;
};

#endif
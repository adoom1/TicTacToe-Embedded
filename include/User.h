#ifndef USER_H
#define USER_H

#include <string>
#include <unordered_map>

class User {
public:
    bool signup(const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);
};

#endif

#include "../include/User.h"
#include <fstream>
#include <iostream>

bool User::signup(const std::string& username, const std::string& password) {
    std::ifstream infile("users.txt");
    std::string user, pass;
    
    while (infile >> user >> pass) {
        if (user == username) {
            std::cout << "Username already exists!\n";
            return false;
        }
    }

    infile.close();
    std::ofstream outfile("users.txt", std::ios::app);
    outfile << username << " " << password << std::endl;
    std::cout << "Signup successful!\n";
    return true;
}

bool User::login(const std::string& username, const std::string& password) {
    std::ifstream infile("users.txt");
    std::string user, pass;

    while (infile >> user >> pass) {
        if (user == username && pass == password) {
            std::cout << "Login successful!\n";
            return true;
        }
    }

    std::cout << "Invalid username or password.\n";
    return false;
}

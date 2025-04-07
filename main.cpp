#include <iostream>
#include "include/Game.h"
#include "include/User.h"



int main() {
    User user;
    int choice;
    std::string username, password;

    std::cout << "1. Sign Up\n2. Log In\nEnter choice: ";
    std::cin >> choice;

    while (true) {
        std::cout << "Username: ";
        std::cin >> username;
        std::cout << "Password: ";
        std::cin >> password;
    
        if (user.login(username, password)) break;
    
        std::cout << "Incorrect username or password. Try again.\n";
    }
    
    int mode;
    std::cout << "\nChoose Game Mode:\n1. Two Players\n2. Play vs AI\n> ";
    std::cin >> mode;
    
    Game game;
    int row, col;
    
    while (true) {
        game.displayBoard();
    
        if (mode == 2 && game.getCurrentPlayer() == 'O') {
            std::cout << "AI is thinking...\n";
            game.makeAIMoveWithTree();

        } else {
            std::cout << "Player " << game.getCurrentPlayer() << ", enter your move (row and col): ";
            std::cin >> row >> col;
    
            if (!game.makeMove(row, col)) {
                std::cout << "Invalid move. Try again.\n";
                continue;
            }
        }
    
        if (game.checkWin()) {
            game.displayBoard();
            std::cout << "Player " << game.getCurrentPlayer() << " wins!\n";
            break;
        }
    
        if (game.checkDraw()) {
            game.displayBoard();
            std::cout << "It's a draw!\n";
            break;
        }
    
        game.switchPlayer();
    }
    
    
    return 0;
}


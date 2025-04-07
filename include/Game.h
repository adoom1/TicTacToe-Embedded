#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>

struct TreeNode {
    char board[3][3];
    char player; // 'X' or 'O'
    std::vector<TreeNode*> children;
    int score;

    TreeNode(char b[3][3], char p) {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                board[i][j] = b[i][j];
        player = p;
        score = 0;
    }
};

class Game {
private:
    std::vector<std::vector<char>> board;
    char currentPlayer;
    int minimax(int depth, bool isMaximizing);
public:
    Game();
    void displayBoard();
    bool makeMove(int row, int col);
    bool checkWin();
    bool checkDraw();
    void switchPlayer();
    char getCurrentPlayer();
    TreeNode* buildGameTree(char b[3][3], char currentPlayer);
int evaluateBoard(char b[3][3]);
int minimaxTree(TreeNode* node, bool isMaximizing);
void makeAIMoveWithTree(); // Replaces previous AI logic

};



#endif // GAME_H

#include <iostream>
#include "Game.h"

Game::Game() {
    board = std::vector<std::vector<char>>(3, std::vector<char>(3, ' '));
    currentPlayer = 'X';
}

void Game::displayBoard() {
    std::cout << "\n";
    for (int i = 0; i < 3; i++) {
        std::cout << " ";
        for (int j = 0; j < 3; j++) {
            std::cout << board[i][j];
            if (j < 2) std::cout << " | ";
        }
        std::cout << "\n";
        if (i < 2) std::cout << "---+---+---\n";
    }
    std::cout << "\n";
}

bool Game::makeMove(int row, int col) {
    if (row < 0 || row > 2 || col < 0 || col > 2) return false;
    if (board[row][col] != ' ') return false;

    board[row][col] = currentPlayer;
    return true;
}

bool Game::checkWin() {
    for (int i = 0; i < 3; i++) {
        // rows and columns
        if ((board[i][0] == currentPlayer &&
             board[i][1] == currentPlayer &&
             board[i][2] == currentPlayer) ||
            (board[0][i] == currentPlayer &&
             board[1][i] == currentPlayer &&
             board[2][i] == currentPlayer))
            return true;
    }
    // diagonals
    if ((board[0][0] == currentPlayer &&
         board[1][1] == currentPlayer &&
         board[2][2] == currentPlayer) ||
        (board[0][2] == currentPlayer &&
         board[1][1] == currentPlayer &&
         board[2][0] == currentPlayer))
        return true;

    return false;
}

bool Game::checkDraw() {
    for (auto& row : board) {
        for (auto& cell : row) {
            if (cell == ' ') return false;
        }
    }
    return !checkWin(); // draw only if there's no winner
}

void Game::switchPlayer() {
    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
}

char Game::getCurrentPlayer() {
    return currentPlayer;
}
int Game::evaluateBoard(char b[3][3]) {
    // Check rows, columns and diagonals for win
    for (int i = 0; i < 3; ++i) {
        if (b[i][0] == b[i][1] && b[i][1] == b[i][2] && b[i][0] != ' ')
            return (b[i][0] == 'O') ? 1 : -1;
        if (b[0][i] == b[1][i] && b[1][i] == b[2][i] && b[0][i] != ' ')
            return (b[0][i] == 'O') ? 1 : -1;
    }
    if (b[0][0] == b[1][1] && b[1][1] == b[2][2] && b[0][0] != ' ')
        return (b[0][0] == 'O') ? 1 : -1;
    if (b[0][2] == b[1][1] && b[1][1] == b[2][0] && b[0][2] != ' ')
        return (b[0][2] == 'O') ? 1 : -1;
    return 0;
}

bool isDraw(char b[3][3]) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (b[i][j] == ' ') return false;
    return true;
}

TreeNode* Game::buildGameTree(char b[3][3], char currentPlayer) {
    TreeNode* node = new TreeNode(b, currentPlayer);
    int score = evaluateBoard(b);
    if (score != 0 || isDraw(b)) {
        node->score = score;
        return node;
    }

    char nextPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (b[i][j] == ' ') {
                char newBoard[3][3];
                for (int x = 0; x < 3; ++x)
                    for (int y = 0; y < 3; ++y)
                        newBoard[x][y] = b[x][y];

                newBoard[i][j] = currentPlayer;
                TreeNode* child = buildGameTree(newBoard, nextPlayer);
                node->children.push_back(child);
            }
        }
    }

    return node;
}

int Game::minimaxTree(TreeNode* node, bool isMaximizing) {
    if (node->children.empty()) return node->score;

    int bestScore = isMaximizing ? -1000 : 1000;
    for (TreeNode* child : node->children) {
        int score = minimaxTree(child, !isMaximizing);
        bestScore = isMaximizing ? std::max(score, bestScore) : std::min(score, bestScore);
    }

    node->score = bestScore;
    return bestScore;
}

void Game::makeAIMoveWithTree() {
    char tempBoard[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            tempBoard[i][j] = board[i][j]; // convert vector to array

    TreeNode* root = buildGameTree(tempBoard, 'O');
    minimaxTree(root, true);

    int bestScore = -1000;
    TreeNode* bestMove = nullptr;
    for (TreeNode* child : root->children) {
        if (child->score > bestScore) {
            bestScore = child->score;
            bestMove = child;
        }
    }

    if (bestMove) {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                board[i][j] = bestMove->board[i][j]; // update the actual vector board
    }

    delete root; // optional: can implement tree delete recursively if needed
}


#include <iostream>
#include <vector>
#include <limits>
#include <fcntl.h>
using namespace std;

bool isValidStrategy(const string &strategy)
{
    // Check if the strategy number has exactly 9 digits
    if (strategy.size() != 9)
    {
        return false;
    }

    // Check if each digit from 1 to 9 appears exactly once
    for (char digit = '1'; digit <= '9'; digit++)
    {
        if (strategy.find(digit) == string::npos)
        {
            return false;
        }
    }

    // Check if the strategy number contains only digits
    for (char ch : strategy)
    {
        if (!isdigit(ch))
        {
            return false;
        }
    }

    // All checks passed, the strategy number is valid
    return true;
}

void printBoard(const vector<vector<int>> &board)
{
    for (int i = 0; i < 3; i++)
    {
        cout << "-------" << endl;
        for (int j = 0; j < 3; j++)
        {
            cout << "|";
            if (board[i][j] == 0)
            {
                cout << " ";
            }
            else if (board[i][j] == 1)
            {
                cout << "X";
            }
            else
            {
                cout << "O";
            }
        }
        cout << "|" << endl;
    }
    cout << "-------" << endl;
    cout << endl;
}

int checkWinner(const vector<vector<int>> &board)
{
    // Check rows
    for (int i = 0; i < 3; i++)
    {
        if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2])
        {
            return board[i][0];
        }
    }

    // Check columns
    for (int j = 0; j < 3; j++)
    {
        if (board[0][j] != 0 && board[0][j] == board[1][j] && board[1][j] == board[2][j])
        {
            return board[0][j];
        }
    }

    // Check diagonals
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2])
    {
        return board[0][0];
    }
    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0])
    {
        return board[0][2];
    }

    // No winner
    return 0;
}

void LunarDeportasion()
{
    cout << "Lunar Deportasion" << endl;
    cout << "Sending humans to the moon..." << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << "<strategy>" << endl;
        return 1;
    }

    string strategy = argv[1];
    if (!isValidStrategy(strategy))
    {
        cerr << "Invalid strategy number" << endl;
        return 1;
    }

    // Create a 3x3 board
    // 0 - empty cell
    // 1 - AI
    // 2 - player
    vector<vector<int>> board(3, vector<int>(3, 0));

    // Print the initial board
    printBoard(board);

    // 123456789

    int ai = 0;
    for (int i = 0; i < 9; i++)
    {
        if (i % 2 == 0)
        {
            cout << "AI's turn" << endl;
            int cell = strategy[ai] - '0';
            if (board[(cell - 1) / 3][(cell - 1) % 3] == 0)
            {
                board[(cell - 1) / 3][(cell - 1) % 3] = 1;
                ai++;
            }
            else
            {
                for (int j = ai; j < 9; j++)
                {
                    if (board[(strategy[j] - '0' - 1) / 3][(strategy[j] - '0' - 1) % 3] == 0)
                    {
                        board[(strategy[j] - '0' - 1) / 3][(strategy[j] - '0' - 1) % 3] = 1;
                        ai = j + 1;
                        break;
                    }
                }
            }
            printBoard(board);
            if (i >= 4)
            {
                int winner = checkWinner(board);
                if (winner == 1)
                {
                    cout << "I Win" << endl;
                    LunarDeportasion();
                    return 0;
                }
                else if (winner == 2)
                {
                    cout << "I Lose" << endl;
                    return 0;
                }
            }
        }
        else
        {
            cout << "Player's turn" << endl;
            cout << "Enter the cell number: ";
            int cell;

            while (!(cin >> cell) || cell < 1 || cell > 9 || board[(cell - 1) / 3][(cell - 1) % 3] != 0 )
            {
                // check if end if file
                if (cin.eof())
                {
                    exit(1);
                }
                else if (!isdigit(cell))
                {
                    cin.clear();
                }
                else if (cin.fail())
                {
                    exit(1);
                }
                cout << "Invalid cell number. Enter a valid cell number: " << cell << endl;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
            board[(cell - 1) / 3][(cell - 1) % 3] = 2;
            printBoard(board);
            if (i >= 4)
            {
                int winner = checkWinner(board);
                if (winner == 1)
                {
                    cout << "I Win" << endl;
                    LunarDeportasion();
                    return 0;
                }
                else if (winner == 2)
                {
                    cout << "I Lose" << endl;
                    return 0;
                }
            }
        }
    }
    // No winner=
    cout << "It's a draw" << endl;

    return 0;
}
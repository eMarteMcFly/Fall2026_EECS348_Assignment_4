/**
 * @file Assignment4.cpp
 * @name EECS 348 Assignment 4 - Object-Oriented Sudoku Solver
 * @brief An object-oriented program that solves partially completed Sudoku 
 *        puzzles using a recursive backtracking (depth-first search) algorithm.
 * @inputs puzzle1.txt, puzzle2.txt, puzzle3.txt, puzzle4.txt, puzzle5.txt
 * @output Puzzle file name, initial puzzle grid, and solved puzzle grid (or "No solution found")
 * @collaborators None (Authored independently following course guidelines)
 * @author Student Name
 * @creation_date October 15, 2026[cite: 7]
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

/**
 * @class SudokuSolver
 * @brief Encapsulates a 9x9 Sudoku grid, file input/output, and recursive backtracking solver logic.
 * @authorship Authored by Student Name.
 */
class SudokuSolver {
private:
    // 9x9 grid representing the Sudoku board where 0 represents an empty cell
    int board[9][9];

    /**
     * @brief Checks if placing a number at a given row and column is valid.
     * @param row The row index (0-8)
     * @param col The column index (0-8)
     * @param num The digit to check (1-9)
     * @return true if valid, false otherwise
     * @authorship Authored by Student Name.
     */
    bool isValid(int row, int col, int num) {
        // Loop through each column in the current row to check for duplicates
        for (int c = 0; c < 9; c++) {
            if (board[row][c] == num) {
                return false; // Duplicate found in row
            }
        }

        // Loop through each row in the current column to check for duplicates
        for (int r = 0; r < 9; r++) {
            if (board[r][col] == num) {
                return false; // Duplicate found in column
            }
        }

        // Determine the top-left starting coordinates of the 3x3 sub-grid (block)
        int startRow = row - (row % 3);
        int startCol = col - (col % 3);

        // Loop through the 3x3 block to check for duplicates
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                if (board[startRow + r][startCol + c] == num) {
                    return false; // Duplicate found in 3x3 block
                }
            }
        }

        return true; // Position is valid for the given number
    }

    /**
     * @brief Recursive backtracking function to solve the Sudoku puzzle.
     * @return true if a solution is found, false otherwise
     * @authorship Authored by Student Name.
     */
    bool solveRecursive() {
        int row = -1;
        int col = -1;
        bool isEmpty = false;

        // Scan the board to find the next empty cell (represented by 0)
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                if (board[r][c] == 0) {
                    row = r;
                    col = c;
                    isEmpty = true;
                    break; // Found an empty cell, exit inner loop
                }
            }
            if (isEmpty) {
                break; // Exit outer loop once empty cell is located
            }
        }

        // If no empty cells remain, the puzzle is successfully solved
        if (!isEmpty) {
            return true;
        }

        // Try candidate digits from 1 to 9 for the empty cell
        for (int num = 1; num <= 9; num++) {
            // Check if candidate digit is valid according to Sudoku rules
            if (isValid(row, col, num)) {
                board[row][col] = num; // Temporarily assign candidate value

                // Recursively attempt to solve the rest of the board
                if (solveRecursive()) {
                    return true; // Solution found
                }

                board[row][col] = 0; // Backtrack if assignment leads to a dead end
            }
        }

        return false; // Trigger backtracking if no candidates work
    }

public:
    /**
     * @brief Default constructor initializing the board to zeros.
     * @authorship Authored by Student Name.
     */
    SudokuSolver() {
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                board[r][c] = 0;
            }
        }
    }

    /**
     * @brief Loads a Sudoku puzzle from a specified text file.
     * @param filename The path to the puzzle file (e.g., puzzle1.txt)[cite: 7]
     * @return true if loaded successfully, false otherwise
     * @authorship Authored by Student Name.
     */
    bool loadPuzzle(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            return false; // Return false if file cannot be opened
        }

        char ch;
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                if (!(file >> ch)) {
                    return false; // Handle unexpected end of file
                }
                // Convert underscore character to 0, or character digit to integer value
                if (ch == '_') {
                    board[r][c] = 0;
                } else if (ch >= '1' && ch <= '9') {
                    board[r][c] = ch - '0';
                } else {
                    board[r][c] = 0; // Default fallback for malformed input
                }
            }
        }
        file.close();
        return true;
    }

    /**
     * @brief Prints the current state of the Sudoku board to standard output.
     * @authorship Authored by Student Name.
     */
    void printBoard() const {
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                if (board[r][c] == 0) {
                    cout << "_ "; // Print underscore for empty cells
                } else {
                    cout << board[r][c] << " ";
                }
            }
            cout << endl; // Newline at the end of each row
        }
    }

    /**
     * @brief Public interface to execute the solving algorithm.
     * @return true if solved, false if no solution exists
     * @authorship Authored by Student Name.
     */
    bool solve() {
        return solveRecursive();
    }
};

/**
 * @brief Main execution function processing all five required puzzle files[cite: 7].
 * @return int Exit status code
 * @authorship Authored by Student Name.
 */
int main() {
    // Array containing the five required puzzle filenames[cite: 7]
    string puzzleFiles[5] = {
        "puzzle1.txt",
        "puzzle2.txt",
        "puzzle3.txt",
        "puzzle4.txt",
        "puzzle5.txt"
    };

    // Iterate through each puzzle file sequentially
    for (int i = 0; i < 5; i++) {
        SudokuSolver solver;

        // Output the puzzle file name header[cite: 7]
        cout << "========================================" << endl;
        cout << "Puzzle File: " << puzzleFiles[i] << endl;
        cout << "========================================" << endl;

        // Attempt to load the puzzle from disk
        if (!solver.loadPuzzle(puzzleFiles[i])) {
            cout << "Error: Could not open or read " << puzzleFiles[i] << endl;
            continue;
        }

        // Print the initial puzzle loaded from the file[cite: 7]
        cout << "Initial Puzzle:" << endl;
        solver.printBoard();
        cout << "----------------------------------------" << endl;

        // Execute the recursive solver and print the outcome[cite: 7, 9]
        if (solver.solve()) {
            cout << "Solution Found:" << endl;
            solver.printBoard();
        } else {
            cout << "No solution found" << endl; // Required fallback message[cite: 7]
        }
        cout << endl;
    }

    return 0;
}
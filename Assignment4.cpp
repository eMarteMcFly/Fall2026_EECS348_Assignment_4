/*
 * ============================================================================
 * Program Name : EECS 348 Assignment 4 - Object-Oriented Sudoku Solver
 * File         : Assignment4.cpp
 *
 * Description  : Reads Sudoku puzzles from text files, prints each puzzle, and
 *                solves it with a recursive depth-first search with
 *                backtracking. EVERY solution is printed (some puzzles have
 *                more than one); if none exists, "No solution found" is
 *                printed. Malformed files and puzzles whose given digits
 *                already break a Sudoku rule are detected and reported.
 *
 * Inputs       : puzzle1.txt, puzzle2.txt, puzzle3.txt, puzzle4.txt, puzzle5.txt
 *                from the current directory (default), or any puzzle file
 *                names passed on the command line:
 *                    ./Assignment4.exe myPuzzle.txt
 *                File format: 9 non-blank lines, each with 9 entries separated
 *                by whitespace. An entry is a digit 1-9 (a given) or '_'
 *                (a blank square). Blank lines are ignored.
 *
 * Output       : For each puzzle file, to standard output:
 *                  - the puzzle file name
 *                  - the puzzle exactly as read from the file
 *                  - every solution, numbered, or "No solution found"
 *                  - the total number of solutions found
 *                  - or, for a malformed file, an error message saying what
 *                    is wrong and where
 *
 * Collaborators: None
 *
 * Other sources:
 *   - Google Gemini (Gemini web app): generated the original program from the
 *     shared assignment prompt: the SudokuSolver class layout, file loading,
 *     board printing, recursive backtracking search, and main loop.
 *   - Claude (Anthropic, claude.ai): helped revise the Gemini program to
 *     (1) print all solutions instead of stopping at the first one,
 *     (2) validate the input file and the given digits,
 *     (3) replace 27-cell rule scans with per-row/column/box bitmasks and
 *         branch on the most constrained empty cell (faster search),
 *     (4) store the board in 1-byte cells and print solutions as they are
 *         found instead of keeping copies (less memory),
 *     (5) add named constants, error messages, and line-by-line comments.
 *
 * Author       : Erick Marte
 * Created      : October 15, 2026
 * Revised      : October 15, 2026
 *
 * Build (EECS Cycle servers):
 *     g++ -std=c++11 -O2 -Wall -Wextra -o Assignment4.exe Assignment4.cpp
 * Run:
 *     ./Assignment4.exe
 * ============================================================================
 */

#include <cstdint>    // std::uint8_t and std::uint16_t: fixed-size compact integer types
#include <fstream>    // std::ifstream: opens and reads the puzzle files
#include <iostream>   // std::cout and std::ostream: console output
#include <sstream>    // std::istringstream: splits one line of a file into entries
#include <string>     // std::string: file names, entries, and error messages

/* ============================================================================
 * Class SudokuSolver
 * Origin: Gemini (original class: board, loading, printing, recursive solve).
 *         Revised with Claude's help: bitmask bookkeeping, input validation,
 *         most-constrained-cell choice, and printing every solution.
 *
 * Holds one 9x9 board. Alongside the board it keeps three groups of bitmasks
 * (one per row, one per column, one per 3x3 box). In each mask, bit d is 1
 * when digit d is already used in that unit. Checking whether a digit fits
 * in a cell then costs a few bit operations instead of scanning 27 cells.
 * ============================================================================ */
class SudokuSolver {
public:
    static const int SIZE = 9;            // the board has 9 rows and 9 columns
    static const int BOX = 3;             // each 3x3 box is 3 cells on a side
    static const int EMPTY = 0;           // value stored in a blank square

    SudokuSolver();                                                     // starts with a blank board
    bool loadPuzzle(const std::string& filename, std::string& error);  // reads and validates a puzzle file
    void printBoard(std::ostream& out) const;                          // prints the board in file format
    bool givensAreValid() const;                                       // false if the givens break a rule
    const std::string& givensProblem() const;                          // describes the first broken rule
    long long solveAll(std::ostream& out);                             // prints every solution, returns count

private:
    static const std::uint16_t ALL_DIGITS = 0x3FE;  // bits 1 through 9 set: "every digit allowed"

    void reset();                                        // clears the board and all bookkeeping
    static int boxOf(int row, int col);                  // which 3x3 box (0-8) a cell belongs to
    static std::uint16_t bitFor(int digit);              // the mask bit that stands for a digit
    static int countBits(std::uint16_t mask);            // how many digits a mask allows
    std::uint16_t candidatesFor(int row, int col) const; // digits that may legally go in a cell
    void placeDigit(int row, int col, int digit);        // writes a digit and marks it used
    void removeDigit(int row, int col);                  // erases a digit and unmarks it
    bool chooseCell(int& row, int& col,
                    std::uint16_t& candidates) const;    // finds the best empty cell to try next
    void solveRecursive(std::ostream& out);              // the recursive backtracking search

    std::uint8_t board[SIZE][SIZE];   // 0 for blank or 1-9; one byte per cell (81 bytes total)
    std::uint16_t rowUsed[SIZE];      // rowUsed[r]: digits already in row r
    std::uint16_t colUsed[SIZE];      // colUsed[c]: digits already in column c
    std::uint16_t boxUsed[SIZE];      // boxUsed[b]: digits already in box b
    bool givensValid;                 // true while no given digit repeats in a row/column/box
    std::string givensIssue;          // explanation of the first repeated given, if any
    long long solutionCount;          // solutions found so far by solveAll()
};

/* ----------------------------------------------------------------------------
 * Constructor
 * Origin: Gemini (zero-filled board); revised with Claude's help to share
 *         reset() so the masks and flags start cleared too.
 * ---------------------------------------------------------------------------- */
SudokuSolver::SudokuSolver() {
    reset();                                              // start from an empty, valid board
}

/* ----------------------------------------------------------------------------
 * reset: blank every cell, clear every mask, and mark the givens as valid.
 * Origin: written with Claude's help (new helper).
 * ---------------------------------------------------------------------------- */
void SudokuSolver::reset() {
    for (int r = 0; r < SIZE; ++r) {                      // go through each row
        for (int c = 0; c < SIZE; ++c) {                  // go through each column in that row
            board[r][c] = EMPTY;                          // make the cell blank
        }
        rowUsed[r] = 0;                                   // row r has no digits yet
        colUsed[r] = 0;                                   // column r has no digits yet
        boxUsed[r] = 0;                                   // box r has no digits yet
    }
    givensValid = true;                                   // an empty board breaks no rule
    givensIssue.clear();                                  // so there is nothing to explain
    solutionCount = 0;                                    // and no solutions have been found
}

/* ----------------------------------------------------------------------------
 * loadPuzzle: read a puzzle file line by line and check its format.
 *   - each non-blank line must hold exactly 9 entries;
 *   - there must be exactly 9 non-blank lines;
 *   - each entry must be '_' or a single digit 1-9.
 * On a format problem it fills `error` and returns false. A given digit that
 * repeats in a row, column, or box does NOT stop loading (the puzzle can
 * still be printed); it is recorded so solveAll() can report no solution.
 * Origin: Gemini (character-by-character reader); rewritten with Claude's
 *         help to read whole entries, reject bad input, and track masks.
 * ---------------------------------------------------------------------------- */
bool SudokuSolver::loadPuzzle(const std::string& filename, std::string& error) {
    reset();                                              // forget any earlier puzzle
    std::ifstream file(filename.c_str());                 // try to open the file
    if (!file.is_open()) {                                // the file is missing or unreadable
        error = "could not open " + filename;             // say which file failed
        return false;                                     // stop: nothing to load
    }

    std::string line;                                     // one line of text from the file
    int lineNumber = 0;                                   // line count, used in error messages
    int row = 0;                                          // board row the next non-blank line fills
    while (std::getline(file, line)) {                    // read the file one line at a time
        ++lineNumber;                                     // we are now on the next line
        std::istringstream entries(line);                 // lets us pull entries out of this line
        std::string entry;                                // one entry, such as "5" or "_"
        int col = 0;                                      // board column the next entry fills

        while (entries >> entry) {                        // pull each whitespace-separated entry
            if (row >= SIZE) {                            // a 10th non-blank line has appeared
                error = "more than 9 rows (extra data on line " +
                        std::to_string(lineNumber) + ")"; // point to the extra line
                return false;                             // reject the file
            }
            if (col >= SIZE) {                            // this line already had 9 entries
                error = "line " + std::to_string(lineNumber) +
                        " has more than 9 entries";       // point to the long line
                return false;                             // reject the file
            }

            if (entry == "_") {                           // an underscore means a blank square
                board[row][col] = EMPTY;                  // store it as blank
            } else if (entry.size() == 1 && entry[0] >= '1' && entry[0] <= '9') { // one digit 1-9
                int digit = entry[0] - '0';               // turn the character into its number
                if (candidatesFor(row, col) & bitFor(digit)) { // digit not yet in this row/col/box
                    placeDigit(row, col, digit);          // store it and update the masks
                } else if (givensValid) {                 // first rule violation in this file
                    givensValid = false;                  // remember the puzzle is unsolvable
                    givensIssue = "digit " + entry + " at row " + std::to_string(row + 1) +
                                  ", column " + std::to_string(col + 1) +
                                  " repeats in its row, column, or box"; // explain where
                    board[row][col] = static_cast<std::uint8_t>(digit); // keep it so it prints
                } else {                                  // a later violation in the same file
                    board[row][col] = static_cast<std::uint8_t>(digit); // keep it so it prints
                }
            } else {                                      // anything else is not allowed
                error = "invalid entry '" + entry + "' on line " +
                        std::to_string(lineNumber) + " (use 1-9 or _)"; // show the bad entry
                return false;                             // reject the file
            }
            ++col;                                        // move to the next column
        }

        if (col == 0) {                                   // the line was empty or only spaces
            continue;                                     // ignore it (puzzle2.txt ends with some)
        }
        if (col != SIZE) {                                // the line had 1 to 8 entries
            error = "line " + std::to_string(lineNumber) + " has " +
                    std::to_string(col) + " entries (expected 9)"; // describe the short line
            return false;                                 // reject the file
        }
        ++row;                                            // this row is complete; go to the next
    }

    if (row != SIZE) {                                    // the file ended before 9 rows
        error = "found " + std::to_string(row) + " rows (expected 9)"; // describe the short file
        return false;                                     // reject the file
    }
    return true;                                          // the file is well formed
}

/* ----------------------------------------------------------------------------
 * printBoard: print the board in the same layout as the puzzle files.
 * Origin: Gemini; revised with Claude's help to print to any stream, avoid
 *         a trailing space, and print the 1-byte cells as numbers.
 * ---------------------------------------------------------------------------- */
void SudokuSolver::printBoard(std::ostream& out) const {
    for (int r = 0; r < SIZE; ++r) {                      // go through each row
        for (int c = 0; c < SIZE; ++c) {                  // go through each column
            if (c > 0) {                                  // all cells except the first in a row
                out << ' ';                               // are preceded by one space
            }
            if (board[r][c] == EMPTY) {                   // a blank square
                out << '_';                               // is shown as an underscore
            } else {                                      // a filled square
                out << static_cast<int>(board[r][c]);     // is shown as its digit (cast so the
            }                                             //   byte prints as a number, not a char)
        }
        out << '\n';                                      // finish the row
    }
}

/* ----------------------------------------------------------------------------
 * givensAreValid / givensProblem: report whether the given digits obey the
 * rules, and if not, which digit broke them first.
 * Origin: written with Claude's help (new).
 * ---------------------------------------------------------------------------- */
bool SudokuSolver::givensAreValid() const {
    return givensValid;                                   // recorded by loadPuzzle()
}

const std::string& SudokuSolver::givensProblem() const {
    return givensIssue;                                   // empty when the givens are fine
}

/* ----------------------------------------------------------------------------
 * solveAll: find and print every solution; return how many there were.
 * The board is restored to the original puzzle when the search finishes,
 * because every digit the search places is removed again on the way back.
 * Origin: Gemini's solve() wrapper; revised with Claude's help to search for
 *         all solutions and to skip the search when the givens are invalid.
 * ---------------------------------------------------------------------------- */
long long SudokuSolver::solveAll(std::ostream& out) {
    solutionCount = 0;                                    // nothing found yet
    if (!givensValid) {                                   // the givens already break a rule
        return 0;                                         // so no solution can exist
    }
    solveRecursive(out);                                  // run the full search
    return solutionCount;                                 // total solutions printed
}

/* ----------------------------------------------------------------------------
 * solveRecursive: one step of the depth-first search with backtracking.
 *   1. Choose the empty cell with the fewest legal digits.
 *   2. No empty cell left  -> the board is solved: print it, then return so
 *      the caller keeps trying other digits (this is how ALL solutions are
 *      found; Gemini's version returned true here and stopped searching).
 *   3. A cell with zero legal digits -> dead end: the loop below does
 *      nothing and we return, which backtracks.
 *   4. Otherwise place each legal digit in turn, recurse, then remove it.
 * Recursion depth is at most the number of blank cells (81 or fewer).
 * Origin: Gemini (recursive structure); revised with Claude's help.
 * ---------------------------------------------------------------------------- */
void SudokuSolver::solveRecursive(std::ostream& out) {
    int row = 0;                                          // row of the cell we will fill
    int col = 0;                                          // column of the cell we will fill
    std::uint16_t candidates = 0;                         // digits allowed in that cell

    if (!chooseCell(row, col, candidates)) {              // there are no blank cells left
        ++solutionCount;                                  // count this solution
        out << "\nSolution " << solutionCount << ":\n";   // label it
        printBoard(out);                                  // print it right away (no copy kept)
        return;                                           // go back and look for more
    }

    for (int digit = 1; digit <= SIZE; ++digit) {         // consider digits 1 through 9
        if (candidates & bitFor(digit)) {                 // skip digits that break a rule here
            placeDigit(row, col, digit);                  // try this digit
            solveRecursive(out);                          // solve the rest with it in place
            removeDigit(row, col);                        // undo it (backtrack) and try the next
        }
    }
}

/* ----------------------------------------------------------------------------
 * chooseCell: scan the blank cells and pick the one with the FEWEST legal
 * digits ("most constrained cell"). Fewer choices per step means far fewer
 * branches to explore. Returns false when the board has no blank cell.
 * Returns early on a cell with 0 choices (dead end) or 1 choice (forced).
 * Origin: written with Claude's help; replaces Gemini's "first blank cell"
 *         scan.
 * ---------------------------------------------------------------------------- */
bool SudokuSolver::chooseCell(int& row, int& col, std::uint16_t& candidates) const {
    bool foundBlank = false;                              // no blank cell seen yet
    int fewest = SIZE + 1;                                // more choices than any cell can have
    for (int r = 0; r < SIZE; ++r) {                      // go through each row
        for (int c = 0; c < SIZE; ++c) {                  // go through each column
            if (board[r][c] != EMPTY) {                   // filled cells need no choice
                continue;                                 // skip to the next cell
            }
            std::uint16_t options = candidatesFor(r, c);  // digits allowed here
            int count = countBits(options);               // how many there are
            if (count < fewest) {                         // tighter than the best so far
                foundBlank = true;                        // there is at least one blank cell
                fewest = count;                           // remember this cell's count
                row = r;                                  // remember its row
                col = c;                                  // remember its column
                candidates = options;                     // remember its allowed digits
                if (count <= 1) {                         // 0 = dead end, 1 = forced move
                    return true;                          // no cell can beat this; stop now
                }
            }
        }
    }
    return foundBlank;                                    // false means the board is full
}

/* ----------------------------------------------------------------------------
 * candidatesFor: digits that can legally go in (row, col). A digit is
 * allowed when it is not already used in that row, column, or box.
 * Origin: written with Claude's help; replaces Gemini's isValid(), which
 *         scanned 27 cells for every digit it tested.
 * ---------------------------------------------------------------------------- */
std::uint16_t SudokuSolver::candidatesFor(int row, int col) const {
    std::uint16_t used = rowUsed[row] | colUsed[col] | boxUsed[boxOf(row, col)]; // digits taken nearby
    return static_cast<std::uint16_t>(ALL_DIGITS & ~used);                      // all the others are allowed
}

/* ----------------------------------------------------------------------------
 * placeDigit / removeDigit: write or erase a digit and keep the masks in step.
 * Origin: written with Claude's help (new; Gemini wrote board cells directly).
 * ---------------------------------------------------------------------------- */
void SudokuSolver::placeDigit(int row, int col, int digit) {
    std::uint16_t bit = bitFor(digit);                    // the mask bit for this digit
    board[row][col] = static_cast<std::uint8_t>(digit);   // write the digit into the cell
    rowUsed[row] |= bit;                                  // the row now uses this digit
    colUsed[col] |= bit;                                  // the column now uses this digit
    boxUsed[boxOf(row, col)] |= bit;                      // the box now uses this digit
}

void SudokuSolver::removeDigit(int row, int col) {
    std::uint16_t bit = bitFor(board[row][col]);          // the mask bit for the digit being erased
    board[row][col] = EMPTY;                              // make the cell blank again
    rowUsed[row] &= static_cast<std::uint16_t>(~bit);     // the row no longer uses it
    colUsed[col] &= static_cast<std::uint16_t>(~bit);     // the column no longer uses it
    boxUsed[boxOf(row, col)] &= static_cast<std::uint16_t>(~bit); // the box no longer uses it
}

/* ----------------------------------------------------------------------------
 * Small helpers.
 * Origin: written with Claude's help. boxOf() uses the same idea as Gemini's
 *         startRow/startCol calculation in isValid().
 * ---------------------------------------------------------------------------- */
int SudokuSolver::boxOf(int row, int col) {
    return (row / BOX) * BOX + (col / BOX);               // boxes numbered 0-8, left to right, top to bottom
}

std::uint16_t SudokuSolver::bitFor(int digit) {
    return static_cast<std::uint16_t>(1u << digit);       // digit d is stored in bit d
}

int SudokuSolver::countBits(std::uint16_t mask) {
    int count = 0;                                        // number of 1 bits seen so far
    while (mask != 0) {                                   // until no 1 bits remain
        mask = static_cast<std::uint16_t>(mask & (mask - 1)); // clear the lowest 1 bit
        ++count;                                          // and count it
    }
    return count;                                         // how many digits the mask allows
}

/* ============================================================================
 * main: solve each puzzle file and print the results.
 * With no command-line arguments the five assignment files are used; any
 * arguments given are treated as puzzle file names instead (handy for
 * testing invalid files).
 * Origin: Gemini (loop over the five files and output headings); revised with
 *         Claude's help to accept file names, show error details, print all
 *         solutions, and report the solution total.
 * ============================================================================ */
int main(int argc, char* argv[]) {
    const char* defaultFiles[] = {"puzzle1.txt", "puzzle2.txt", "puzzle3.txt",
                                  "puzzle4.txt", "puzzle5.txt"}; // the five assigned puzzles
    const int defaultCount = 5;                           // how many default files there are
    int fileCount = (argc > 1) ? (argc - 1) : defaultCount; // use arguments if any were given
    int exitStatus = 0;                                   // becomes 1 if any file is malformed

    for (int i = 0; i < fileCount; ++i) {                 // handle one puzzle file per pass
        std::string filename = (argc > 1) ? argv[i + 1] : defaultFiles[i]; // this pass's file
        SudokuSolver solver;                              // a fresh solver for this puzzle

        std::cout << "========================================\n"; // heading border
        std::cout << "Puzzle File: " << filename << '\n'; // required: the file name
        std::cout << "========================================\n"; // heading border

        std::string error;                                // filled in if the file is malformed
        if (!solver.loadPuzzle(filename, error)) {        // read and check the file
            std::cout << "Invalid input: " << error << "\n\n"; // explain what is wrong
            exitStatus = 1;                               // remember that a file failed
            continue;                                     // move on to the next file
        }

        std::cout << "Initial Puzzle:\n";                 // label the original puzzle
        solver.printBoard(std::cout);                     // required: the puzzle from the file
        std::cout << "----------------------------------------\n"; // separator

        long long solutions = solver.solveAll(std::cout); // search; prints each solution found
        if (solutions == 0) {                             // nothing was found
            std::cout << "No solution found\n";           // required message
            if (!solver.givensAreValid()) {               // the givens themselves are invalid
                std::cout << "(Reason: " << solver.givensProblem() << ")\n"; // say which digit
            }
        }
        std::cout << "\nTotal solutions found: " << solutions << "\n\n"; // summary line
    }

    return exitStatus;                                    // 0 = every file was well formed
}

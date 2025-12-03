#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <fstream>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
using namespace std;

// ======================= Piece Base Class =======================
class Piece {
protected:
    char symbol;
    bool isWhite;
    bool hasMoved;
public:
    Piece(char s, bool white) : symbol(s), isWhite(white), hasMoved(false) {}
    virtual ~Piece() {}
    char getSymbol() const { return symbol; }
    bool getColor() const { return isWhite; }
    bool getHasMoved() const { return hasMoved; }
    void setMoved() { hasMoved = true; }
    void setHasMoved(bool v) { hasMoved = v; }
    virtual bool isValidMove(int sr, int sc, int er, int ec, Piece* board[8][8]) = 0;
};

// ======================= Pawn =======================
class Pawn : public Piece {
public:
    Pawn(bool white) : Piece(white ? 'P' : 'p', white) {}
    bool isValidMove(int sr, int sc, int er, int ec, Piece* board[8][8]) override {
        int dir = isWhite ? -1 : 1;
        // forward move
        if (sc == ec && board[er][ec] == nullptr) {
            if (er == sr + dir) return true;
            if ((isWhite && sr == 6) || (!isWhite && sr == 1)) {
                if (er == sr + 2 * dir && board[sr + dir][sc] == nullptr) return true;
            }
        }
        // diagonal capture
        if (abs(ec - sc) == 1 && er == sr + dir && board[er][ec] != nullptr) {
            return board[er][ec]->getColor() != isWhite;
        }
        return false;
    }
};

// ======================= Rook =======================
class Rook : public Piece {
public:
    Rook(bool white) : Piece(white ? 'R' : 'r', white) {}
    bool isValidMove(int sr, int sc, int er, int ec, Piece* board[8][8]) override {
        if (sr != er && sc != ec) return false;
        int dr = (er > sr) ? 1 : (er < sr ? -1 : 0);
        int dc = (ec > sc) ? 1 : (ec < sc ? -1 : 0);
        int r = sr + dr, c = sc + dc;
        while (r != er || c != ec) {
            if (board[r][c] != nullptr) return false;
            r += dr;c += dc;
        }
        return board[er][ec] == nullptr || board[er][ec]->getColor() != isWhite;
    }
};

// ======================= Knight =======================
class Knight : public Piece {
public:
    Knight(bool white) : Piece(white ? 'N' : 'n', white) {}
    bool isValidMove(int sr, int sc, int er, int ec, Piece* board[8][8]) override {
        int dr = abs(er - sr), dc = abs(ec - sc);
        if ((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) {
            return board[er][ec] == nullptr || board[er][ec]->getColor() != isWhite;
        }
        return false;
    }
};

// ======================= Bishop =======================
class Bishop : public Piece {
public:
    Bishop(bool white) : Piece(white ? 'B' : 'b', white) {}
    bool isValidMove(int sr, int sc, int er, int ec, Piece* board[8][8]) override {
        if (abs(er - sr) != abs(ec - sc)) return false;
        int dr = (er > sr) ? 1 : -1, dc = (ec > sc) ? 1 : -1;
        int r = sr + dr, c = sc + dc;
        while (r != er && c != ec) {
            if (board[r][c] != nullptr) return false;
            r += dr;c += dc;
        }
        return board[er][ec] == nullptr || board[er][ec]->getColor() != isWhite;
    }
};

// ======================= Queen =======================
class Queen : public Piece {
public:
    Queen(bool white) : Piece(white ? 'Q' : 'q', white) {}
    bool isValidMove(int sr, int sc, int er, int ec, Piece* board[8][8]) override {
        // rook-like
        if (sr == er || sc == ec) {
            int dr = (er > sr) ? 1 : (er < sr ? -1 : 0);
            int dc = (ec > sc) ? 1 : (ec < sc ? -1 : 0);
            int r = sr + dr, c = sc + dc;
            while (r != er || c != ec) {
                if (board[r][c] != nullptr) return false;
                r += dr;c += dc;
            }
            return board[er][ec] == nullptr || board[er][ec]->getColor() != isWhite;
        }
        // bishop-like
        if (abs(er - sr) == abs(ec - sc)) {
            int dr = (er > sr) ? 1 : -1, dc = (ec > sc) ? 1 : -1;
            int r = sr + dr, c = sc + dc;
            while (r != er && c != ec) {
                if (board[r][c] != nullptr) return false;
                r += dr;c += dc;
            }
            return board[er][ec] == nullptr || board[er][ec]->getColor() != isWhite;
        }
        return false;
    }
};

// ======================= King =======================
class King : public Piece {
public:
    King(bool white) : Piece(white ? 'K' : 'k', white) {}
    bool isValidMove(int sr, int sc, int er, int ec, Piece* board[8][8]) override {
        int dr = abs(er - sr), dc = abs(ec - sc);
        if (dr <= 1 && dc <= 1) {
            return board[er][ec] == nullptr || board[er][ec]->getColor() != isWhite;
        }
        // Castling basic path clearance (attack checks handled in Board::movePiece)
        if (dr == 0 && dc == 2 && !hasMoved) {
            int rookCol = (ec > sc) ? 7 : 0;
            Piece* rook = board[sr][rookCol];
            if (rook != nullptr && dynamic_cast<Rook*>(rook) != nullptr && !rook->getHasMoved()) {
                int step = (ec > sc) ? 1 : -1;
                for (int c = sc + step;c != rookCol;c += step) {
                    if (board[sr][c] != nullptr) return false;
                }
                return true;
            }
        }
        return false;
    }
};

// ======================= Board =======================
class Board {
private:
    Piece* squares[8][8];
    pair<int, int> lastMoveStart;
    pair<int, int> lastMoveEnd;

    // Simpler glyph() function: just return the piece letter
    string glyph(char ch) {
        return string(1, ch);
    }

public:
    Board() {
        for (int r = 0;r < 8;r++)for (int c = 0;c < 8;c++)squares[r][c] = nullptr;
        lastMoveStart = { -1,-1 };
        lastMoveEnd = { -1,-1 };
    }
    ~Board() {
        for (int r = 0;r < 8;r++)for (int c = 0;c < 8;c++)delete squares[r][c];
    }
    void setupBoard() {
        for (int c = 0;c < 8;c++) {
            squares[6][c] = new Pawn(true);
            squares[1][c] = new Pawn(false);
        }
        squares[7][0] = new Rook(true);squares[7][7] = new Rook(true);
        squares[0][0] = new Rook(false);squares[0][7] = new Rook(false);
        squares[7][1] = new Knight(true);squares[7][6] = new Knight(true);
        squares[0][1] = new Knight(false);squares[0][6] = new Knight(false);
        squares[7][2] = new Bishop(true);squares[7][5] = new Bishop(true);
        squares[0][2] = new Bishop(false);squares[0][5] = new Bishop(false);
        squares[7][3] = new Queen(true);squares[0][3] = new Queen(false);
        squares[7][4] = new King(true);squares[0][4] = new King(false);
    }

    void display() {
        cout << "\n";
        cout << "      a   b   c   d   e   f   g   h\n";
        cout << "    +---+---+---+---+---+---+---+---+\n";
        for (int r = 0;r < 8;r++) {
            cout << "  " << (8 - r) << " |";
            for (int c = 0;c < 8;c++) {
                if (squares[r][c] == nullptr) cout << "   |";
                else cout << " " << glyph(squares[r][c]->getSymbol()) << " |";
            }
            cout << " " << (8 - r) << "\n";
            cout << "    +---+---+---+---+---+---+---+---+\n";
        }
        cout << "      a   b   c   d   e   f   g   h\n\n";
    }

    Piece* getPiece(int r, int c) { return squares[r][c]; }

    pair<int, int> findKing(bool white) {
        for (int r = 0;r < 8;r++) {
            for (int c = 0;c < 8;c++) {
                if (squares[r][c] != nullptr && dynamic_cast<King*>(squares[r][c]) != nullptr) {
                    if (squares[r][c]->getColor() == white) return { r,c };
                }
            }
        }
        return { -1,-1 };
    }

    bool isSquareAttacked(int row, int col, bool byWhite) {
        for (int r = 0;r < 8;r++) {
            for (int c = 0;c < 8;c++) {
                Piece* p = squares[r][c];
                if (p != nullptr && p->getColor() == byWhite) {
                    if (p->isValidMove(r, c, row, col, squares)) return true;
                }
            }
        }
        return false;
    }

    bool isInCheck(bool white) {
        auto kingPos = findKing(white);
        return isSquareAttacked(kingPos.first, kingPos.second, !white);
    }

    bool hasLegalMoves(bool white) {
        for (int sr = 0;sr < 8;sr++) {
            for (int sc = 0;sc < 8;sc++) {
                Piece* p = squares[sr][sc];
                if (p != nullptr && p->getColor() == white) {
                    for (int er = 0;er < 8;er++) {
                        for (int ec = 0;ec < 8;ec++) {
                            if (tryMove(sr, sc, er, ec, white)) return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    // Try a move by simulating it (including en passant and castling), then undo.
    bool tryMove(int sr, int sc, int er, int ec, bool whiteTurn) {
        Piece* p = squares[sr][sc];
        if (!p || p->getColor() != whiteTurn) return false;

        // Handle en passant possibility in simulation
        bool enPassant = false;
        int ep_r = -1, ep_c = -1;
        if (dynamic_cast<Pawn*>(p) != nullptr && abs(er - sr) == 1 && abs(ec - sc) == 1 && squares[er][ec] == nullptr) {
            if (lastMoveStart.first != -1) {
                int target_r = sr; // the captured pawn is on same row as moving pawn started
                int target_c = ec;
                Piece* adj = squares[target_r][target_c];
                if (adj != nullptr && dynamic_cast<Pawn*>(adj) != nullptr) {
                    if (abs(lastMoveEnd.first - lastMoveStart.first) == 2 &&
                        lastMoveEnd.first == target_r && lastMoveEnd.second == target_c) {
                        enPassant = true;
                        ep_r = target_r; ep_c = target_c;
                    }
                }
            }
        }

        // Validate basic move legality (piece rules)
        if (!enPassant && !p->isValidMove(sr, sc, er, ec, squares)) return false;

        // Save state
        Piece* captured = squares[er][ec];
        Piece* epCaptured = nullptr;

        // Apply move
        if (enPassant) {
            epCaptured = squares[ep_r][ep_c];
            squares[ep_r][ep_c] = nullptr;
        }
        squares[er][ec] = p;
        squares[sr][sc] = nullptr;

        // Additional castling safety checks: king not in, through, or into check
        bool castle = dynamic_cast<King*>(p) != nullptr && abs(ec - sc) == 2;
        bool legal = true;
        if (castle) {
            int step = (ec > sc) ? 1 : -1;
            int midc = sc + step;
            int row = er;
            if (isInCheck(whiteTurn) || isSquareAttacked(row, midc, !whiteTurn) || isSquareAttacked(row, ec, !whiteTurn)) {
                legal = false;
            }
        }
        else {
            legal = !isInCheck(whiteTurn);
        }

        // Undo move
        squares[sr][sc] = p;
        squares[er][ec] = captured;
        if (enPassant && epCaptured) {
            squares[ep_r][ep_c] = epCaptured;
        }

        return legal;
    }

    bool movePiece(int sr, int sc, int er, int ec, bool whiteTurn) {
        Piece* p = squares[sr][sc];
        if (!p || p->getColor() != whiteTurn) return false;

        // En Passant actual capture
        bool didEnPassant = false;
        if (dynamic_cast<Pawn*>(p) != nullptr && abs(er - sr) == 1 && abs(ec - sc) == 1 && squares[er][ec] == nullptr) {
            if (lastMoveStart.first != -1) {
                int target_r = sr;
                int target_c = ec;
                Piece* adj = squares[target_r][target_c];
                if (adj != nullptr && dynamic_cast<Pawn*>(adj) != nullptr) {
                    if (abs(lastMoveEnd.first - lastMoveStart.first) == 2 &&
                        lastMoveEnd.first == target_r && lastMoveEnd.second == target_c) {
                        // Validate that move doesn't leave king in check
                        Piece* epCaptured = squares[target_r][target_c];
                        squares[target_r][target_c] = nullptr;
                        Piece* captured = squares[er][ec]; // should be null
                        squares[er][ec] = p;
                        squares[sr][sc] = nullptr;
                        bool legal = !isInCheck(whiteTurn);
                        // Undo
                        squares[sr][sc] = p;
                        squares[er][ec] = captured;
                        squares[target_r][target_c] = epCaptured;
                        if (!legal) return false;

                        // Perform en passant
                        delete squares[target_r][target_c];
                        squares[target_r][target_c] = nullptr;
                        squares[er][ec] = p;
                        squares[sr][sc] = nullptr;
                        p->setMoved();
                        lastMoveStart = { sr,sc };
                        lastMoveEnd = { er,ec };
                        didEnPassant = true;
                    }
                }
            }
            if (didEnPassant) return true;
        }

        // Validate piece rule
        if (!p->isValidMove(sr, sc, er, ec, squares)) return false;

        // Save captured
        Piece* captured = squares[er][ec];

        // Apply move
        squares[er][ec] = p;
        squares[sr][sc] = nullptr;

        // Castling path safety (not in, through, into check)
        if (dynamic_cast<King*>(p) != nullptr && abs(ec - sc) == 2) {
            int step = (ec > sc) ? 1 : -1;
            int midc = sc + step;
            int row = er;
            if (isInCheck(whiteTurn) || isSquareAttacked(row, midc, !whiteTurn) || isSquareAttacked(row, ec, !whiteTurn)) {
                // Undo
                squares[sr][sc] = p;
                squares[er][ec] = captured;
                return false;
            }
        }

        // Check legality
        bool legal = !isInCheck(whiteTurn);
        if (!legal) {
            squares[sr][sc] = p;
            squares[er][ec] = captured;
            return false;
        }

        // Finalize capture
        delete captured;
        p->setMoved();

        // Pawn promotion
        if (dynamic_cast<Pawn*>(p) != nullptr) {
            if ((p->getColor() && er == 0) || (!p->getColor() && er == 7)) {
                delete squares[er][ec];
                squares[er][ec] = new Queen(p->getColor());
            }
        }

        // Castling: move rook too
        if (dynamic_cast<King*>(p) != nullptr && abs(ec - sc) == 2) {
            int rookCol = (ec > sc) ? 7 : 0;
            int newRookCol = (ec > sc) ? ec - 1 : ec + 1;
            Piece* rook = squares[er][rookCol];
            squares[er][newRookCol] = rook;
            squares[er][rookCol] = nullptr;
            if (rook) rook->setMoved();
        }

        lastMoveStart = { sr,sc };
        lastMoveEnd = { er,ec };
        return true;
    }

    // Save board and history to file
    void saveGame(const string& filename, const vector<string>& history) {
        ofstream out(filename);
        if (!out) { cout << "Error saving file!\n"; return; }
        for (int r = 0;r < 8;r++) {
            for (int c = 0;c < 8;c++) {
                if (squares[r][c] == nullptr) out << ".";
                else out << squares[r][c]->getSymbol();
            }
            out << "\n";
        }
        out << "HISTORY\n";
        for (auto& m : history) out << m << "\n";
        out.close();
        cout << "Game saved to " << filename << "\n";
    }

    // Load board and history from file (marks pieces as moved to avoid castling ambiguity)
    void loadGame(const string& filename, vector<string>& history) {
        ifstream in(filename);
        if (!in) { cout << "Error loading file!\n"; return; }
        // Clear existing
        for (int r = 0;r < 8;r++)for (int c = 0;c < 8;c++) { delete squares[r][c]; squares[r][c] = nullptr; }

        // Read board
        for (int r = 0;r < 8;r++) {
            string line; getline(in, line);
            if (line.size() < 8) { cout << "Invalid file format.\n"; return; }
            for (int c = 0;c < 8;c++) {
                char ch = line[c];
                if (ch == '.') continue;
                bool white = isupper(ch);
                switch (toupper(ch)) {
                case 'P': squares[r][c] = new Pawn(white); break;
                case 'R': squares[r][c] = new Rook(white); break;
                case 'N': squares[r][c] = new Knight(white); break;
                case 'B': squares[r][c] = new Bishop(white); break;
                case 'Q': squares[r][c] = new Queen(white); break;
                case 'K': squares[r][c] = new King(white); break;
                }
                if (squares[r][c]) squares[r][c]->setHasMoved(true); // conservative choice
            }
        }
        string marker; getline(in, marker);
        history.clear();
        string move;
        while (getline(in, move)) {
            if (move.size() == 4) history.push_back(move);
        }
        // Reset lastMove tracking from history if possible
        if (!history.empty()) {
            string last = history.back();
            int sr = 8 - (last[1] - '0');
            int sc = last[0] - 'a';
            int er = 8 - (last[3] - '0');
            int ec = last[2] - 'a';
            lastMoveStart = { sr,sc };
            lastMoveEnd = { er,ec };
        }
        else {
            lastMoveStart = { -1,-1 };
            lastMoveEnd = { -1,-1 };
        }
        cout << "Game loaded from " << filename << "\n";
    }
};

// ======================= AI (Random Legal Move) =======================
struct Move { int sr, sc, er, ec; };

Move getRandomAIMove(Board& board, bool aiWhite) {
    vector<Move> moves;
    for (int sr = 0;sr < 8;sr++) {
        for (int sc = 0;sc < 8;sc++) {
            Piece* p = board.getPiece(sr, sc);
            if (p != nullptr && p->getColor() == aiWhite) {
                for (int er = 0;er < 8;er++) {
                    for (int ec = 0;ec < 8;ec++) {
                        if (board.tryMove(sr, sc, er, ec, aiWhite)) {
                            moves.push_back({ sr,sc,er,ec });
                        }
                    }
                }
            }
        }
    }
    if (moves.empty()) return { -1,-1,-1,-1 };
    size_t idx = rand() % moves.size();
    return moves[idx];
}

// ======================= Game =======================
class Game {
private:
    Board board;
    bool whiteTurn;
    vector<string> history;
    bool aiEnabled;
    bool aiIsWhite;

    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

public:
    Game() :whiteTurn(true), aiEnabled(false), aiIsWhite(false) {
        board.setupBoard();
        srand((unsigned)time(0));
    }

    void chooseMode() {
        cout << "Select mode:\n";
        cout << "1) Human vs Human\n";
        cout << "2) Human vs AI\n";
        cout << "Choice: ";
        int choice; cin >> choice;
        if (choice == 2) {
            aiEnabled = true;
            cout << "Should AI play as (w)hite or (b)lack? ";
            char side; cin >> side;
            aiIsWhite = (side == 'w' || side == 'W');
            cout << "AI set to " << (aiIsWhite ? "White" : "Black") << ".\n";
        }
        else {
            aiEnabled = false;
        }
    }

    string moveToString(int sr, int sc, int er, int ec) {
        string s;
        s.push_back(char('a' + sc));
        s.push_back(char('0' + (8 - sr)));
        s.push_back(char('a' + ec));
        s.push_back(char('0' + (8 - er)));
        return s;
    }

    void helpForSquare(const string& pos) {
        if (pos.size() != 2) { cout << "Usage: help e2\n"; return; }
        int sr = 8 - (pos[1] - '0');
        int sc = pos[0] - 'a';
        if (sr < 0 || sr>7 || sc < 0 || sc>7) { cout << "Invalid square.\n"; return; }
        Piece* p = board.getPiece(sr, sc);
        if (!p) { cout << "No piece at " << pos << ".\n"; return; }
        if (p->getColor() != whiteTurn) {
            cout << "It's " << (whiteTurn ? "White" : "Black") << "'s turn. Select your own piece.\n";
            return;
        }
        cout << "Possible moves for " << pos << ": ";
        bool any = false;
        for (int er = 0;er < 8;er++) {
            for (int ec = 0;ec < 8;ec++) {
                if (board.tryMove(sr, sc, er, ec, whiteTurn)) {
                    cout << char('a' + ec) << 8 - er << " ";
                    any = true;
                }
            }
        }
        if (!any) cout << "(none)";
        cout << "\n";
    }

    void play() {
        chooseMode();
        while (true) {
            // Refresh screen each turn
            clearScreen();
            board.display();

            // Check/checkmate/stalemate
            if (board.isInCheck(whiteTurn)) {
                cout << (whiteTurn ? "White" : "Black") << " is in check!\n";
            }
            if (!board.hasLegalMoves(whiteTurn)) {
                if (board.isInCheck(whiteTurn)) {
                    cout << "Checkmate! " << (whiteTurn ? "Black" : "White") << " wins!\n";
                }
                else {
                    cout << "Stalemate! It's a draw.\n";
                }
                break;
            }

            // AI turn
            if (aiEnabled && whiteTurn == aiIsWhite) {
                Move m = getRandomAIMove(board, aiIsWhite);
                if (m.sr == -1) {
                    cout << "AI has no legal moves.\n";
                    break;
                }
                board.movePiece(m.sr, m.sc, m.er, m.ec, aiIsWhite);
                history.push_back(moveToString(m.sr, m.sc, m.er, m.ec));
                cout << "AI played: " << history.back() << "\n";
                whiteTurn = !whiteTurn;
                continue;
            }

            // Human turn and commands
            cout << (whiteTurn ? "White" : "Black") << " to move.\n";
            cout << "Enter move (e.g. e2e4), or commands: help e2 | save filename | load filename | quit\n";
            string cmd; cin >> cmd;
            if (cmd == "quit") break;
            if (cmd == "save") {
                string fname; cin >> fname;
                board.saveGame(fname, history);
                continue;
            }
            if (cmd == "load") {
                string fname; cin >> fname;
                board.loadGame(fname, history);
                // After load, set turn based on history count parity
                whiteTurn = (history.size() % 2 == 0);
                continue;
            }
            if (cmd == "help") {
                string pos; cin >> pos;
                helpForSquare(pos);
                cout << "Press Enter to continue...";
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cin.get();
                continue;
            }

            // Otherwise treat as move
            string move = cmd;
            if (move.size() != 4) { cout << "Invalid input!\n"; continue; }
            int sr = 8 - (move[1] - '0');
            int sc = move[0] - 'a';
            int er = 8 - (move[3] - '0');
            int ec = move[2] - 'a';
            if (sr < 0 || sr>7 || sc < 0 || sc>7 || er < 0 || er>7 || ec < 0 || ec>7) {
                cout << "Out of bounds!\n";continue;
            }
            if (board.movePiece(sr, sc, er, ec, whiteTurn)) {
                history.push_back(move);
                whiteTurn = !whiteTurn;
            }
            else {
                cout << "Invalid move!\n";
                cout << "Press Enter to continue...";
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cin.get();
            }
        }

        // Print move history at end
        cout << "\nGame Over. Move history:\n";
        for (size_t i = 0;i < history.size();i++) {
            cout << i + 1 << ". " << history[i] << "\n";
        }
    }
};

// ======================= Main =======================
int main() {
    Game game;
    game.play();
    return 0;
}
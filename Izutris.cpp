#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <deque>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <cstring>

using namespace std;

struct ScoreEntry {
    string name;
    string mode;
    string detail; 
    string result; 

    string toString() const {
        char buf[128];
        snprintf(buf, sizeof(buf), "%-10s | %-8s | %-4s | %s", name.c_str(), mode.c_str(), detail.c_str(), result.c_str());
        return string(buf);
    }
};

struct Config {
    int das = 8, arr = 0, sdf = 40, dcd = 0;
    bool uniformSkin = false;
    string skin = "██", emptySkin = ". ", ghostSkin = "##"; 
    string kLeft = "LEFT", kRight = "RIGHT", kSoft = "DOWN", kHard = " ", kCW = "e", kCCW = "q", k180 = "w", kHold = "f";
    string kRestart = "r", kPause = "ESC", kMenu = "m"; 
    string keyColor = "96"; 
    bool showGhost = true;
    bool ghostColored = false;
    string flameColor = "31"; 
    string flashColor = "107";
    string clearColor = "107";
    string lightningColor = "97"; 
    string comboClr = "97"; 
    map<int, string> pieceSkins = {
        {1, "[]"}, {2, "[]"}, {3, "[]"}, {4, "[]"}, {5, "[]"}, {6, "[]"}, {7, "[]"}
    };

    void load(string filename) {
        ifstream file(filename);
        if (!file.is_open()) return;
        string line;
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t sep = line.find('=');
            if (sep == string::npos) continue;
            string key = line.substr(0, sep);
            string val = line.substr(sep + 1);
            if (key == "DAS") das = stoi(val);
            else if (key == "ARR") arr = stoi(val);
            else if (key == "SDF") sdf = stoi(val);
            else if (key == "DCD") dcd = stoi(val);
            else if (key == "UNIFORM") uniformSkin = (val == "ON");
            else if (key == "SKIN") skin = val;
            else if (key == "EMPTY") emptySkin = val;
            else if (key == "GHOST_SKIN") ghostSkin = val;
            else if (key == "GHOST") showGhost = (val == "ON");
            else if (key == "GHOST_CLR") ghostColored = (val == "ON");
            else if (key == "FLAME_CLR") flameColor = val;
            else if (key == "CLEAR_CLR") clearColor = val;
            else if (key == "FLASH_CLR") flashColor = val;
            else if (key == "KEY_CLR") keyColor = val;
            else if (key == "LIGHT_CLR") lightningColor = val;
            else if (key == "COMBO_CLR") comboClr = val;
            else if (key == "LEFT") kLeft = val;
            else if (key == "RIGHT") kRight = val;
            else if (key == "SOFT") kSoft = val;
            else if (key == "CW") kCW = val;
            else if (key == "CCW") kCCW = val;
            else if (key == "R180") k180 = val;
            else if (key == "HOLD") kHold = val;
            else if (key == "RESTART") kRestart = val;
            else if (key == "PAUSE") kPause = val;
            else if (key == "MENU") kMenu = val;
        }
    }

    void save(string filename) {
        ofstream file(filename);
        file << "DAS=" << das << "\n";
        file << "ARR=" << arr << "\n";
        file << "SDF=" << sdf << "\n";
        file << "DCD=" << dcd << "\n";
        file << "UNIFORM=" << (uniformSkin ? "ON" : "OFF") << "\n";
        file << "SKIN=" << skin << "\n";
        file << "EMPTY=" << emptySkin << "\n";
        file << "GHOST_SKIN=" << ghostSkin << "\n";
        file << "GHOST=" << (showGhost ? "ON" : "OFF") << "\n";
        file << "GHOST_CLR=" << (ghostColored ? "ON" : "OFF") << "\n";
        file << "FLAME_CLR=" << flameColor << "\n";
        file << "CLEAR_CLR=" << clearColor << "\n";
        file << "FLASH_CLR=" << flashColor << "\n";
        file << "KEY_CLR=" << keyColor << "\n";
        file << "LIGHT_CLR=" << lightningColor << "\n";
        file << "COMBO_CLR=" << comboClr << "\n";
        file << "LEFT=" << kLeft << "\n";
        file << "RIGHT=" << kRight << "\n";
        file << "SOFT=" << kSoft << "\n";
        file << "CW=" << kCW << "\n";
        file << "CCW=" << kCCW << "\n";
        file << "R180=" << k180 << "\n";
        file << "HOLD=" << kHold << "\n";
        file << "RESTART=" << kRestart << "\n";
        file << "PAUSE=" << kPause << "\n";
        file << "MENU=" << kMenu << "\n";
    }
};

struct Piece { 
    vector<pair<int, int>> cells; 
    int type; 
    int rotationState = 0; 
};

class SevenBag {
    vector<int> bag; mt19937 rng;
public:
    SevenBag() : rng(random_device{}()) {}
    void reset() { bag.clear(); }
    Piece getPiece(int type) {
        switch(type) {
            case 1: return {{{0,0}, {-1,0}, {1,0}, {0,-1}}, 1, 0}; 
            case 2: return {{{0,0}, {1,0}, {0,1}, {1,1}}, 2, 0};  
            case 3: return {{{-1,0}, {0,0}, {1,0}, {2,0}}, 3, 0}; 
            case 4: return {{{-1,0}, {0,0}, {0,-1}, {1,-1}}, 4, 0}; 
            case 5: return {{{-1,-1}, {0,-1}, {0,0}, {1,0}}, 5, 0}; 
            case 6: return {{{-1,-1}, {-1,0}, {0,0}, {1,0}}, 6, 0}; 
            default: return {{{-1,0}, {0,0}, {1,0}, {1,-1}}, 7, 0}; 
        }
    }
    int nextType() {
        if (bag.empty()) { bag = {1, 2, 3, 4, 5, 6, 7}; shuffle(bag.begin(), bag.end(), rng); }
        int t = bag.back(); bag.pop_back(); return t;
    }
};

enum GameState { MAIN_MENU, PLAY_MENU, OPTION_MENU, INGAME, HANDLING_MENU, VISUAL_MENU, CONTROL_MENU, SCOREBOARD, NAME_INPUT };
enum GameMode { ZEN, MARATHON, BLITZ };

class Tetris {
    int board[20][10];
    int curX, curY;
    Piece curPiece;
    deque<int> nextQueue;
    int holdPieceType;
    bool canHold;
    SevenBag bag;
    Config cfg;
    float gravity = 800.0f, fallTimer = 0, lockDelayLimit = 1000.0f, lockTimer = 0;
    long long score = 0;
    int combo = -1;
    int b2b = -1;
    string actionText = "";
    bool lastMoveWasRotate = false; 
    float activeTime = 0;
    float multiplier = 1.0f;
    float decayTimer = 0.0f;
    static constexpr float DECAY_INTERVAL = 5000.0f;
    static constexpr float DECAY_AMOUNT   = 0.5f;
    static constexpr float MULTIPLIER_MIN = 1.0f;
    bool isAnimating = false;
    vector<int> clearingLines;
    float animationTimer = 0;
    int waveFrame = 0;
    float whiteFlashTimer = 0;
    vector<pair<int, int>> lastLockedCells;
    bool useRainbowEffect = false;
    int flameHeights[10] = {0};
    mt19937 flameRng;
    float lightningTimer = 0;
    set<int> lightningCols;
    int lightningBottomY = 0;
    GameState state = MAIN_MENU;
    GameMode mode = ZEN;
    int menuCursor = 0;
    bool isTyping = false;
    string typingBuffer = "";
    map<string, float> keyStates;
    string lastDirKey = "";
    float dasTimer = 0, arrTimer = 0;
    int marathonLimit = 40, blitzTimeLimit = 120;
    float blitzTimer = 0;
    float flashLeftTimer = 0, flashRightTimer = 0;
    int linesClearedTotal = 0;
    vector<ScoreEntry> scores;
    // SRS Kick Data: key = oldState*10 + newState
    map<int, vector<pair<int, int>>> srsOffsets = {
        {01, {{0,0}, {-1,0}, {-1, 1}, {0,-2}, {-1,-2}}}, {10, {{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}}},
        {12, {{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}}},     {21, {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}}},
        {23, {{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}}},     {32, {{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}},
        {30, {{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}},   {03, {{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}}},
        // 180 Kicks (SRS-X / SRS+ style)
        {02, {{0,0}, {0,1}, {1,1}, {-1,1}, {1,0}, {-1,0}, {0,-1}}}, {20, {{0,0}, {0,-1}, {-1,-1}, {1,-1}, {-1,0}, {1,0}, {0,1}}},
        {13, {{0,0}, {1,0}, {1,2}, {1,1}, {0,2}, {0,1}, {1,-1}}},   {31, {{0,0}, {-1,0}, {-1,2}, {-1,1}, {0,2}, {0,1}, {-1,-1}}}
    };
 // I piece kicks
    map<int, vector<pair<int, int>>> srsOffsetsI = {
        {01, {{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}}},  {10, {{0,0}, {2,0}, {-1,0}, {2,1}, {-1,-2}}},
        {12, {{0,0}, {-1,0}, {2,0}, {-1,2}, {2,-1}}},  {21, {{0,0}, {1,0}, {-2,0}, {1,-2}, {-2,1}}},
        {23, {{0,0}, {2,0}, {-1,0}, {2,1}, {-1,-2}}},  {32, {{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}}},
        {30, {{0,0}, {1,0}, {-2,0}, {1,-2}, {-2,1}}},  {03, {{0,0}, {-1,0}, {2,0}, {-1,2}, {2,-1}}},
        // 180 Kicks I
        {02, {{0,1}, {0,2}, {-1,1}, {1,1}}}, {20, {{0,-1}, {0,-2}, {1,-1}, {-1,-1}}},
        {13, {{-1,0}, {-2,0}, {-1,1}, {-1,-1}}}, {31, {{1,0}, {2,0}, {1,-1}, {1,1}}}
    };

public:
    Tetris() : flameRng(random_device{}()) { 
        cfg.load("setting.cfg");
        loadScores();
        setNonBlocking(); 
        initGame(); 
    }

    void setNonBlocking() {
        struct termios ttystate; tcgetattr(STDIN_FILENO, &ttystate);
        ttystate.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    }

    void loadScores() {
        scores.clear();
        ifstream file("hiscore.sav");
        string line;
        while(getline(file, line)) {
            stringstream ss(line);
            string n, m, d, r;
            if (getline(ss, n, '|') && getline(ss, m, '|') && getline(ss, d, '|') && getline(ss, r, '|')) {
                auto trim = [](string s) {
                    size_t first = s.find_first_not_of(' ');
                    if (string::npos == first) return s;
                    size_t last = s.find_last_not_of(' ');
                    return s.substr(first, (last - first + 1));
                };
                scores.push_back({trim(n), trim(m), trim(d), trim(r)});
            }
        }
    }

    void saveScore(string name) {
        ScoreEntry e; e.name = name;
        if(mode == MARATHON) {
            e.mode = "Marathon"; e.detail = to_string(marathonLimit) + "L";
            int ts = (int)(activeTime / 1000);
            char buf[16]; sprintf(buf, "%d:%02d:%02d", ts/3600, (ts%3600)/60, ts%60);
            e.result = buf;
        } else {
            e.mode = "Blitz"; e.detail = to_string(blitzTimeLimit/60) + "m";
            e.result = to_string(score);
        }
        scores.insert(scores.begin(), e);
        if(scores.size() > 10) scores.pop_back();
        ofstream file("hiscore.sav");
        for(auto& s : scores) file << s.name << " | " << s.mode << " | " << s.detail << " | " << s.result << "\n";
    }

    void initGame() {
        for(int y=0; y<20; y++) for(int x=0; x<10; x++) board[y][x] = 0;
        nextQueue.clear(); bag.reset();
        for(int i=0; i<10; i++) nextQueue.push_back(bag.nextType());
        holdPieceType = 0; score = 0; combo = -1; b2b = -1; actionText = "";
        multiplier = 1.0f; decayTimer = 0.0f;
        isAnimating = false; isPaused = false; gameOver = false;
        linesClearedTotal = 0; lightningTimer = 0; activeTime = 0;
        blitzTimer = (float)blitzTimeLimit * 1000.0f;
        spawnPiece();
    }

    bool gameOver = false, isPaused = false;

    // --- Sửa lỗi T-Spin: Nhận diện theo số góc bị chặn ---
    int checkTSpin() {
        if (curPiece.type != 1 || !lastMoveWasRotate) return 0;
        int corners = 0;
        int dx[] = {-1, 1, -1, 1}, dy[] = {-1, -1, 1, 1};
        for (int i = 0; i < 4; i++) {
            int tx = curX + dx[i], ty = curY + dy[i];
            if (tx < 0 || tx >= 10 || ty >= 20 || (ty >= 0 && board[ty][tx] != 0)) corners++;
        }
        return (corners >= 3) ? 1 : 0;
    }

    void spawnPiece() {
        curPiece = bag.getPiece(nextQueue.front());
        nextQueue.pop_front();
        if (nextQueue.size() < 7) nextQueue.push_back(bag.nextType());
        curX = 4; curY = 1; canHold = true; lockTimer = 0;
        lastMoveWasRotate = false;
        if (collision(curX, curY, curPiece)) {
            gameOver = true;
            if(mode != ZEN) { state = NAME_INPUT; isTyping = true; typingBuffer = ""; }
        }
    }

    bool collision(int nx, int ny, Piece p) {
        for (auto& cell : p.cells) {
            int tx = nx + cell.first, ty = ny + cell.second;
            if (tx < 0 || tx >= 10 || ty >= 20 || (ty >= 0 && board[ty][tx] != 0)) return true;
        }
        return false;
    }

    void handleInput() {
        char c; string k = "";
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (isTyping) {
                if (c == '\n' || c == '\r') {
                    if(state == NAME_INPUT) { 
                        saveScore(typingBuffer == "" ? "GUEST" : typingBuffer); 
                        state = SCOREBOARD; 
                        menuCursor = (int)scores.size(); 
                    }
                    else finalizeTyping();
                    isTyping = false; return;
                }
                if (c == 127 || c == 8) { if(!typingBuffer.empty()) typingBuffer.pop_back(); return; }
                if (typingBuffer.length() < 10) typingBuffer += c; return;
            }
            if (c == 27) {
                char nC; if (read(STDIN_FILENO, &nC, 1) <= 0) k = "ESC";
                else if (nC == '[') {
                    char seq; if (read(STDIN_FILENO, &seq, 1) > 0) {
                        if (seq == 'A') k = "UP"; else if (seq == 'B') k = "DOWN";
                        else if (seq == 'C') k = "RIGHT"; else if (seq == 'D') k = "LEFT";
                    }
                }
            } else { k = string(1, c); if(c == ' ') k = " "; }
            
            keyStates[k] = 150.0f;
            if (k == "LEFT") flashLeftTimer = 100.0f;
            if (k == "RIGHT") flashRightTimer = 100.0f;

            if (state == MAIN_MENU) handleMainMenu(k);
            else if (state == PLAY_MENU) handlePlayMenu(k);
            else if (state == OPTION_MENU) handleOptionMenu(k);
            else if (state == HANDLING_MENU) handleHandlingMenu(k);
            else if (state == VISUAL_MENU) handleVisualMenu(k);
            else if (state == CONTROL_MENU) handleControlMenu(k);
            else if (state == SCOREBOARD) handleScoreboardMenu(k);
            else if (state == INGAME) handleIngameInput(k);
        } else { lastDirKey = ""; }
    }

    void finalizeTyping() {
        if (state == VISUAL_MENU) {
            if (menuCursor == 3) cfg.skin = typingBuffer;
            else if (menuCursor >= 4 && menuCursor <= 10) {
                int mapIdx[] = {1, 2, 3, 7, 6, 4, 5}; 
                cfg.pieceSkins[mapIdx[menuCursor-4]] = typingBuffer;
            }
            else if (menuCursor == 11) cfg.emptySkin = typingBuffer;
            else if (menuCursor == 12) cfg.ghostSkin = typingBuffer;
            else if (menuCursor == 13) cfg.clearColor = typingBuffer;
            else if (menuCursor == 14) cfg.flameColor = typingBuffer;
            else if (menuCursor == 15) cfg.flashColor = typingBuffer;
            else if (menuCursor == 16) cfg.keyColor = typingBuffer;
            else if (menuCursor == 17) cfg.lightningColor = typingBuffer;
            else if (menuCursor == 18) cfg.comboClr = typingBuffer;
        } else if (state == CONTROL_MENU) {
            string* targets[] = {&cfg.kLeft, &cfg.kRight, &cfg.k180, &cfg.kCCW, &cfg.kCW, &cfg.kHold, &cfg.kSoft, &cfg.kHard, &cfg.kPause, &cfg.kRestart, &cfg.kMenu};
            if (menuCursor <= 10) *targets[menuCursor] = typingBuffer;
        }
        cfg.save("setting.cfg");
    }

    void handleMainMenu(string k) {
        if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 4) % 4;
        else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 4;
        else if (k == " " || k == "\n" || k == "e") {
            if (menuCursor == 0) state = PLAY_MENU;
            else if (menuCursor == 1) { state = SCOREBOARD; menuCursor = (int)scores.size(); }
            else if (menuCursor == 2) state = OPTION_MENU;
            else if (menuCursor == 3) exit(0);
            if (state != SCOREBOARD) menuCursor = 0;
        }
    }

    void handleScoreboardMenu(string k) {
        int sz = (int)scores.size();
        if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + sz + 1) % (sz + 1);
        else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % (sz + 1);
        else if (k == " " || k == "\n" || k == "e") {
            if(menuCursor == sz) { state = MAIN_MENU; menuCursor = 0; }
        }
    }

    void handlePlayMenu(string k) {
        if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 4) % 4;
        else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 4;
        else if (k == "LEFT" || k == "a") {
            if (menuCursor == 1) marathonLimit = min(200, marathonLimit + 10); 
            if (menuCursor == 2) blitzTimeLimit = min(600, blitzTimeLimit + 30);
            flashLeftTimer = 100.0f;
        } else if (k == "RIGHT" || k == "d") {
            if (menuCursor == 1) marathonLimit = max(10, marathonLimit - 10);
            if (menuCursor == 2) blitzTimeLimit = max(60, blitzTimeLimit - 30);
            flashRightTimer = 100.0f;
        } else if (k == " " || k == "\n" || k == "e") {
            if (menuCursor == 0) { mode = ZEN; state = INGAME; initGame(); }
            else if (menuCursor == 1) { mode = MARATHON; state = INGAME; initGame(); }
            else if (menuCursor == 2) { mode = BLITZ; state = INGAME; initGame(); }
            else { state = MAIN_MENU; menuCursor = 0; }
        }
    }

    void handleOptionMenu(string k) {
        if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 4) % 4;
        else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 4;
        else if (k == " " || k == "\n" || k == "e") {
            if (menuCursor == 0) state = HANDLING_MENU;
            else if (menuCursor == 1) state = VISUAL_MENU;
            else if (menuCursor == 2) state = CONTROL_MENU;
            else { state = MAIN_MENU; cfg.save("setting.cfg"); }
            menuCursor = 0;
        }
    }

    void handleHandlingMenu(string k) {
        if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 5) % 5;
        else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 5;
        else if (k == "LEFT" || k == "a") {
            if(menuCursor == 0) cfg.arr++;
            else if(menuCursor == 1) cfg.das++;
            else if(menuCursor == 2) { if(cfg.sdf < 100) cfg.sdf++; } 
            else if(menuCursor == 3) cfg.dcd++;
            flashLeftTimer = 100.0f;
        } else if (k == "RIGHT" || k == "d") {
            if(menuCursor == 0) cfg.arr = max(0, cfg.arr - 1);
            else if(menuCursor == 1) cfg.das = max(0, cfg.das - 1);
            else if(menuCursor == 2) cfg.sdf = max(0, cfg.sdf - 1); 
            else if(menuCursor == 3) cfg.dcd = max(0, cfg.dcd - 1);
            flashRightTimer = 100.0f;
        } else if (k == " " || k == "\n" || k == "e") if(menuCursor == 4) { state = OPTION_MENU; cfg.save("setting.cfg"); menuCursor = 0; }
    }

    void handleVisualMenu(string k) {
        if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 20) % 20;
        else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 20;
        else if (k == "LEFT" || k == "a" || k == "RIGHT" || k == "d") {
            if (menuCursor == 0) cfg.uniformSkin = !cfg.uniformSkin;
            else if (menuCursor == 1) cfg.showGhost = !cfg.showGhost;
            else if (menuCursor == 2) cfg.ghostColored = !cfg.ghostColored;
            if(k == "LEFT" || k == "a") flashLeftTimer = 100.0f; else flashRightTimer = 100.0f;
        } else if (k == " " || k == "\n" || k == "e") {
            if (menuCursor >= 3 && menuCursor <= 18) { isTyping = true; typingBuffer = ""; }
            else if (menuCursor == 19) { state = OPTION_MENU; cfg.save("setting.cfg"); menuCursor = 0; }
        }
    }

    void handleControlMenu(string k) {
        if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 12) % 12;
        else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 12;
        else if (k == " " || k == "\n" || k == "e") {
            if (menuCursor <= 10) { isTyping = true; typingBuffer = ""; }
            else { state = OPTION_MENU; cfg.save("setting.cfg"); menuCursor = 0; }
        }
    }

    void handleIngameInput(string k) {
        if (k == cfg.kRestart) initGame();
        else if (k == cfg.kPause || k == "ESC") { if (!gameOver) isPaused = !isPaused; }
        else if (k == cfg.kMenu || k == "m") { state = MAIN_MENU; menuCursor = 0; }
        if (gameOver || isPaused) return;
        if (k == cfg.kLeft || k == cfg.kRight) { 
            if (k != lastDirKey) { move(k == cfg.kLeft ? -1 : 1); dasTimer = 0; arrTimer = 0; lastDirKey = k; } 
        }
        else if (k == cfg.kSoft) { moveDown(); lastMoveWasRotate = false; }
        else if (k == cfg.kCW) { rotate(1); dasTimer = max(0.0f, dasTimer - (float)cfg.dcd * 16.67f); }
        else if (k == cfg.kCCW) { rotate(2); dasTimer = max(0.0f, dasTimer - (float)cfg.dcd * 16.67f); }
        else if (k == cfg.k180) { rotate(3); dasTimer = max(0.0f, dasTimer - (float)cfg.dcd * 16.67f); }
        else if (k == cfg.kHold) hold();
        else if (k == cfg.kHard) { hardDrop(); lastMoveWasRotate = false; }
    }

    void move(int dx) { if (!collision(curX + dx, curY, curPiece)) { curX += dx; lastMoveWasRotate = false; } }
    void moveDown() { if (!collision(curX, curY + 1, curPiece)) { curY++; score += 1; resetDecay(); } }
    bool isOnGround() { return collision(curX, curY + 1, curPiece); }
    
    vector<pair<int,int>> getIShape(int state) {
        switch(state) {
            case 0: return {{-1, 0}, { 0, 0}, { 1, 0}, { 2, 0}}; 
            case 1: return {{ 1,-1}, { 1, 0}, { 1, 1}, { 1, 2}}; 
            case 2: return {{-1, 1}, { 0, 1}, { 1, 1}, { 2, 1}}; 
            default: return {{ 0,-1}, { 0, 0}, { 0, 1}, { 0, 2}}; 
        }
    }

    // --- Cập nhật hàm Rotate sử dụng SRS ---
    void rotate(int r_mode) {
        if (curPiece.type == 2) return; 

        int oldState = curPiece.rotationState;
        int newState;
        if (r_mode == 1)      newState = (oldState + 1) % 4; // CW
        else if (r_mode == 2) newState = (oldState + 3) % 4; // CCW
        else                  newState = (oldState + 2) % 4; // 180

        Piece nS = curPiece;
        nS.rotationState = newState;

        // Xoay tọa độ cells
        if (curPiece.type == 3) {
            nS.cells = getIShape(newState);
        } else {
            int times = (r_mode == 3) ? 2 : (r_mode == 1 ? 1 : 3);
            for(int t=0; t<times; t++) {
                for (auto& p : nS.cells) {
                    int x = p.first, y = p.second;
                    p.first = -y; p.second = x; 
                }
            }
        }

        // Kiểm tra Kick
        int lookupKey = oldState * 10 + newState;
        vector<pair<int, int>> kicks;
        if (curPiece.type == 3) kicks = srsOffsetsI[lookupKey];
        else kicks = srsOffsets[lookupKey];

        // Mặc định thử vị trí hiện tại (0,0) nếu bảng trống
        if (kicks.empty()) kicks.push_back({0, 0});

        for (auto& offset : kicks) {
            int dx = offset.first;
            int dy = -offset.second; // SRS dùng Y dương là lên, game dùng Y dương là xuống

            if (!collision(curX + dx, curY + dy, nS)) {
                curX += dx;
                curY += dy;
                curPiece = nS;
                lastMoveWasRotate = true;
                return;
            }
        }
    }

    void hold() {
        if (!canHold) return;
        if (holdPieceType == 0) { holdPieceType = curPiece.type; spawnPiece(); }
        else { int temp = curPiece.type; curPiece = bag.getPiece(holdPieceType); holdPieceType = temp; curX = 4; curY = 1; }
        canHold = false; lastMoveWasRotate = false;
    }

    void resetDecay() { decayTimer = 0.0f; }

    void hardDrop() { 
        int dropped = 0;
        while (!isOnGround()) { curY++; dropped++; }
        score += dropped * 10;
        resetDecay();
        lightningCols.clear();
        int maxY = 0;
        for (auto& p : curPiece.cells) { lightningCols.insert(curX + p.first); if (curY + p.second > maxY) maxY = curY + p.second; }
        lightningBottomY = maxY; lightningTimer = 120.0f; whiteFlashTimer = 200.0f;
        lock(); 
    }

    void lock() {
        int isTS = checkTSpin();
        actionText = ""; 
        lastLockedCells.clear();
        for (auto& p : curPiece.cells) {
            int tx = curX + p.first, ty = curY + p.second;
            if (ty >= 0 && ty < 20) { board[ty][tx] = curPiece.type; lastLockedCells.push_back({tx, ty}); }
        }
        clearingLines.clear();
        for (int y = 0; y < 20; y++) {
            bool full = true;
            for (int x = 0; x < 10; x++) if (board[y][x] == 0) full = false;
            if (full) clearingLines.push_back(y);
        }
        int lines = (int)clearingLines.size();
        useRainbowEffect = false;
        if (lines > 0 || isTS) {
            isAnimating = (lines > 0); animationTimer = 400.0f;
            bool isB2Bworthy = false;
            bool isPerfectClear = false;
            if (lines > 0) {
                set<int> clearSet(clearingLines.begin(), clearingLines.end());
                bool boardEmpty = true;
                for (int cy = 0; cy < 20 && boardEmpty; cy++) {
                    if (clearSet.count(cy)) continue;
                    for (int cx = 0; cx < 10 && boardEmpty; cx++)
                        if (board[cy][cx] != 0) boardEmpty = false;
                }
                isPerfectClear = boardEmpty;
            }
            long long baseScore = 0;
            if (isTS) {
                isB2Bworthy = (lines > 0);
                if (lines == 0)      { actionText = "T-SPIN MINI"; baseScore = 50; }
                else if (lines == 1) { actionText = "T-SPIN 1";    baseScore = 600;  useRainbowEffect = true; }
                else if (lines == 2) { actionText = "T-SPIN 2+";   baseScore = 1000; useRainbowEffect = true; }
                else if (lines == 3) { actionText = "T-SPIN 3+";   baseScore = 1250; useRainbowEffect = true; }
            } else {
                isB2Bworthy = (lines == 4);
                if      (lines == 1) { actionText = "SINGLE";  baseScore = 100; }
                else if (lines == 2) { actionText = "DOUBLE";  baseScore = 200; }
                else if (lines == 3) { actionText = "TRIPLE";  baseScore = 500; }
                else if (lines == 4) { actionText = "QUAD!!!"; baseScore = 1750; useRainbowEffect = true; }
            }
            if (isPerfectClear) { actionText = "CLEAR!!!"; baseScore = 1750; useRainbowEffect = true; }
            if (lines > 0) { combo++; linesClearedTotal += lines; } else combo = -1;
            if (isB2Bworthy) b2b++;
            else if (lines > 0) b2b = -1;
            if (combo >= 1) multiplier += 0.05f;
            if (b2b > 0)    multiplier += 0.05f;
            float totalMult = multiplier;
            if (totalMult < 1.0f) totalMult = 1.0f;
            score += (long long)(baseScore * totalMult);
            multiplier += 0.01f;
            resetDecay();
            if (!isAnimating) spawnPiece();
        } else { combo = -1; spawnPiece(); }
        
        if (mode == MARATHON && linesClearedTotal >= marathonLimit) {
            gameOver = true; state = NAME_INPUT; isTyping = true; typingBuffer = "";
        }
    }

    void update(float dt) {
        if (flashLeftTimer > 0) flashLeftTimer -= dt;
        if (flashRightTimer > 0) flashRightTimer -= dt;
        for (auto& kv : keyStates) if (kv.second > 0) kv.second -= dt;
        if (state == INGAME && !isPaused && !gameOver) activeTime += dt;
        if (state != INGAME || gameOver || isPaused) return;
        waveFrame++;
        if (lightningTimer > 0) lightningTimer -= dt;
        if (whiteFlashTimer > 0) whiteFlashTimer -= dt;
        decayTimer += dt;
        while (decayTimer >= DECAY_INTERVAL) {
            decayTimer -= DECAY_INTERVAL;
            multiplier -= DECAY_AMOUNT;
            if (multiplier < MULTIPLIER_MIN) multiplier = MULTIPLIER_MIN;
        }

        float dasMs = (float)cfg.das * 16.6667f;
        float arrMs = (float)cfg.arr * 16.6667f;

        if (lastDirKey != "") {
            dasTimer += dt;
            if (dasTimer >= dasMs) {
                arrTimer += dt;
                if (cfg.arr == 0) { 
                    while(!collision(curX + (lastDirKey == cfg.kLeft ? -1 : 1), curY, curPiece)) 
                        move(lastDirKey == cfg.kLeft ? -1 : 1); 
                }
                else while (arrTimer >= arrMs) { 
                    move(lastDirKey == cfg.kLeft ? -1 : 1); 
                    arrTimer -= arrMs; 
                }
            }
        }

        if (combo > 0) {
            for (int i = 0; i < 10; i++) {
                int target = 3 + (combo * 2); if (target > 18) target = 18;
                flameHeights[i] = (int)(flameRng() % 4) + target; 
            }
        } else for (int i = 0; i < 10; i++) if (flameHeights[i] > 0) flameHeights[i]--;

        if (mode == BLITZ) { 
            blitzTimer -= dt; 
            if (blitzTimer <= 0) { 
                blitzTimer = 0; gameOver = true; 
                state = NAME_INPUT; isTyping = true; typingBuffer = ""; 
            } 
        }

        if (isAnimating) {
            animationTimer -= dt;
            if (animationTimer <= 0) {
                for (int y : clearingLines) {
                    for (int ty = y; ty > 0; ty--) for (int tx = 0; tx < 10; tx++) board[ty][tx] = board[ty-1][tx];
                    for (int tx = 0; tx < 10; tx++) board[0][tx] = 0;
                }
                isAnimating = false; clearingLines.clear(); spawnPiece();
            }
            return; 
        }

        if (isOnGround()) { lockTimer += dt; if (lockTimer >= lockDelayLimit) lock(); }
        else { 
            lockTimer = 0; 
            fallTimer += dt; 
            float effectiveGravity = gravity;
            if (keyStates[cfg.kSoft] > 0) {
                if (cfg.sdf == 0) effectiveGravity = 0.001f; 
                else effectiveGravity = gravity / (float)cfg.sdf;
            }

            if (fallTimer >= effectiveGravity) { 
                curY++; fallTimer = 0; 
                lastMoveWasRotate = false; 
            } 
        }
    }

    string getWaveColor() {
        string colors[] = {"\x1B[91m", "\x1B[93m", "\x1B[92m", "\x1B[96m", "\x1B[94m", "\x1B[95m"};
        return colors[(waveFrame / 4) % 6];
    }

    string getStr(int type, bool ghost = false, bool isClearing = false, int x = -1, int y = -1) {
        if (whiteFlashTimer > 0) {
            for (auto& cell : lastLockedCells) if (cell.first == x && cell.second == y) return "\x1B[" + cfg.flashColor + "m██\x1B[0m";
            if (isClearing && !useRainbowEffect) return "\x1B[" + cfg.clearColor + "m██\x1B[0m";
        }
        if (isClearing && useRainbowEffect) return getWaveColor() + "██" + "\x1B[0m";
        if (type == 0) {
            if (lightningTimer > 0 && lightningCols.count(x) && y <= lightningBottomY) return "\x1B[" + cfg.lightningColor + "m██\x1B[0m";
            if (x >= 0 && y >= 0 && (19 - y) < flameHeights[x]) return "\x1B[" + cfg.flameColor + "m██\x1B[0m"; 
            return cfg.emptySkin;
        }
        string colors[] = {"", "\x1B[35m", "\x1B[33m", "\x1B[36m", "\x1B[31m", "\x1B[32m", "\x1B[34m", "\x1B[38;5;208m"};
        if (ghost) {
            string gColor = cfg.ghostColored ? colors[type] : "\x1B[38;5;232m";
            string gSkin = cfg.uniformSkin ? cfg.ghostSkin : cfg.pieceSkins[type];
            return gColor + gSkin + "\x1B[0m";
        }
        string s = cfg.uniformSkin ? cfg.skin : cfg.pieceSkins[type];
        if (combo > 0 && x >= 0 && y >= 0) {
            try {
                int bgColorCode = stoi(cfg.flameColor) + 10; 
                return "\x1B[" + to_string(bgColorCode) + "m\x1B[" + cfg.comboClr + "m" + s + "\x1B[0m"; 
            } catch (...) {
                return "\x1B[" + cfg.comboClr + "m" + s + "\x1B[0m"; 
            }
        }
        return colors[type] + s + "\x1B[0m";
    }

    void drawOverlay(string t, string l2, string c) {
        int cX = 21, cY = 10;
        printf("\033[%d;%dH\x1B[1;97;%sm┌────────────────────┐\x1B[0m", cY - 1, cX, c.c_str());
        printf("\033[%d;%dH\x1B[1;97;%sm│ %-18s │\x1B[0m", cY, cX, c.c_str(), t.c_str());
        printf("\033[%d;%dH\x1B[1;97;%sm│ %-18s │\x1B[0m", cY + 1, cX, c.c_str(), l2.c_str());
        printf("\033[%d;%dH\x1B[1;97;%sm│ %-18s │\x1B[0m", cY + 2, cX, c.c_str(), "Press M to return");
        printf("\033[%d;%dH\x1B[1;97;%sm└────────────────────┘\x1B[0m", cY + 3, cX, c.c_str());
    }

    void drawNameInput() {
        int cX = 21, cY = 10;
        string clr = "45"; 
        printf("\033[%d;%dH\x1B[1;97;%sm┌────────────────────┐\x1B[0m", cY - 1, cX, clr.c_str());
        printf("\033[%d;%dH\x1B[1;97;%sm│   NEW HIGH SCORE!  │\x1B[0m", cY, cX, clr.c_str());
        string inputRow = " Name: " + typingBuffer + "_";
        printf("\033[%d;%dH\x1B[1;97;%sm│ %-18s │\x1B[0m", cY + 1, cX, clr.c_str(), inputRow.c_str());
        printf("\033[%d;%dH\x1B[1;97;%sm│ [ENTER] to confirm │\x1B[0m", cY + 2, cX, clr.c_str());
        printf("\033[%d;%dH\x1B[1;97;%sm└────────────────────┘\x1B[0m", cY + 3, cX, clr.c_str());
    }

    void drawKeystrokes() {
        struct KsEntry { string label; string key; };
        KsEntry entries[] = {
            {"HOLD", cfg.kHold}, {"CCW", cfg.kCCW}, {"180", cfg.k180}, {"CW", cfg.kCW},
            {"LEFT", cfg.kLeft}, {"SOFT", cfg.kSoft}, {"RIGHT", cfg.kRight}, {"HARD", cfg.kHard}
        };
        cout << "Keystrokes: ";
        for (auto& e : entries) {
            bool active = (keyStates.count(e.key) && keyStates[e.key] > 0);
            if (active) cout << "\x1B[" + cfg.keyColor + "m[" << e.label << "]\x1B[0m ";
            else cout << "[" << e.label << "] ";
        }
    }

    string dVal(string v, int i) { return (isTyping && menuCursor == i) ? "\"" + typingBuffer + "_\"" : "\"" + v + "\""; }

    string bkt(string side, int idx) {
        bool flash = false;
        if (side == "<" && flashLeftTimer > 0 && menuCursor == idx) flash = true;
        if (side == ">" && flashRightTimer > 0 && menuCursor == idx) flash = true;
        if (flash) return "\x1B[1;93m" + side + "\x1B[0m";
        return side;
    }

    void printMenuLine(string label, string value, int index, int width = 26) {
        string displayStr = (value == "") ? label : label + ": " + value;
        int actualLen = 0;
        bool inEsc = false;
        for(size_t i=0; i < displayStr.length(); ++i) {
            if(displayStr[i] == '\x1B') inEsc = true;
            if(!inEsc) actualLen++;
            if(inEsc && displayStr[i] == 'm') inEsc = false;
        }
        printf("    │ ");
        if (menuCursor == index) {
            printf("\x1B[1;96m ▶ %s\x1B[0m", displayStr.c_str());
            for(int i=0; i < (width - 3 - actualLen); i++) printf(" ");
        } else {
            printf("   %s", displayStr.c_str());
            for(int i=0; i < (width - 3 - actualLen); i++) printf(" ");
        }
        printf(" │\n");
    }

    string getRating() {
        if (multiplier >= 2.0f) return "\x1B[91mX+      \x1B[0m"; 
        if (multiplier >= 1.8f) return "\x1B[38;5;208mS       \x1B[0m";
        if (multiplier >= 1.6f) return "\x1B[93mA       \x1B[0m"; 
        if (multiplier >= 1.4f) return "\x1B[92mB       \x1B[0m"; 
        if (multiplier >= 1.2f) return "\x1B[36mC       \x1B[0m"; 
        return "\x1B[96mD       \x1B[0m"; 
    }

    void draw() {
        printf("\033[H\033[J"); 
        if (state == MAIN_MENU) {
            cout << "\x1B[96m";
            cout << " ___   _______  __   __  _______  ______    ___   _______ \n";
            cout << "|   | |       ||  | |  ||       ||    _ |  |   | |       |\n";
            cout << "|   | |____   ||  | |  ||_     _||   | ||  |   | |  _____|\n";
            cout << "|   |  ____|  ||  | |  |  |   |  |   |_||_ |   | | |_____ \n";
            cout << "|   | | ______||  |_|  |  |   |  |    __  ||   | |_____  |\n";
            cout << "|   | | |_____ |       |  |   |  |   |  | ||   |  _____| |\n";
            cout << "|___| |_______||_______|  |___|  |___|  |_||___| |_______|\n";
            cout << "               by Ch1lly\n\n";
            cout << "\x1B[0m";
            cout << "    ┌────────────────────────────┐\n";
            printMenuLine("Play", "", 0, 26);
            printMenuLine("Scoreboard", "", 1, 26);
            printMenuLine("Option", "", 2, 26);
            cout << "    │                            │\n";
            printMenuLine("Exit", "", 3, 26);
            cout << "    └────────────────────────────┘\n";
            cout << "    W/S UP/DOWN - Navigate | Enter/Space - Select\n";
        } 
        else if (state == SCOREBOARD) {
            cout << "\n\n    ┌── Scoreboard ────────────────────────────────┐\n";
            for(int i=0; i<(int)scores.size(); i++) printMenuLine(scores[i].toString(), "", i, 44);
            cout << "    │                                              │\n";
            printMenuLine("Back", "", (int)scores.size(), 44);
            cout << "    └──────────────────────────────────────────────┘\n";
            cout << "    W/S UP/DOWN - Navigate | Enter/Space - Select | M - Main Menu\n";
        }
        else if (state == OPTION_MENU) {
            cout << "\n\n    ┌── Option ────────┐\n";
            printMenuLine("Handling", "", 0, 16);
            printMenuLine("Visual", "", 1, 16);
            printMenuLine("Control", "", 2, 16);
            cout << "    │                  │\n";
            printMenuLine("Back", "", 3, 16);
            cout << "    └──────────────────┘\n";
            cout << "    W/S UP/DOWN - Navigate | Enter/Space - Select | M - Main Menu\n";
        } else if (state == HANDLING_MENU) {
            string sdfValue = (cfg.sdf == 0) ? "INF" : to_string(cfg.sdf) + " x";
            cout << "\n\n    ┌── Handling ────────────────────────────┐\n";
            printMenuLine("ARR", bkt("<", 0) + " " + to_string(cfg.arr) + " f " + bkt(">", 0), 0, 38);
            printMenuLine("DAS", bkt("<", 1) + " " + to_string(cfg.das) + " f " + bkt(">", 1), 1, 38);
            printMenuLine("SDF", bkt("<", 2) + " " + sdfValue + " " + bkt(">", 2), 2, 38);
            printMenuLine("DCD", bkt("<", 3) + " " + to_string(cfg.dcd) + " f " + bkt(">", 3), 3, 38);
            cout << "    │                                        │\n";
            printMenuLine("Back", "", 4, 38);
            cout << "    └────────────────────────────────────────┘\n";
            cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
        } else if (state == VISUAL_MENU) {
            cout << "    ┌── Visual ─────────────────┐\n";
            printMenuLine("Uniform skin", bkt("<", 0) + (cfg.uniformSkin ? " On " : " Off ") + bkt(">", 0), 0, 25);
            printMenuLine("Show Ghost", bkt("<", 1) + (cfg.showGhost ? " On " : " Off ") + bkt(">", 1), 1, 25);
            printMenuLine("Ghost Color", bkt("<", 2) + (cfg.ghostColored ? " On " : " Off ") + bkt(">", 2), 2, 25);
            printMenuLine("General Skin", dVal(cfg.skin, 3), 3, 25);
            printMenuLine("T", dVal(cfg.pieceSkins[1], 4), 4, 25);
            printMenuLine("O", dVal(cfg.pieceSkins[2], 5), 5, 25);
            printMenuLine("I", dVal(cfg.pieceSkins[3], 6), 6, 25);
            printMenuLine("L", dVal(cfg.pieceSkins[7], 7), 7, 25);
            printMenuLine("J", dVal(cfg.pieceSkins[6], 8), 8, 25);
            printMenuLine("S", dVal(cfg.pieceSkins[4], 9), 9, 25);
            printMenuLine("Z", dVal(cfg.pieceSkins[5], 10), 10, 25);
            printMenuLine("Empty", dVal(cfg.emptySkin, 11), 11, 25);
            printMenuLine("Ghost Skin", dVal(cfg.ghostSkin, 12), 12, 25);
            printMenuLine("Clear Color", dVal(cfg.clearColor, 13), 13, 25);
            printMenuLine("Flame Color", dVal(cfg.flameColor, 14), 14, 25);
            printMenuLine("Flash Color", dVal(cfg.flashColor, 15), 15, 25);
            printMenuLine("Keystroke Color", dVal(cfg.keyColor, 16), 16, 25);
            printMenuLine("Lightning Color", dVal(cfg.lightningColor, 17), 17, 25);
            printMenuLine("Combo Color", dVal(cfg.comboClr, 18), 18, 25);
            printf("    │                           │\n");
            printMenuLine("Back", "", 19, 25);
            cout << "    └───────────────────────────┘\n";
            cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
        } else if (state == CONTROL_MENU) {
            cout << "    ┌── Control ──────────────────────┐\n";
            string* keys[] = {&cfg.kLeft, &cfg.kRight, &cfg.k180, &cfg.kCCW, &cfg.kCW, &cfg.kHold, &cfg.kSoft, &cfg.kHard, &cfg.kPause, &cfg.kRestart, &cfg.kMenu};
            string labels[] = {"Left", "Right", "180", "CCW", "CW", "Hold", "Soft drop", "Hard drop", "Pause", "Restart", "Menu"};
            for(int i=0; i<11; i++) printMenuLine(labels[i], dVal(*keys[i], i), i, 31);
            printf("    │                                 │\n");
            printMenuLine("Back", "", 11, 31);
            cout << "    └─────────────────────────────────┘\n";
            cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
        } else if (state == PLAY_MENU) {
            cout << "\n\n    ┌── Modes ─────────────────┐\n";
            printMenuLine("Zen", "", 0, 24);
            printMenuLine("Marathon", bkt("<", 1) + " " + to_string(marathonLimit) + " " + bkt(">", 1), 1, 24);
            printMenuLine("Blitz", bkt("<", 2) + " " + to_string(blitzTimeLimit / 60) + "m " + bkt(">", 2), 2, 24);
            cout << "    │                          │\n";
            printMenuLine("Back", "", 3, 24);
            cout << "    └──────────────────────────┘\n";
            cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
        } else if (state == INGAME || state == NAME_INPUT) {
            cout << "  "; drawKeystrokes(); cout << "\n";
            int gy = curY; 
            if (!isPaused && !gameOver && cfg.showGhost) 
                while (!collision(curX, gy + 1, curPiece)) gy++;
            
            int ts = (int)(activeTime / 1000);

            char statLine[16][32];
            memset(statLine, 0, sizeof(statLine));
            int sl = 0;
            if (mode == BLITZ) {
                int rem = (int)blitzTimer;
                snprintf(statLine[sl++], 32, "Time: %02d:%02d", rem/60000, (rem/1000)%60);
            } else {
                snprintf(statLine[sl++], 32, "Time: %02d:%02d", (ts%3600)/60, ts%60);
            }
            if (mode == MARATHON)
                snprintf(statLine[sl++], 32, "Lines:%d/%d", linesClearedTotal, marathonLimit);
            else if (mode == BLITZ)
                snprintf(statLine[sl++], 32, "Lines: %d", linesClearedTotal);
            snprintf(statLine[sl++], 32, "Score: %lld", score);
            string rating = getRating();
            int barLen = 16;
            float decayFrac = 1.0f - (decayTimer / DECAY_INTERVAL);
            if (decayFrac < 0) decayFrac = 0; if (decayFrac > 1) decayFrac = 1;
            int filled = (int)(decayFrac * barLen);
            string decayBar = "";
            for (int i = 0; i < filled; i++) decayBar += "█";
            for (int i = filled; i < barLen; i++) decayBar += " ";

            char multBuf[24]; snprintf(multBuf, sizeof(multBuf), "x%.2f Mult", multiplier);
            string comboLine = "";
            if (combo >= 1) comboLine = "+ Combo x" + to_string(combo + 1);

            string b2bLine = "";
            if (b2b > 0) b2bLine = "+ B2B x" + to_string(b2b);
            string actLine = actionText;
            if (!actLine.empty()) { actLine = "+ " + actLine; if(actLine.size() > 16) actLine = actLine.substr(0, 16); }

            cout << "  ┌ NEXT ────┐  ┌ BOARD ─────────────┐  ┌ RATING ──────────┐\n";
            for (int y = 0; y < 20; y++) {
                if (y < 15) {
                    int qi = y / 3;
                    int row = y % 3;
                    if (row < 2) {
                        Piece p = bag.getPiece(nextQueue[qi]);
                        int minX = 9, minY = 9;
                        for(auto& c : p.cells) { minX = min(minX, c.first); minY = min(minY, c.second); }
                        cout << "  │ ";
                        for(int x = 0; x < 4; x++) {
                            bool occ = false;
                            for(auto& c : p.cells)
                                if(c.first - minX == x && c.second - minY == row) { occ = true; break; }
                            cout << (occ ? getStr(nextQueue[qi]) : "  ");
                        }
                        cout << " │";
                    } else {
                        cout << "  │          │";
                    }
                } else if (y == 15) {
                    cout << "  ├── HOLD ──┤";
                } else if (y < 18) {
                    int hrow = y - 16;
                    cout << "  │ ";
                    if (holdPieceType) {
                        Piece p = bag.getPiece(holdPieceType);
                        int minX = 9, minY = 9;
                        for(auto& c : p.cells) { minX = min(minX, c.first); minY = min(minY, c.second); }
                        for(int x = 0; x < 4; x++){
                            bool occ = false;
                            for(auto& c : p.cells) if(c.first-minX==x && c.second-minY==hrow) { occ=true; break; }
                            cout << (occ ? getStr(holdPieceType) : "  ");
                        }
                    } else cout << "        ";
                    cout << " │";
                } else if (y == 18) {
                    cout << "  └──────────┘";
                } else {
                    cout << "              ";
                }
                cout << "  │";
                bool isRowClearing = false; for(int ly : clearingLines) if(ly == y) isRowClearing = true;
                for (int x = 0; x < 10; x++) {
                    if (isAnimating && isRowClearing) cout << getStr(0, false, true, x, y);
                    else {
                        int pT = 0; bool isG = false;
                        if(!isPaused && !gameOver) {
                            for(auto& p : curPiece.cells) { 
                                if(curX+p.first==x && curY+p.second==y) pT = curPiece.type; 
                                if(cfg.showGhost && curX+p.first==x && gy+p.second==y && pT==0) { pT = curPiece.type; isG = true; } 
                            }
                        }
                        cout << getStr(pT ? pT : board[y][x], isG, false, x, y);
                    }
                }
                cout << "│";

                {
                    int pr = y;
                    auto printRatingRow = [&](const string& content) {
                        int vlen = 0; bool inE = false;
                        for(char ch : content){ if(ch=='\x1B') inE=true; if(!inE) vlen++; if(inE&&ch=='m') inE=false; }
                        printf("  │ %-*s │", 16 + (int)(content.size() - vlen), content.c_str());
                    };
                    if      (pr == 0)  { printf("  │ Rating: %-8s │", rating.c_str()); }
                    else if (pr == 1)  { printRatingRow(""); }
                    else if (pr == 2)  { printRatingRow(comboLine); }
                    else if (pr == 3)  { printRatingRow(actLine); }
                    else if (pr == 4)  { printRatingRow(b2bLine); }
                    else if (pr == 5)  { printf("  │   ────────────── │"); }
                    else if (pr == 6)  { printRatingRow("Decay:"); }
                    else if (pr == 7)  { printRatingRow(decayBar); }
                    else if (pr == 8)  { printRatingRow(string(multBuf)); }
                    else if (pr == 9)  { printf("  └──────────────────┘"); }
                    else if (pr == 10) { printf("  %-18s", statLine[0]); }
                    else if (pr == 11) { printf("  %-18s", sl > 1 ? statLine[1] : ""); }
                    else if (pr == 12) { printf("  %-18s", sl > 2 ? statLine[2] : ""); }
                    else               { printf("  %-18s", ""); }
                }

                cout << "\n";
            }
            string bar = "                └"; 
            if (isOnGround() && !gameOver && !isPaused && !isAnimating) { 
                int p = (int)((1.0f-(lockTimer/lockDelayLimit))*20); 
                if(p<0) p=0; for(int i=0; i<20; i++) bar += (i<p?"─":" "); 
            }
            else bar += "────────────────────";
            cout << bar << "┘\n";

            cout << " \x1B[90mIzutris by Ch1lly~\x1B[0m\n";

            if (state == NAME_INPUT) drawNameInput();
            else if (gameOver) {
                char l1[32], l2[32]; string clr = "41";
                if (mode == MARATHON && linesClearedTotal >= marathonLimit) {
                    clr = "42"; int ts = (int)(activeTime / 1000);
                    sprintf(l1, "  MARATHON WIN! "); sprintf(l2, " TIME: %02d:%02d:%02d ", ts / 3600, (ts % 3600) / 60, ts % 60);
                } else if (mode == BLITZ) { sprintf(l1, " BLITZ FINISHED "); sprintf(l2, " SCORE: %-10lld", score); }
                else { sprintf(l1, "   GAME OVER!   "); sprintf(l2, "Press R to Retry"); }
                drawOverlay(l1, l2, clr);
            } else if (isPaused) drawOverlay("     PAUSED     ", "ESC to Resume", "44");
        }
        cout << flush;
    }
};

int main() {
    printf("\033[2J\033[H\033[?25l"); 
    Tetris game; 
    auto last = chrono::steady_clock::now(); 
    while (true) {
        auto now = chrono::steady_clock::now();
        float delta = chrono::duration<float, milli>(now - last).count();
        last = now; 
        game.handleInput(); 
        game.update(delta); 
        game.draw();
        this_thread::sleep_for(chrono::milliseconds(16));
    }
    return 0;
}
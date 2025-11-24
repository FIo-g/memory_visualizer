// C++ Memory Visualizer - User Final Edition

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <limits>
#include <cstdlib>

using namespace std;

// ==================== 메모리 블록 정의 ====================

enum class MemoryType {
    STACK,
    HEAP
};

enum class PointerType {
    RAW
};

class MemoryBlock {
public:
    int id;
    string name;
    size_t size;
    MemoryType type;
    void* address;
    bool isAllocated;
    int lifetime;

    bool isPointer;
    PointerType pointerType;
    int pointsTo;

    float x, y;
    float targetX, targetY;
    bool isHighlighted;

    MemoryBlock()
        : id(-1), name(""), size(0), type(MemoryType::STACK),
        address(nullptr), isAllocated(false), lifetime(0),
        isPointer(false), pointerType(PointerType::RAW),
        pointsTo(-1),
        x(0), y(0), targetX(0), targetY(0), isHighlighted(false) {
    }
};

class MemoryEvent {
public:
    enum class EventType {
        ALLOCATE,
        DEALLOCATE,
        ASSIGN,
        LEAK
    };

    EventType type;
    int blockId;
    string description;
    float timestamp;

    MemoryEvent(EventType t, int id, const string& desc, float time)
        : type(t), blockId(id), description(desc), timestamp(time) {
    }
};

// ==================== 메모리 관리자 ====================

class MemoryManager {
private:
    vector<MemoryBlock> blocks;
    vector<MemoryEvent> events;
    int nextId;
    int stackDepth;
    float currentTime;

    void addEvent(MemoryEvent::EventType type, int blockId, const string& description) {
        events.push_back(MemoryEvent(type, blockId, description, currentTime));
        currentTime += 1.0f;
    }

public:
    // 메모리 관리자 초기화
    MemoryManager() : nextId(1), stackDepth(0), currentTime(0.0f) {}

    // 스택 변수 생성 (지역 변수)
    int createStackVariable(const string& name, size_t size) {
        MemoryBlock block;
        block.id = nextId++;
        block.name = name;
        block.size = size;
        block.type = MemoryType::STACK;
        block.address = (void*)(0x7fff0000 + blocks.size() * 8);
        block.isAllocated = true;
        block.isPointer = false;
        blocks.push_back(block);
        stackDepth++;

        addEvent(MemoryEvent::EventType::ALLOCATE, block.id,
            "스택 변수 생성: " + name);

        return block.id;
    }

    // 힙 메모리 할당 (동적 메모리)
    int allocateHeap(const string& name, size_t size, PointerType ptrType = PointerType::RAW) {
        MemoryBlock block;
        block.id = nextId++;
        block.name = name;
        block.size = size;
        block.type = MemoryType::HEAP;
        block.address = (void*)(0x10000000 + blocks.size() * 16);
        block.isAllocated = true;
        block.isPointer = false;
        blocks.push_back(block);

        addEvent(MemoryEvent::EventType::ALLOCATE, block.id,
            "힙 메모리 할당: " + name);

        return block.id;
    }

    // 메모리 해제 (delete 수행)
    bool deallocate(int blockId) {
        for (auto& block : blocks) {
            if (block.id == blockId && block.isAllocated) {
                block.isAllocated = false;

                for (auto& b : blocks) {
                    if (b.isPointer && b.pointsTo == blockId) {
                        b.pointsTo = -1;
                    }
                }

                addEvent(MemoryEvent::EventType::DEALLOCATE, blockId,
                    "메모리 해제: " + block.name);

                return true;
            }
        }
        return false;
    }

    // 포인터 변수에 주소 할당 (ptr = &var 또는 ptr = ptr2)
    bool assignPointer(int pointerBlockId, int targetBlockId) {
        for (auto& block : blocks) {
            if (block.id == pointerBlockId) {
                block.pointsTo = targetBlockId;

                string targetName = (targetBlockId == -1) ? "nullptr" : findBlock(targetBlockId)->name;

                addEvent(MemoryEvent::EventType::ASSIGN, pointerBlockId,
                    "포인터 연결: " + block.name + " -> " + targetName);

                return true;
            }
        }
        return false;
    }

    // 프로그램 종료 시 모든 스택 메모리 정리
    void clearAllStack() {
        for (auto& block : blocks) {
            if (block.type == MemoryType::STACK && block.isAllocated) {
                block.isAllocated = false;
                addEvent(MemoryEvent::EventType::DEALLOCATE, block.id,
                    "프로그램 종료로 변수 해제: " + block.name);
            }
        }
        stackDepth = 0;
    }

    // 메모리 누수 감지 (해제되지 않은 힙 메모리 찾기)
    vector<int> detectLeaks() const {
        vector<int> leaks;
        for (const auto& block : blocks) {
            if (block.type == MemoryType::HEAP && block.isAllocated) {
                bool hasPointer = false;
                for (const auto& ptr : blocks) {
                    if (ptr.isPointer && ptr.isAllocated && ptr.pointsTo == block.id) {
                        hasPointer = true;
                        break;
                    }
                }
                if (!hasPointer) {
                    leaks.push_back(block.id);
                }
            }
        }
        return leaks;
    }

    // ID로 메모리 블록 찾기
    MemoryBlock* findBlock(int id) {
        for (auto& block : blocks) {
            if (block.id == id) return &block;
        }
        return nullptr;
    }

    const MemoryBlock* findBlock(int id) const {
        for (const auto& block : blocks) {
            if (block.id == id) return &block;
        }
        return nullptr;
    }

    const vector<MemoryBlock>& getMemoryBlocks() const { return blocks; }
    const vector<MemoryEvent>& getEvents() const { return events; }

    // 메모리 관리자 초기화 (모든 데이터 삭제)
    void reset() {
        blocks.clear();
        events.clear();
        nextId = 1;
        stackDepth = 0;
        currentTime = 0.0f;
    }
};

// ==================== 화면 출력 ====================

class Visualizer {
private:
    string colorReset = "\033[0m";
    string colorRed = "\033[31m";
    string colorGreen = "\033[32m";
    string colorYellow = "\033[33m";
    string colorBlue = "\033[34m";
    string colorMagenta = "\033[35m";
    string colorCyan = "\033[36m";
    string colorBold = "\033[1m";

    void printSeparator(char ch, int width) const {
        for (int i = 0; i < width; i++) {
            cout << ch;
        }
        cout << endl;
    }

    // 스택 메모리 영역 출력
    void printStack(const vector<MemoryBlock>& blocks) const {
        bool hasStack = false;
        for (const auto& block : blocks) {
            if (block.type == MemoryType::STACK && block.isAllocated) {
                hasStack = true;
                cout << "│ " << colorBlue;
                cout << block.name;
                for (size_t i = block.name.length(); i < 15; i++) cout << " ";

                if (block.isPointer) {
                    cout << " [ptr]          ";
                }
                else {
                    cout << " [val]          ";
                }

                cout << block.size << "bytes";
                cout << colorReset << endl;
            }
        }

        if (!hasStack) {
            cout << "│ (비어있음)" << endl;
        }
    }

    // 힙 메모리 영역 출력
    void printHeap(const vector<MemoryBlock>& blocks) const {
        bool hasHeap = false;
        for (const auto& block : blocks) {
            if (block.type == MemoryType::HEAP && block.isAllocated) {
                hasHeap = true;
                cout << "│ " << colorRed;
                cout << block.name;
                for (size_t i = block.name.length(); i < 30; i++) cout << " ";
                cout << block.size << "bytes";
                cout << colorReset << endl;
            }
        }

        if (!hasHeap) {
            cout << "│ (비어있음)" << endl;
        }
    }

    // 포인터 연결 관계 출력 (ptr -> data)
    void printPointerConnections(const vector<MemoryBlock>& blocks) const {
        bool hasConnections = false;
        for (const auto& block : blocks) {
            if (block.isPointer && block.isAllocated && block.pointsTo != -1) {
                hasConnections = true;
                const MemoryBlock* target = nullptr;
                for (const auto& b : blocks) {
                    if (b.id == block.pointsTo) {
                        target = &b;
                        break;
                    }
                }

                cout << "  " << colorYellow << block.name << colorReset;
                cout << " ──> ";

                if (target && target->isAllocated) {
                    if (target->type == MemoryType::HEAP)
                        cout << colorRed << target->name << colorReset;
                    else
                        cout << colorBlue << target->name << " (Stack)" << colorReset;
                }
                else {
                    cout << colorRed << "(dangling)" << colorReset;
                }
                cout << endl;
            }
        }

        if (!hasConnections) {
            cout << "  (포인터 연결 없음)" << endl;
        }
    }

    // 이벤트 로그 출력 (최근 count개)
    void printEventLog(const vector<MemoryEvent>& events, int count) const {
        int startIdx = (int)events.size() - count;
        if (startIdx < 0) startIdx = 0;

        for (size_t i = startIdx; i < events.size(); i++) {
            const auto& event = events[i];

            switch (event.type) {
            case MemoryEvent::EventType::ALLOCATE:
                cout << "  " << colorGreen << "[ALLOC]  " << colorReset;
                break;
            case MemoryEvent::EventType::DEALLOCATE:
                cout << "  " << colorRed << "[FREE]   " << colorReset;
                break;
            case MemoryEvent::EventType::ASSIGN:
                cout << "  " << colorYellow << "[ASSIGN] " << colorReset;
                break;
            case MemoryEvent::EventType::LEAK:
                cout << "  " << colorRed << "[LEAK]   " << colorReset;
                break;
            }

            cout << event.description << endl;
        }
    }

public:
    // 화면 지우기
    void clearScreen() const {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

    // 메모리 누수 경고 출력
    void printLeakWarnings(const vector<int>& leaks, const MemoryManager& memManager) const {
        if (leaks.empty()) return;

        cout << colorBold << colorRed;
        cout << "⚠ 메모리 누수 감지! " << leaks.size() << "개 블록" << colorReset << endl;

        for (int id : leaks) {
            const MemoryBlock* block = memManager.findBlock(id);
            if (block) {
                cout << "  - " << colorRed << block->name << " (" << block->size << " bytes, @"
                    << block->address << ")" << colorReset << endl;
            }
        }
    }

    // 전체 메모리 상태 출력 (최종 결과 화면)
    void printMemoryState(const MemoryManager& memManager) const {
        clearScreen();

        cout << colorBold << colorCyan;
        printSeparator('=', 70);
        cout << "        C++ 메모리 관리 시각화 도구 - 콘솔 버전" << endl;
        printSeparator('=', 70);
        cout << colorReset << endl;

        const auto& blocks = memManager.getMemoryBlocks();

        auto leaks = memManager.detectLeaks();
        if (!leaks.empty()) {
            printLeakWarnings(leaks, memManager);
            cout << endl;
        }

        cout << colorBold << colorBlue << "┌─ STACK 메모리 ─────────────────┐" << colorReset << endl;
        printStack(blocks);
        cout << colorBlue << "└─────────────────────────────────┘" << colorReset << endl;
        cout << endl;

        cout << colorBold << colorRed << "┌─ HEAP 메모리 ──────────────────┐" << colorReset << endl;
        printHeap(blocks);
        cout << colorRed << "└─────────────────────────────────┘" << colorReset << endl;
        cout << endl;

        cout << colorBold << colorYellow << "포인터 연결:" << colorReset << endl;
        printPointerConnections(blocks);
        cout << endl;

        cout << colorBold << colorGreen << "최근 이벤트:" << colorReset << endl;
        printEventLog(memManager.getEvents(), 15);
        cout << endl;

        printSeparator('-', 70);
    }

    // 메모리 상태 출력 (단계별 실행 화면 - 현재 실행 라인 표시)
    void printMemoryStateWithLine(const MemoryManager& memManager,
        const string& currentLine,
        int lineNumber) const {
        clearScreen();

        cout << colorBold << colorCyan;
        printSeparator('=', 70);
        cout << "        C++ 메모리 관리 시각화 도구 - 단계별 실행" << endl;
        printSeparator('=', 70);
        cout << colorReset << endl;

        cout << colorBold << colorMagenta << "▶ 현재 실행 라인 " << lineNumber << ": " << colorReset;
        cout << colorYellow << currentLine << colorReset << endl;
        cout << endl;

        const auto& blocks = memManager.getMemoryBlocks();

        auto leaks = memManager.detectLeaks();
        if (!leaks.empty()) {
            printLeakWarnings(leaks, memManager);
            cout << endl;
        }

        cout << colorBold << colorBlue << "┌─ STACK 메모리 ─────────────────┐" << colorReset << endl;
        printStack(blocks);
        cout << colorBlue << "└────────────────────────────────┘" << colorReset << endl;
        cout << endl;

        cout << colorBold << colorRed << "┌─ HEAP 메모리 ──────────────────┐" << colorReset << endl;
        printHeap(blocks);
        cout << colorRed << "└────────────────────────────────┘" << colorReset << endl;
        cout << endl;

        cout << colorBold << colorYellow << "포인터 연결:" << colorReset << endl;
        printPointerConnections(blocks);
        cout << endl;

        cout << colorBold << colorGreen << "최근 이벤트:" << colorReset << endl;
        printEventLog(memManager.getEvents(), 15);
        cout << endl;

        printSeparator('-', 70);
        cout << colorGreen << "▶ Enter를 누르면 다음 단계로 진행합니다..." << colorReset << endl;
    }
};

// ==================== 코드 파서 ==================== 

class ScriptParser {
private:
    MemoryManager& memManager;
    unordered_map<string, int> variables;
    int scopeLevel;

    vector<string> splitIntoLines(const string& code) {
        vector<string> lines;
        istringstream stream(code);
        string line;
        while (getline(stream, line)) {
            lines.push_back(line);
        }
        return lines;
    }

    string removeComments(const string& line) {
        size_t pos = line.find("//");
        if (pos != string::npos) {
            return line.substr(0, pos);
        }
        return line;
    }

    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    vector<string> tokenize(const string& line) {
        vector<string> tokens;
        string current;
        for (char ch : line) {
            if (isspace(ch) || ch == ';' || ch == '(' || ch == ')' || ch == '{' || ch == '}') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            else {
                current += ch;
            }
        }
        if (!current.empty()) {
            tokens.push_back(current);
        }
        return tokens;
    }

    bool isBasicType(const string& type) {
        string t = type;
        t.erase(remove(t.begin(), t.end(), '*'), t.end());
        return t == "int" || t == "float" || t == "double" ||
            t == "char" || t == "bool" || t == "long" || t == "short";
    }

    size_t getTypeSize(const string& type) {
        if (type.find("int") != string::npos) return 4;
        if (type.find("double") != string::npos) return 8;
        if (type.find("float") != string::npos) return 4;
        if (type.find("char") != string::npos) return 1;
        if (type.find("long") != string::npos) return 8;
        if (type.find("short") != string::npos) return 2;
        if (type.find("bool") != string::npos) return 1;
        return 4;
    }

    // 변수 선언 파싱 (int* ptr; 등)
    bool parseDeclaration(const string& line) {
        auto tokens = tokenize(line);
        if (tokens.size() < 2) return false;

        string type = tokens[0];
        string name = tokens[1];

        if (!name.empty() && name.back() == ';') {
            name.pop_back();
        }

        while (!name.empty() && name[0] == '*') {
            type += "*";
            name = name.substr(1);
        }

        bool isPtr = (type.find('*') != string::npos);

        size_t size = isPtr ? sizeof(void*) : getTypeSize(type);

        if (isPtr) {
            int id = memManager.createStackVariable(name, size);
            MemoryBlock* block = memManager.findBlock(id);
            if (block) {
                block->isPointer = true;
                block->pointerType = PointerType::RAW;
                block->pointsTo = -1;
            }
            variables[name] = id;
        }
        else {
            int id = memManager.createStackVariable(name, size);
            variables[name] = id;
        }

        return true;
    }

    // new 연산자 파싱 (ptr = new int;)
    bool parseNew(const string& line) {
        size_t equalPos = line.find('=');
        if (equalPos == string::npos) return false;

        string varPart = trim(line.substr(0, equalPos));
        string valuePart = trim(line.substr(equalPos + 1));

        auto tokens = tokenize(varPart);
        string varName;

        for (auto it = tokens.rbegin(); it != tokens.rend(); ++it) {
            string token = *it;
            token.erase(remove(token.begin(), token.end(), '*'), token.end());
            if (!token.empty() && !isBasicType(token)) {
                varName = *it;
                varName.erase(remove(varName.begin(), varName.end(), '*'), varName.end());
                break;
            }
        }

        if (varName.empty() && !tokens.empty()) {
            varName = tokens.back();
            varName.erase(remove(varName.begin(), varName.end(), '*'), varName.end());
        }

        size_t newPos = valuePart.find("new ");
        if (newPos == string::npos) return false;

        string typeStr = trim(valuePart.substr(newPos + 4));
        if (typeStr.find('(') != string::npos) typeStr = typeStr.substr(0, typeStr.find('('));
        if (typeStr.find(';') != string::npos) typeStr = typeStr.substr(0, typeStr.find(';'));
        typeStr = trim(typeStr);

        string heapName = varName + "_data";
        size_t size = getTypeSize(typeStr);
        int heapId = memManager.allocateHeap(heapName, size, PointerType::RAW);

        auto it = variables.find(varName);
        if (it == variables.end()) return false;

        memManager.assignPointer(it->second, heapId);
        return true;
    }

    // delete 연산자 파싱 (delete ptr;)
    bool parseDelete(const string& line) {
        string varName = trim(line.substr(7));
        if (!varName.empty() && varName.back() == ';') varName.pop_back();

        auto it = variables.find(varName);
        if (it == variables.end()) return false;

        MemoryBlock* ptrBlock = memManager.findBlock(it->second);
        if (!ptrBlock || !ptrBlock->isPointer) return false;

        if (ptrBlock->pointsTo != -1) {
            memManager.deallocate(ptrBlock->pointsTo);
            ptrBlock->pointsTo = -1;
        }
        return true;
    }

    // 할당 연산 파싱 (ptr = &var; 또는 ptr = nullptr;)
    bool parseAssignment(const string& line) {
        size_t equalPos = line.find('=');
        if (equalPos == string::npos) return true;

        string leftSide = trim(line.substr(0, equalPos));
        string rightSide = trim(line.substr(equalPos + 1));

        if (!rightSide.empty() && rightSide.back() == ';') rightSide.pop_back();

        auto leftTokens = tokenize(leftSide);
        string leftVarName = leftTokens.back();
        leftVarName.erase(remove(leftVarName.begin(), leftVarName.end(), '*'), leftVarName.end());

        auto leftIt = variables.find(leftVarName);
        if (leftIt == variables.end()) return true;

        if (rightSide == "nullptr" || rightSide == "NULL") {
            memManager.assignPointer(leftIt->second, -1);
            return true;
        }

        if (rightSide.size() > 1 && rightSide[0] == '&') {
            string targetName = trim(rightSide.substr(1));
            targetName.erase(remove(targetName.begin(), targetName.end(), ';'), targetName.end());

            auto targetIt = variables.find(targetName);
            if (targetIt != variables.end()) {
                memManager.assignPointer(leftIt->second, targetIt->second);
                return true;
            }
        }

        string rightVarName = rightSide;
        rightVarName.erase(remove(rightVarName.begin(), rightVarName.end(), ';'), rightVarName.end());

        auto rightIt = variables.find(rightVarName);
        if (rightIt != variables.end()) {
            MemoryBlock* rightBlock = memManager.findBlock(rightIt->second);
            if (rightBlock && rightBlock->isPointer) {
                memManager.assignPointer(leftIt->second, rightBlock->pointsTo);
            }
            return true;
        }

        return true;
    }

    // 한 줄의 코드 실행
    bool executeLine(const string& line) {
        string trimmedLine = trim(line);

        if (trimmedLine.find("int main(") != string::npos) {
            scopeLevel = 0;
            return true;
        }

        if (trimmedLine == "{") {
            scopeLevel++;
            return true;
        }

        if (trimmedLine == "}") {
            if (scopeLevel > 0) {
                scopeLevel--;
            }
            return true;
        }

        if (trimmedLine.find("delete ") == 0) return parseDelete(trimmedLine);

        if (trimmedLine.find(" = new ") != string::npos) {
            auto tokens = tokenize(trimmedLine);
            if (tokens.size() >= 2 && (isBasicType(tokens[0]) || tokens[0].find('*') != string::npos)) {
                size_t equalPos = trimmedLine.find('=');
                string declPart = trim(trimmedLine.substr(0, equalPos));
                if (!parseDeclaration(declPart + ";")) return false;
                return parseNew(trimmedLine);
            }
            else {
                return parseNew(trimmedLine);
            }
        }

        if (trimmedLine.find(" = ") != string::npos && trimmedLine.find(';') != string::npos) {
            auto tokens = tokenize(trimmedLine);
            if (tokens.size() >= 2 && (isBasicType(tokens[0]) || tokens[0].find('*') != string::npos)) {
                size_t equalPos = trimmedLine.find('=');
                string declPart = trim(trimmedLine.substr(0, equalPos));

                if (!parseDeclaration(declPart + ";")) return false;

                string varName = tokens[1];
                if (tokens[0].find('*') != string::npos && tokens.size() == 2) {
                }
                else if (tokens.size() > 2) {
                    auto declTokens = tokenize(declPart);
                    varName = declTokens.back();
                }
                varName.erase(remove(varName.begin(), varName.end(), '*'), varName.end());

                string assignCmd = varName + " = " + trim(trimmedLine.substr(equalPos + 1));
                return parseAssignment(assignCmd);
            }
            return parseAssignment(trimmedLine);
        }

        if (trimmedLine.find("return ") == 0) return true;

        auto tokens = tokenize(trimmedLine);
        if (tokens.size() >= 2) {
            if (isBasicType(tokens[0]) || tokens[0].find('*') != string::npos) {
                return parseDeclaration(trimmedLine);
            }
        }

        return true;
    }

public:
    ScriptParser(MemoryManager& manager)
        : memManager(manager), scopeLevel(0) {
    }

    // 스크립트를 한 줄씩 단계별로 실행
    bool executeScriptStepByStep(const string& script,
        function<void(const string&, int)> stepCallback) {
        auto lines = splitIntoLines(script);
        int lineNumber = 0;

        for (const auto& line : lines) {
            lineNumber++;
            string cleaned = removeComments(line);
            string trimmedLine = trim(cleaned);

            if (trimmedLine.empty()) continue;

            if (stepCallback) {
                stepCallback(line, lineNumber);
            }

            if (!executeLine(trimmedLine)) {
                return false;
            }
        }

        if (stepCallback) {
            stepCallback("// 프로그램 종료 - 스택 메모리 정리 중...", lineNumber + 1);
        }

        memManager.clearAllStack();

        return true;
    }

    // 파서 초기화
    void reset() {
        variables.clear();
        scopeLevel = 0;
    }

    // 예제 스크립트 가져오기
    static string getExampleScript(int index) {
        switch (index) {
        case 0:
            return
                "// 예제 1: Raw Pointer - Memory Leak\n"
                "int main() {\n"
                "    int* ptr = new int;\n"
                "    return 0;\n"
                "}";

        case 1:
            return
                "// 예제 2: Raw Pointer - Proper Usage\n"
                "int main() {\n"
                "    int* ptr = new int;\n"
                "    delete ptr;\n"
                "    return 0;\n"
                "}";

        case 2:
            return
                "// 예제 3: Dangling Pointer\n"
                "int main() {\n"
                "    int* ptr1 = new int;\n"
                "    int* ptr2 = ptr1;\n"
                "    delete ptr1;\n"
                "    return 0;\n"
                "}";

        case 3:
            return
                "// 예제 4: Stack vs Heap\n"
                "int main() {\n"
                "    int stackVar = 5;\n"
                "    int* heapPtr = new int;\n"
                "    delete heapPtr;\n"
                "    return 0;\n"
                "}";

        case 4:
            return
                "// 예제 5: 다중 포인터 (Double Pointer)\n"
                "int main() {\n"
                "    int* ptr = new int;\n"
                "    int** dptr = &ptr;\n"
                "    delete ptr;\n"
                "    return 0;\n"
                "}";

        case 5:
            return
                "// 예제 6: 포인터 배열\n"
                "int main() {\n"
                "    int* arr1 = new int;\n"
                "    int* arr2 = new int;\n"
                "    int* arr3 = new int;\n"
                "    delete arr3;\n"
                "    delete arr2;\n"
                "    delete arr1;\n"
                "    return 0;\n"
                "}";

        default:
            return "";
        }
    }

    static int getExampleCount() {
        return 6;
    }
};

// ==================== 메인 함수 ====================

// 메인 메뉴 출력
void displayMenu() {
    cout << "\n+-- 메뉴 ---------------------------+" << endl;
    cout << "| 1. 예제 스크립트 단계별 실행      |" << endl;
    cout << "| 2. 직접 코드 단계별 실행          |" << endl;
    cout << "| 0. 종료                           |" << endl;
    cout << "+-----------------------------------+" << endl;
    cout << "선택: ";
}

// 예제 목록 출력
void displayExamples() {
    cout << "\n예제 스크립트:" << endl;
    for (int i = 0; i < ScriptParser::getExampleCount(); i++) {
        cout << "  " << (i + 1) << ". ";
        string script = ScriptParser::getExampleScript(i);
        size_t newline = script.find('\n');
        if (newline != string::npos) {
            cout << script.substr(3, newline - 3);
        }
        cout << endl;
    }
    cout << "  0. 돌아가기" << endl;
    cout << "선택: ";
}


// 예제 스크립트를 단계별로 실행
void runExampleStepByStep(int index, MemoryManager& memManager, ScriptParser& parser, Visualizer& visualizer) {
    parser.reset();
    memManager.reset();

    string script = ScriptParser::getExampleScript(index);

    cout << "\n" << "=== 스크립트 (단계별 실행) ===" << endl;
    cout << script << endl;
    cout << "===============================" << endl;

    cout << "\n⭐ 단계별 실행 모드" << endl;
    cout << "각 줄이 실행될 때마다 메모리 변화를 확인할 수 있습니다." << endl;
    cout << "\n아무 키나 누르면 시작합니다...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();

    bool result = parser.executeScriptStepByStep(script,
        [&memManager, &visualizer](const string& line, int lineNum) {
            visualizer.printMemoryStateWithLine(memManager, line, lineNum);
            cin.get();
        });

    if (!result) {
        cout << "\n[ERROR] 스크립트 실행 실패!" << endl;
        cout << "\n아무 키나 누르면 계속...";
        cin.get();
    }
    else {
        visualizer.clearScreen();
        cout << "\n" << "✅ 프로그램 실행 완료!" << endl;
        cout << "\n최종 메모리 상태:" << endl << endl;

        auto leaks = memManager.detectLeaks();
        if (!leaks.empty()) {
            cout << "\033[1;31m";
            cout << "⚠ 메모리 누수 감지! " << leaks.size() << "개 블록" << "\033[0m" << endl;
            for (int id : leaks) {
                const MemoryBlock* block = memManager.findBlock(id);
                if (block) {
                    cout << "  - \033[31m" << block->name << " (" << block->size << " bytes)\033[0m" << endl;
                }
            }
            cout << endl;
        }
        else {
            cout << "\033[32m[OK] 메모리 누수가 없습니다!\033[0m" << endl << endl;
        }

        visualizer.printMemoryState(memManager);

        cout << "\n아무 키나 누르면 계속...";
        cin.get();
    }
}

// 사용자가 직접 입력한 코드를 단계별로 실행
void runCustomCode(MemoryManager& memManager, ScriptParser& parser, Visualizer& visualizer) {
    parser.reset();
    memManager.reset();

    cout << "\n┌──────────────────────────────────┐" << endl;
    cout << "코드를 입력하세요 (입력 완료 후 빈 줄에서 Enter):" << endl;
    cout << "└──────────────────────────────────┘" << endl;
    cout << "\n예시:\n";
    cout << "int main() {\n";
    cout << "    int x = 5;\n";
    cout << "    int* ptr = new int;\n";
    cout << "    delete ptr;\n";
    cout << "    return 0;\n";
    cout << "}\n" << endl;
    cout << "입력 시작 (빈 줄에서 Enter로 종료):\n" << endl;

    string code;
    string line;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    int lineCount = 0;
    while (true) {
        cout << (lineCount + 1) << ": ";
        getline(cin, line);

        if (line.empty()) {
            cout << "[DEBUG] 입력 종료 (총 " << lineCount << "줄)" << endl;
            break;
        }

        code += line + "\n";
        lineCount++;
    }

    if (code.empty()) {
        cout << "\n[ERROR] 코드가 입력되지 않았습니다." << endl;
        cout << "아무 키나 누르면 계속...";
        cin.get();
        return;
    }

    cout << "\n[DEBUG] 입력된 코드:\n" << code << endl;

    cout << "\n⭐ 단계별 실행 시작" << endl;
    cout << "각 줄이 실행될 때마다 메모리 변화를 확인할 수 있습니다." << endl;
    cout << "\n아무 키나 누르면 시작합니다...";
    cin.get();

    bool result = parser.executeScriptStepByStep(code,
        [&memManager, &visualizer](const string& line, int lineNum) {
            visualizer.printMemoryStateWithLine(memManager, line, lineNum);
            cin.get();
        });

    if (!result) {
        cout << "\n[ERROR] 스크립트 실행 실패!" << endl;
        cout << "\n아무 키나 누르면 계속...";
        cin.get();
    }
    else {
        visualizer.clearScreen();
        cout << "\n" << "✅ 프로그램 실행 완료!" << endl;
        cout << "\n최종 메모리 상태:" << endl << endl;

        auto leaks = memManager.detectLeaks();
        if (!leaks.empty()) {
            cout << "\033[1;31m";
            cout << "⚠ 메모리 누수 감지! " << leaks.size() << "개 블록" << "\033[0m" << endl;
            for (int id : leaks) {
                const MemoryBlock* block = memManager.findBlock(id);
                if (block) {
                    cout << "  - \033[31m" << block->name << " (" << block->size << " bytes)\033[0m" << endl;
                }
            }
            cout << endl;
        }
        else {
            cout << "\033[32m[OK] 메모리 누수가 없습니다!\033[0m" << endl << endl;
        }

        visualizer.printMemoryState(memManager);

        cout << "\n아무 키나 누르면 계속...";
        cin.get();
    }
}

// 프로그램 시작점
int main() {
    MemoryManager memManager;
    Visualizer visualizer;
    ScriptParser parser(memManager);

    cout << "\033[1;36m";
    cout << R"(
====================================
                                                                
   C++ Memory Visualizer Tool
   메모리 관리 시각화 도구
                                                                                                                    
====================================
)" << endl;
    cout << "\033[0m";

    cout << "\n아무 키나 누르면 시작합니다...";
    cin.get();

    bool running = true;

    while (running) {
        displayMenu();

        int choice;
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1: {
            displayExamples();
            int exampleChoice;
            cin >> exampleChoice;

            if (exampleChoice > 0 && exampleChoice <= ScriptParser::getExampleCount()) {
                runExampleStepByStep(exampleChoice - 1, memManager, parser, visualizer);
            }
            break;
        }

        case 2: {
            runCustomCode(memManager, parser, visualizer);
            break;
        }

        case 0: {
            running = false;
            cout << "\n프로그램을 종료합니다." << endl;
            break;
        }

        default:
            cout << "잘못된 선택입니다." << endl;
            cout << "아무 키나 누르면 계속...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            break;
        }
    }

    return 0;
}

#include "ScriptParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

ScriptParser::ScriptParser(MemoryManager& manager)
    : memManager(manager), scopeLevel(0), inFunction(false),
    inClass(false), currentClassName("") {
}

ScriptParser::~ScriptParser() {
}

// 전체 스크립트 실행
bool ScriptParser::executeScript(const std::string& script) {
    auto lines = splitIntoLines(script);

    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = removeComments(lines[i]);
        line = trim(line);

        if (line.empty()) continue;

        if (!executeLine(line)) {
            lastError = "Line " + std::to_string(i + 1) + ": " + lastError;
            return false;
        }
    }

    return true;
}

// 한 줄 실행
bool ScriptParser::executeLine(const std::string& line) {
    std::string trimmedLine = trim(line);

    if (trimmedLine.empty()) return true;

    // 클래스 파싱
    if (trimmedLine.find("class ") == 0) {
        return parseClassDeclaration(trimmedLine);
    }

    // 클래스 정의 내부
    if (inClass) {
        if (trimmedLine.find("public:") == 0 ||
            trimmedLine.find("private:") == 0 ||
            trimmedLine.find("protected:") == 0) {
            return true;  // 접근 지정자 무시
        }

        if (trimmedLine == "};") {
            return parseClassEnd(trimmedLine);
        }

        return parseClassMember(trimmedLine);
    }

    // 함수 시작
    if (trimmedLine.find("int main") != std::string::npos ||
        trimmedLine.find("void main") != std::string::npos) {
        return parseFunctionStart(trimmedLine);
    }

    // return 문
    if (trimmedLine.find("return") == 0) {
        return parseReturn(trimmedLine);
    }

    // 스코프 시작
    if (trimmedLine == "{") {
        return parseScopeStart(trimmedLine);
    }

    // 스코프 종료
    if (trimmedLine == "}") {
        return parseScopeEnd(trimmedLine);
    }

    // 함수 내부에서만 명령어 실행
    if (!inFunction) {
        return true;  // 함수 밖 코드는 무시
    }

    // delete 문
    if (trimmedLine.find("delete ") == 0) {
        return parseDelete(trimmedLine);
    }

    // int* ptr = new int; 같은 선언+new 할당
    if (trimmedLine.find(" = new ") != std::string::npos) {
        auto tokens = tokenize(trimmedLine);
        // 첫 토큰이 타입이면 선언+할당
        if (tokens.size() >= 2 && (isBasicType(tokens[0]) || isClassType(tokens[0]))) {
            // 먼저 선언 파싱
            size_t equalPos = trimmedLine.find('=');
            std::string declPart = trim(trimmedLine.substr(0, equalPos));
            if (!parseDeclaration(declPart + ";")) {
                return false;
            }
            // 그 다음 new 할당 파싱
            return parseNew(trimmedLine);
        }
        else {
            // 기존 변수에 new 할당
            return parseNew(trimmedLine);
        }
    }

    // 스마트 포인터
    if (trimmedLine.find("unique_ptr") != std::string::npos ||
        trimmedLine.find("shared_ptr") != std::string::npos ||
        trimmedLine.find("make_unique") != std::string::npos ||
        trimmedLine.find("make_shared") != std::string::npos) {
        return parseSmartPtr(trimmedLine);
    }

    // 할당문 (=가 있고 == 가 아님)
    if (trimmedLine.find('=') != std::string::npos &&
        trimmedLine.find("==") == std::string::npos) {

        // 초기화와 할당 구분
        auto tokens = tokenize(trimmedLine);
        if (tokens.size() >= 3 && isBasicType(tokens[0])) {
            return parseDeclarationWithInit(trimmedLine);
        }
        else if (tokens.size() >= 2 && isClassType(tokens[0])) {
            return parseObjectDeclaration(trimmedLine);
        }
        else {
            return parseAssignment(trimmedLine);
        }
    }

    // 변수 선언
    auto tokens = tokenize(trimmedLine);
    if (tokens.size() >= 2) {
        if (isBasicType(tokens[0]) || isPointerType(tokens[0])) {
            return parseDeclaration(trimmedLine);
        }
        else if (isClassType(tokens[0])) {
            return parseObjectDeclaration(trimmedLine);
        }
    }

    return true;
}

// 클래스 선언 파싱
bool ScriptParser::parseClassDeclaration(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.size() < 2) {
        lastError = "Invalid class declaration";
        return false;
    }

    currentClassName = tokens[1];

    // 세미콜론이나 중괄호 제거
    if (!currentClassName.empty()) {
        if (currentClassName.back() == '{') {
            currentClassName.pop_back();
        }
        currentClassName = trim(currentClassName);
    }

    ClassInfo classInfo;
    classInfo.className = currentClassName;
    classInfo.totalSize = 0;

    classes[currentClassName] = classInfo;
    inClass = true;

    return true;
}

// 클래스 멤버 파싱
bool ScriptParser::parseClassMember(const std::string& line) {
    // 생성자/소멸자 감지
    if (line.find(currentClassName + "()") != std::string::npos) {
        classes[currentClassName].hasConstructor = true;
        return true;
    }
    if (line.find("~" + currentClassName + "()") != std::string::npos) {
        classes[currentClassName].hasDestructor = true;
        return true;
    }

    // 멤버 변수 파싱
    auto tokens = tokenize(line);
    if (tokens.size() < 2) return true;

    std::string type = tokens[0];
    std::string name = tokens[1];

    // 세미콜론 제거
    if (!name.empty() && name.back() == ';') {
        name.pop_back();
    }

    if (isBasicType(type)) {
        size_t size = getTypeSize(type);

        classes[currentClassName].memberNames.push_back(name);
        classes[currentClassName].memberTypes.push_back(type);
        classes[currentClassName].memberSizes.push_back(size);
        classes[currentClassName].totalSize += size;
    }
    else if (isClassType(type)) {
        // 다른 클래스 타입 멤버
        if (classes.find(type) != classes.end()) {
            size_t size = classes[type].totalSize;

            classes[currentClassName].memberNames.push_back(name);
            classes[currentClassName].memberTypes.push_back(type);
            classes[currentClassName].memberSizes.push_back(size);
            classes[currentClassName].totalSize += size;
        }
    }

    return true;
}

// 클래스 종료
bool ScriptParser::parseClassEnd(const std::string& line) {
    inClass = false;
    currentClassName = "";
    return true;
}

// 함수 시작
bool ScriptParser::parseFunctionStart(const std::string& line) {
    inFunction = true;
    scopeLevel = 0;

    // 다음 줄이 { 인 경우를 대비
    if (line.find('{') != std::string::npos) {
        scopeLevel = 1;
    }

    return true;
}

// 함수 종료
bool ScriptParser::parseFunctionEnd(const std::string& line) {
    if (scopeLevel == 0) {
        inFunction = false;
    }
    return true;
}

// return 문
bool ScriptParser::parseReturn(const std::string& line) {
    // return은 특별히 처리할 것 없음
    return true;
}

// 스코프 시작
bool ScriptParser::parseScopeStart(const std::string& line) {
    scopeLevel++;
    return true;
}

// 스코프 종료
bool ScriptParser::parseScopeEnd(const std::string& line) {
    if (scopeLevel > 0) {
        scopeLevel--;
        memManager.endScope();

        if (scopeLevel == 0 && inFunction) {
            inFunction = false;
        }

        return true;
    }

    lastError = "Unexpected scope end";
    return false;
}

// 기본 변수 선언
bool ScriptParser::parseDeclaration(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.size() < 2) {
        lastError = "Invalid declaration: " + line;
        return false;
    }

    std::string type = tokens[0];
    std::string name = tokens[1];

    // 세미콜론 제거
    if (!name.empty() && name.back() == ';') {
        name.pop_back();
    }

    // 포인터 처리
    bool isPtr = isPointerType(type) || (!name.empty() && name[0] == '*');
    if (!name.empty() && name[0] == '*') {
        name = name.substr(1);
    }

    size_t size = isPtr ? sizeof(void*) : getTypeSize(type);

    if (isPtr) {
        // 포인터 변수
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
        // 일반 변수
        int id = memManager.createStackVariable(name, size);
        variables[name] = id;
    }

    return true;
}

// 초기화를 포함한 선언
bool ScriptParser::parseDeclarationWithInit(const std::string& line) {
    size_t equalPos = line.find('=');
    if (equalPos == std::string::npos) {
        return parseDeclaration(line);
    }

    std::string declPart = trim(line.substr(0, equalPos));
    std::string valuePart = trim(line.substr(equalPos + 1));

    // 세미콜론 제거
    if (!valuePart.empty() && valuePart.back() == ';') {
        valuePart.pop_back();
        valuePart = trim(valuePart);
    }

    // 먼저 변수 선언
    if (!parseDeclaration(declPart + ";")) {
        return false;
    }

    // 값은 무시 (시뮬레이션이므로)
    return true;
}

// 객체 선언
bool ScriptParser::parseObjectDeclaration(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.size() < 2) {
        lastError = "Invalid object declaration";
        return false;
    }

    std::string className = tokens[0];
    std::string objName = tokens[1];

    // 포인터인지 확인
    bool isPtr = (objName[0] == '*');
    if (isPtr) {
        objName = objName.substr(1);
    }

    // 세미콜론 제거
    if (!objName.empty() && objName.back() == ';') {
        objName.pop_back();
    }

    // 클래스 정보 찾기
    if (classes.find(className) == classes.end()) {
        lastError = "Unknown class: " + className;
        return false;
    }

    ClassInfo* classInfo = &classes[className];

    if (isPtr) {
        // 포인터 변수
        int id = memManager.createStackVariable(objName, sizeof(void*));
        MemoryBlock* block = memManager.findBlock(id);
        if (block) {
            block->isPointer = true;
            block->pointerType = PointerType::RAW;
            block->pointsTo = -1;
            block->isObject = true;
            block->classInfo = classInfo;
        }
        variables[objName] = id;
    }
    else {
        // 스택 객체
        int id = memManager.createClassObject(objName, classInfo);
        variables[objName] = id;
    }

    return true;
}

// new 할당 파싱
bool ScriptParser::parseNew(const std::string& line) {
    size_t equalPos = line.find('=');
    size_t newPos = line.find("new ");

    if (equalPos == std::string::npos || newPos == std::string::npos) {
        lastError = "Invalid new statement";
        return false;
    }

    std::string varName = trim(line.substr(0, equalPos));
    std::string afterNew = trim(line.substr(newPos + 4));

    // 타입 추출
    std::string typeName = "";
    size_t parenPos = afterNew.find('(');
    if (parenPos != std::string::npos) {
        typeName = trim(afterNew.substr(0, parenPos));
    }
    else {
        size_t semicolonPos = afterNew.find(';');
        if (semicolonPos != std::string::npos) {
            typeName = trim(afterNew.substr(0, semicolonPos));
        }
        else {
            typeName = trim(afterNew);
        }
    }

    // 변수 확인
    if (variables.find(varName) == variables.end()) {
        lastError = "Undefined variable: " + varName;
        return false;
    }

    int ptrId = variables[varName];
    MemoryBlock* ptrBlock = memManager.findBlock(ptrId);

    if (!ptrBlock || !ptrBlock->isPointer) {
        lastError = "Variable is not a pointer: " + varName;
        return false;
    }

    // 힙 메모리 할당
    int heapId;
    if (isClassType(typeName)) {
        // 클래스 객체
        if (classes.find(typeName) == classes.end()) {
            lastError = "Unknown class: " + typeName;
            return false;
        }
        heapId = memManager.allocateClassObject(typeName + "_heap", &classes[typeName], PointerType::RAW);
    }
    else {
        // 기본 타입
        size_t size = getTypeSize(typeName);
        heapId = memManager.allocateHeap(typeName + "_heap", size, PointerType::RAW);
    }

    // 포인터 연결
    memManager.assignPointer(ptrId, heapId);

    return true;
}

// delete 파싱
bool ScriptParser::parseDelete(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.size() < 2) {
        lastError = "Invalid delete statement";
        return false;
    }

    std::string varName = tokens[1];
    if (!varName.empty() && varName.back() == ';') {
        varName.pop_back();
    }

    if (variables.find(varName) == variables.end()) {
        lastError = "Undefined variable: " + varName;
        return false;
    }

    int ptrId = variables[varName];
    MemoryBlock* ptrBlock = memManager.findBlock(ptrId);

    if (!ptrBlock || !ptrBlock->isPointer) {
        lastError = "Variable is not a pointer: " + varName;
        return false;
    }

    if (ptrBlock->pointsTo == -1) {
        lastError = "Cannot delete null pointer: " + varName;
        return false;
    }

    // 메모리 해제
    memManager.deallocate(ptrBlock->pointsTo);
    memManager.assignPointer(ptrId, -1);

    return true;
}

// 스마트 포인터 파싱
bool ScriptParser::parseSmartPtr(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.size() < 2) {
        lastError = "Invalid smart pointer declaration";
        return false;
    }

    PointerType ptrType = getPointerType(tokens[0]);

    // 변수 이름 추출
    std::string varName = "";
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i].find('>') != std::string::npos) {
            if (i + 1 < tokens.size()) {
                varName = tokens[i + 1];
                break;
            }
        }
    }

    if (varName.empty()) {
        lastError = "Cannot extract variable name";
        return false;
    }

    // = 제거
    if (varName == "=") {
        if (tokens.size() > 3) {
            varName = tokens[1];
        }
    }

    // 세미콜론 제거
    if (!varName.empty() && varName.back() == ';') {
        varName.pop_back();
    }

    // 포인터 변수 생성
    int ptrId = memManager.createStackVariable(varName, sizeof(void*));
    MemoryBlock* ptrBlock = memManager.findBlock(ptrId);
    if (ptrBlock) {
        ptrBlock->isPointer = true;
        ptrBlock->pointerType = ptrType;
        ptrBlock->pointsTo = -1;
    }
    variables[varName] = ptrId;

    // make_unique 또는 make_shared가 있으면 힙 할당
    if (line.find("make_unique") != std::string::npos ||
        line.find("make_shared") != std::string::npos) {

        // 타입 추출
        size_t ltPos = line.find('<');
        size_t gtPos = line.find('>', ltPos);

        if (ltPos != std::string::npos && gtPos != std::string::npos) {
            std::string typeName = trim(line.substr(ltPos + 1, gtPos - ltPos - 1));

            int heapId;
            if (isClassType(typeName)) {
                if (classes.find(typeName) != classes.end()) {
                    heapId = memManager.allocateClassObject(varName + "_data", &classes[typeName], ptrType);
                    memManager.assignPointer(ptrId, heapId);
                }
            }
            else {
                heapId = memManager.allocateHeap(varName + "_data", getTypeSize(typeName), ptrType);
                memManager.assignPointer(ptrId, heapId);
            }
        }
    }

    return true;
}

// 할당문 파싱
bool ScriptParser::parseAssignment(const std::string& line) {
    size_t equalPos = line.find('=');
    if (equalPos == std::string::npos) {
        return false;
    }

    std::string left = trim(line.substr(0, equalPos));
    std::string right = trim(line.substr(equalPos + 1));

    // 세미콜론 제거
    if (!right.empty() && right.back() == ';') {
        right.pop_back();
        right = trim(right);
    }

    // 좌변 변수 확인
    if (variables.find(left) == variables.end()) {
        lastError = "Undefined variable: " + left;
        return false;
    }

    int leftId = variables[left];
    MemoryBlock* leftBlock = memManager.findBlock(leftId);

    if (!leftBlock) {
        lastError = "Invalid block: " + left;
        return false;
    }

    // nullptr 할당
    if (right == "nullptr" || right == "NULL") {
        if (leftBlock->isPointer) {
            memManager.assignPointer(leftId, -1);
        }
        return true;
    }

    // 우변이 변수인 경우
    if (variables.find(right) != variables.end()) {
        int rightId = variables[right];
        MemoryBlock* rightBlock = memManager.findBlock(rightId);

        if (rightBlock && leftBlock->isPointer && rightBlock->isPointer) {
            // shared_ptr 복사
            if (leftBlock->pointerType == PointerType::SHARED &&
                rightBlock->pointerType == PointerType::SHARED) {
                int newId = memManager.copySharedPtr(rightId, left);
                if (newId != -1) {
                    variables[left] = newId;
                }
            }
            // unique_ptr 이동
            else if (leftBlock->pointerType == PointerType::UNIQUE &&
                rightBlock->pointerType == PointerType::UNIQUE &&
                line.find("std::move") != std::string::npos) {
                memManager.moveUniquePtr(rightId, leftId);
            }
            // 일반 포인터 할당
            else {
                memManager.assignPointer(leftId, rightBlock->pointsTo);
            }
        }
    }

    return true;
}

// 라인 분리
std::vector<std::string> ScriptParser::splitIntoLines(const std::string& code) {
    std::vector<std::string> lines;
    std::istringstream stream(code);
    std::string line;

    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    return lines;
}

// 주석 제거
std::string ScriptParser::removeComments(const std::string& line) {
    std::string result = line;

    // // 스타일 주석
    size_t doubleSlash = result.find("//");
    if (doubleSlash != std::string::npos) {
        result = result.substr(0, doubleSlash);
    }

    // /* */ 스타일 주석 (간단한 버전)
    size_t commentStart = result.find("/*");
    size_t commentEnd = result.find("*/");
    if (commentStart != std::string::npos && commentEnd != std::string::npos) {
        result = result.substr(0, commentStart) + result.substr(commentEnd + 2);
    }

    return result;
}

// 공백 제거
std::string ScriptParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// 토큰화
std::vector<std::string> ScriptParser::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

// 기본 타입 확인
bool ScriptParser::isBasicType(const std::string& type) {
    return type == "int" || type == "float" || type == "double" ||
        type == "char" || type == "long" || type == "short" ||
        type == "bool" || type == "void" || type == "size_t";
}

// 포인터 타입 확인
bool ScriptParser::isPointerType(const std::string& type) {
    return type.find('*') != std::string::npos ||
        type.find("_ptr") != std::string::npos;
}

// 클래스 타입 확인
bool ScriptParser::isClassType(const std::string& type) {
    return classes.find(type) != classes.end();
}

// 포인터 타입 가져오기
PointerType ScriptParser::getPointerType(const std::string& type) {
    if (type.find("unique_ptr") != std::string::npos) {
        return PointerType::UNIQUE;
    }
    else if (type.find("shared_ptr") != std::string::npos) {
        return PointerType::SHARED;
    }
    return PointerType::RAW;
}

// 타입 크기 가져오기
size_t ScriptParser::getTypeSize(const std::string& type) {
    if (type.find("int") != std::string::npos) return sizeof(int);
    if (type.find("double") != std::string::npos) return sizeof(double);
    if (type.find("float") != std::string::npos) return sizeof(float);
    if (type.find("char") != std::string::npos) return sizeof(char);
    if (type.find("long") != std::string::npos) return sizeof(long);
    if (type.find("short") != std::string::npos) return sizeof(short);
    if (type.find("bool") != std::string::npos) return sizeof(bool);
    return sizeof(int);
}

// 타입 이름 추출
std::string ScriptParser::extractTypeName(const std::string& declaration) {
    auto tokens = tokenize(declaration);
    return tokens.empty() ? "" : tokens[0];
}

// 변수 이름 추출
std::string ScriptParser::extractVariableName(const std::string& declaration) {
    auto tokens = tokenize(declaration);
    return tokens.size() >= 2 ? tokens[1] : "";
}

// 변수 ID 가져오기
int ScriptParser::getVariableId(const std::string& name) const {
    auto it = variables.find(name);
    return (it != variables.end()) ? it->second : -1;
}

// 클래스 정보 가져오기
ClassInfo* ScriptParser::getClassInfo(const std::string& className) {
    auto it = classes.find(className);
    return (it != classes.end()) ? &it->second : nullptr;
}

// 초기화
void ScriptParser::reset() {
    variables.clear();
    classes.clear();
    lastError.clear();
    scopeLevel = 0;
    inFunction = false;
    inClass = false;
    currentClassName = "";
}

// 예제 스크립트
std::string ScriptParser::getExampleScript(int index) {
    switch (index) {
    case 0:
        return
            "// 예제 1: Raw Pointer - Memory Leak\n"
            "int main() {\n"
            "    int* ptr = new int;\n"
            "    // delete 호출 없이 종료\n"
            "    return 0;\n"
            "}  // ⚠️ Memory Leak!";

    case 1:
        return
            "// 예제 2: Raw Pointer - Proper Usage\n"
            "int main() {\n"
            "    int* ptr = new int;\n"
            "    delete ptr;\n"
            "    return 0;\n"
            "}  // ✅ 메모리 제대로 해제";

    case 2:
        return
            "// 예제 3: unique_ptr - Automatic Management\n"
            "int main() {\n"
            "    unique_ptr<int> ptr = make_unique<int>();\n"
            "    return 0;\n"
            "}  // ✅ 자동으로 메모리 해제";

    case 3:
        return
            "// 예제 4: shared_ptr - Reference Counting\n"
            "int main() {\n"
            "    shared_ptr<int> ptr1 = make_shared<int>();\n"
            "    shared_ptr<int> ptr2 = ptr1;\n"
            "    // ptr2 복사로 참조 카운트 2\n"
            "    return 0;\n"
            "}  // 참조 카운트 0이 되면 자동 해제";

    case 4:
        return
            "// 예제 5: Dangling Pointer\n"
            "int main() {\n"
            "    int* ptr1 = new int;\n"
            "    int* ptr2 = ptr1;\n"
            "    delete ptr1;\n"
            "    // ptr2는 이제 dangling pointer!\n"
            "    return 0;\n"
            "}";

    case 5:
        return
            "// 예제 6: Stack vs Heap\n"
            "int main() {\n"
            "    int stackVar = 5;\n"
            "    int* heapPtr = new int;\n"
            "    delete heapPtr;\n"
            "    return 0;\n"
            "}";

    case 6:
        return
            "// 예제 7: 기본 클래스\n"
            "class Point {\n"
            "public:\n"
            "    int x;\n"
            "    int y;\n"
            "};\n\n"
            "int main() {\n"
            "    Point p1;\n"
            "    Point* p2 = new Point();\n"
            "    delete p2;\n"
            "    return 0;\n"
            "}";

    case 7:
        return
            "// 예제 8: 클래스 + 스마트 포인터\n"
            "class Player {\n"
            "public:\n"
            "    int health;\n"
            "    int mana;\n"
            "};\n\n"
            "int main() {\n"
            "    Player p1;\n"
            "    unique_ptr<Player> p2 = make_unique<Player>();\n"
            "    shared_ptr<Player> p3 = make_shared<Player>();\n"
            "    return 0;\n"
            "}";

    case 8:
        return
            "// 예제 9: 중첩 클래스\n"
            "class Vector {\n"
            "public:\n"
            "    int x;\n"
            "    int y;\n"
            "};\n\n"
            "class Entity {\n"
            "public:\n"
            "    Vector position;\n"
            "    int id;\n"
            "};\n\n"
            "int main() {\n"
            "    Entity e;\n"
            "    return 0;\n"
            "}";

    default:
        return "";
    }
}

int ScriptParser::getExampleCount() {
    return 10;
}
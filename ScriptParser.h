#pragma once
#include "MemoryManager.h"
#include <string>
#include <vector>
#include <unordered_map>

// C++ 코드 파서 (main 함수 + 클래스 지원)
class ScriptParser {
public:
    ScriptParser(MemoryManager& manager);
    ~ScriptParser();

    // 전체 스크립트 실행 (main 함수 포함)
    bool executeScript(const std::string& script);

    // 한 줄 명령어 실행
    bool executeLine(const std::string& line);

    // 에러 메시지 가져오기
    const std::string& getLastError() const { return lastError; }

    // 변수 이름 -> 블록 ID 매핑
    int getVariableId(const std::string& name) const;

    // 클래스 정의 가져오기
    ClassInfo* getClassInfo(const std::string& className);

    // 초기화
    void reset();

    // 예제 스크립트들
    static std::string getExampleScript(int index);
    static int getExampleCount();

private:
    MemoryManager& memManager;
    std::unordered_map<std::string, int> variables;      // 변수 이름 -> 블록 ID
    std::unordered_map<std::string, ClassInfo> classes;  // 클래스 정의들
    std::string lastError;
    int scopeLevel;                                      // 스코프 레벨
    bool inFunction;                                     // 함수 안인지
    bool inClass;                                        // 클래스 정의 안인지
    std::string currentClassName;                        // 현재 파싱 중인 클래스 이름

    // 전처리
    std::vector<std::string> splitIntoLines(const std::string& code);
    std::string removeComments(const std::string& line);
    std::string trim(const std::string& str);

    // 클래스 파싱
    bool parseClassDeclaration(const std::string& line);
    bool parseClassMember(const std::string& line);
    bool parseClassEnd(const std::string& line);

    // 함수 파싱
    bool parseFunctionStart(const std::string& line);
    bool parseFunctionEnd(const std::string& line);
    bool parseReturn(const std::string& line);

    // 변수 선언 파싱
    bool parseDeclaration(const std::string& line);
    bool parseDeclarationWithInit(const std::string& line);

    // 객체 선언 파싱
    bool parseObjectDeclaration(const std::string& line);

    // 메모리 할당/해제
    bool parseNew(const std::string& line);
    bool parseDelete(const std::string& line);

    // 스마트 포인터
    bool parseSmartPtr(const std::string& line);

    // 할당문
    bool parseAssignment(const std::string& line);

    // 스코프 관리
    bool parseScopeStart(const std::string& line);
    bool parseScopeEnd(const std::string& line);

    // 유틸리티
    std::vector<std::string> tokenize(const std::string& line);
    bool isBasicType(const std::string& type);
    bool isPointerType(const std::string& type);
    bool isClassType(const std::string& type);
    PointerType getPointerType(const std::string& type);
    size_t getTypeSize(const std::string& type);
    std::string extractTypeName(const std::string& declaration);
    std::string extractVariableName(const std::string& declaration);
};

#pragma once
#include "MemoryManager.h"
#include <iostream>
#include <iomanip>
#include <string>

// 콘솔 기반 메모리 시각화
class Visualizer {
public:
    Visualizer();
    ~Visualizer();

    // 메모리 상태 출력
    void printMemoryState(const MemoryManager& memManager);

    // 스택 영역 출력
    void printStack(const std::vector<MemoryBlock>& blocks);

    // 힙 영역 출력
    void printHeap(const std::vector<MemoryBlock>& blocks);

    // 포인터 관계 출력
    void printPointerConnections(const std::vector<MemoryBlock>& blocks);

    // 이벤트 로그 출력
    void printEventLog(const std::vector<MemoryEvent>& events, int count = 10);

    // 메모리 누수 경고 출력
    void printLeakWarnings(const std::vector<int>& leakIds, const MemoryManager& memManager);

    // 화면 지우기
    void clearScreen();

    // 구분선 출력
    void printSeparator(char ch = '=', int length = 70) const;

    // 메모리 블록 한 줄 출력
    void printMemoryBlock(const MemoryBlock& block) const;

private:
    // 색상 코드 (ANSI)
    std::string colorReset;
    std::string colorRed;
    std::string colorGreen;
    std::string colorYellow;
    std::string colorBlue;
    std::string colorMagenta;
    std::string colorCyan;
    std::string colorBold;

    // 블록 타입별 색상 가져오기
    std::string getBlockColor(const MemoryBlock& block) const;

    // 포인터 타입 문자열
    std::string getPointerTypeString(PointerType type) const;
};

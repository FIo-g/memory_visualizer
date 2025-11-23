#include "MemoryManager.h"
#include "Visualizer.h"
#include "ScriptParser.h"
#include <iostream>
#include <string>
#include <limits>

void displayMenu() {
    std::cout << "\n+-- 메뉴 ---------------------------+" << std::endl;
    std::cout << "| 1. 예제 스크립트 실행             |" << std::endl;
    std::cout << "| 2. 직접 코드 입력                 |" << std::endl;
    std::cout << "| 3. 메모리 누수 검사               |" << std::endl;
    std::cout << "| 4. 초기화                         |" << std::endl;
    std::cout << "| 9. 간단 테스트 (디버깅용)         |" << std::endl;
    std::cout << "| 0. 종료                           |" << std::endl;
    std::cout << "+-----------------------------------+" << std::endl;
    std::cout << "선택: ";
}

void displayExamples() {
    std::cout << "\n예제 스크립트:" << std::endl;
    for (int i = 0; i < ScriptParser::getExampleCount(); i++) {
        std::cout << "  " << (i + 1) << ". ";

        // 예제 제목만 추출
        std::string script = ScriptParser::getExampleScript(i);
        size_t newline = script.find('\n');
        if (newline != std::string::npos) {
            std::cout << script.substr(3, newline - 3);  // "// " 제거
        }
        std::cout << std::endl;
    }
    std::cout << "  0. 돌아가기" << std::endl;
    std::cout << "선택: ";
}

void runExample(int index, MemoryManager& memManager, ScriptParser& parser, Visualizer& visualizer) {
    parser.reset();
    memManager.reset();

    std::string script = ScriptParser::getExampleScript(index);

    std::cout << "\n" << "=== 스크립트 ===" << std::endl;
    std::cout << script << std::endl;
    std::cout << "=================" << std::endl;

    std::cout << "\n[DEBUG] 스크립트 실행 시작..." << std::endl;

    bool result = parser.executeScript(script);

    if (!result) {
        std::cout << "\n[ERROR] 스크립트 실행 실패!" << std::endl;
        std::cout << "오류: " << parser.getLastError() << std::endl;
    }
    else {
        std::cout << "[DEBUG] 스크립트 실행 성공!" << std::endl;
    }

    std::cout << "[DEBUG] 메모리 블록 수: " << memManager.getMemoryBlocks().size() << std::endl;
    std::cout << "[DEBUG] 이벤트 수: " << memManager.getEvents().size() << std::endl;

    std::cout << "\n아무 키나 누르면 메모리 상태를 확인합니다...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    visualizer.printMemoryState(memManager);

    std::cout << "\n아무 키나 누르면 계속...";
    std::cin.get();
}

void runCustomCode(MemoryManager& memManager, ScriptParser& parser, Visualizer& visualizer) {
    parser.reset();
    memManager.reset();

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "코드를 입력하세요 (입력 완료 후 빈 줄에서 Enter):" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "\n예시:\n";
    std::cout << "int main() {\n";
    std::cout << "    int x = 5;\n";
    std::cout << "    int* ptr = new int;\n";
    std::cout << "    delete ptr;\n";
    std::cout << "    return 0;\n";
    std::cout << "}\n" << std::endl;
    std::cout << "입력 시작 (빈 줄에서 Enter로 종료):\n" << std::endl;

    std::string code;
    std::string line;

    // 입력 버퍼 비우기
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    int lineCount = 0;
    while (true) {
        std::cout << (lineCount + 1) << ": ";
        std::getline(std::cin, line);

        if (line.empty()) {
            std::cout << "[DEBUG] 입력 종료 (총 " << lineCount << "줄)" << std::endl;
            break;
        }

        code += line + "\n";
        lineCount++;
    }

    if (code.empty()) {
        std::cout << "\n[ERROR] 코드가 입력되지 않았습니다." << std::endl;
        std::cout << "아무 키나 누르면 계속...";
        std::cin.get();
        return;
    }

    std::cout << "\n[DEBUG] 입력된 코드:\n" << code << std::endl;
    std::cout << "\n[DEBUG] 실행 중..." << std::endl;

    bool result = parser.executeScript(code);

    if (!result) {
        std::cout << "\n[ERROR] 오류 발생: " << parser.getLastError() << std::endl;
    }
    else {
        std::cout << "[DEBUG] 실행 성공!" << std::endl;
    }

    std::cout << "[DEBUG] 메모리 블록 수: " << memManager.getMemoryBlocks().size() << std::endl;

    visualizer.printMemoryState(memManager);

    std::cout << "\n아무 키나 누르면 계속...";
    std::cin.get();
}

void checkMemoryLeaks(const MemoryManager& memManager, const Visualizer& visualizer) {
    auto leaks = memManager.detectLeaks();

    std::cout << "\n메모리 누수 검사 결과:" << std::endl;
    visualizer.printSeparator('-', 70);

    if (leaks.empty()) {
        std::cout << "[OK] 메모리 누수가 없습니다!" << std::endl;
    }
    else {
        const_cast<Visualizer&>(visualizer).printLeakWarnings(leaks, memManager);
    }

    visualizer.printSeparator('-', 70);

    std::cout << "\n아무 키나 누르면 계속...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void runSimpleTest(MemoryManager& memManager, Visualizer& visualizer) {
    std::cout << "\n=== 간단 테스트 모드 ===" << std::endl;
    std::cout << "파서 없이 직접 메모리를 생성하여 기본 기능을 테스트합니다.\n" << std::endl;

    memManager.reset();

    std::cout << "[TEST 1] 스택 변수 생성..." << std::endl;
    int id1 = memManager.createStackVariable("test_int", sizeof(int));
    std::cout << "  생성됨: ID=" << id1 << std::endl;

    std::cout << "[TEST 2] 스택 포인터 변수 생성..." << std::endl;
    int id2 = memManager.createStackVariable("test_ptr", sizeof(void*));
    MemoryBlock* ptrBlock = memManager.findBlock(id2);
    if (ptrBlock) {
        ptrBlock->isPointer = true;
        ptrBlock->pointerType = PointerType::RAW;
        ptrBlock->pointsTo = -1;
        std::cout << "  생성됨: ID=" << id2 << std::endl;
    }

    std::cout << "[TEST 3] 힙 메모리 할당..." << std::endl;
    int id3 = memManager.allocateHeap("heap_data", sizeof(int), PointerType::RAW);
    std::cout << "  할당됨: ID=" << id3 << std::endl;

    std::cout << "[TEST 4] 포인터 연결..." << std::endl;
    memManager.assignPointer(id2, id3);
    std::cout << "  연결됨: ptr -> heap_data" << std::endl;

    std::cout << "\n현재 메모리 블록 수: " << memManager.getMemoryBlocks().size() << std::endl;
    std::cout << "현재 이벤트 수: " << memManager.getEvents().size() << std::endl;

    std::cout << "\n아무 키나 누르면 메모리 상태를 확인합니다...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    visualizer.printMemoryState(memManager);

    std::cout << "\n아무 키나 누르면 계속...";
    std::cin.get();
}

int main() {
    MemoryManager memManager;
    Visualizer visualizer;
    ScriptParser parser(memManager);

    std::cout << "\033[1;36m";  // 청록색 볼드
    std::cout << R"(
===============================================================
                                                               
   C++ Memory Visualizer - Console Edition                    
   메모리 관리 시각화 도구 - 콘솔 버전                        
                                                               
   순수 C++만으로 제작 (외부 라이브러리 없음)                 
                                                               
===============================================================
)" << std::endl;
    std::cout << "\033[0m";  // 색상 리셋

    std::cout << "\n아무 키나 누르면 시작합니다...";
    std::cin.get();

    bool running = true;

    while (running) {
        visualizer.printMemoryState(memManager);
        displayMenu();

        int choice;
        std::cin >> choice;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "잘못된 입력입니다." << std::endl;
            continue;
        }

        switch (choice) {
        case 1: {
            // 예제 실행
            displayExamples();
            int exampleChoice;
            std::cin >> exampleChoice;

            if (exampleChoice > 0 && exampleChoice <= ScriptParser::getExampleCount()) {
                runExample(exampleChoice - 1, memManager, parser, visualizer);
            }
            break;
        }

        case 2: {
            // 직접 코드 입력
            runCustomCode(memManager, parser, visualizer);
            break;
        }

        case 3: {
            // 메모리 누수 검사
            checkMemoryLeaks(memManager, visualizer);
            break;
        }

        case 4: {
            // 초기화
            memManager.reset();
            parser.reset();
            std::cout << "\n초기화되었습니다." << std::endl;
            std::cout << "아무 키나 누르면 계속...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
            break;
        }

        case 9: {
            // 간단 테스트
            runSimpleTest(memManager, visualizer);
            break;
        }

        case 0: {
            // 종료
            running = false;
            std::cout << "\n프로그램을 종료합니다." << std::endl;
            break;
        }

        default:
            std::cout << "잘못된 선택입니다." << std::endl;
            std::cout << "아무 키나 누르면 계속...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
            break;
        }
    }

    return 0;
}

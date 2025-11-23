#include "Visualizer.h"
#include <algorithm>
#include <sstream>

Visualizer::Visualizer() {
    // ANSI 색상 코드
    colorReset = "\033[0m";
    colorRed = "\033[31m";
    colorGreen = "\033[32m";
    colorYellow = "\033[33m";
    colorBlue = "\033[34m";
    colorMagenta = "\033[35m";
    colorCyan = "\033[36m";
    colorBold = "\033[1m";
}

Visualizer::~Visualizer() {
}

void Visualizer::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void Visualizer::printSeparator(char ch, int length) const {
    std::cout << std::string(length, ch) << std::endl;
}

void Visualizer::printMemoryState(const MemoryManager& memManager) {
    clearScreen();

    std::cout << colorBold << colorCyan;
    printSeparator('=', 70);
    std::cout << "       C++ 메모리 관리 시각화 도구 - 콘솔 버전" << std::endl;
    printSeparator('=', 70);
    std::cout << colorReset << std::endl;

    const auto& blocks = memManager.getMemoryBlocks();

    // 메모리 누수 확인
    auto leaks = memManager.detectLeaks();
    if (!leaks.empty()) {
        printLeakWarnings(leaks, memManager);
        std::cout << std::endl;
    }

    // 스택 영역
    std::cout << colorBold << colorBlue << "┌─ STACK 메모리 ─────────────────┐" << colorReset << std::endl;
    printStack(blocks);
    std::cout << colorBlue << "└─────────────────────────────────┘" << colorReset << std::endl;
    std::cout << std::endl;

    // 힙 영역
    std::cout << colorBold << colorRed << "┌─ HEAP 메모리 ──────────────────┐" << colorReset << std::endl;
    printHeap(blocks);
    std::cout << colorRed << "└─────────────────────────────────┘" << colorReset << std::endl;
    std::cout << std::endl;

    // 포인터 연결
    std::cout << colorBold << colorYellow << "포인터 연결:" << colorReset << std::endl;
    printPointerConnections(blocks);
    std::cout << std::endl;

    // 이벤트 로그
    std::cout << colorBold << colorGreen << "최근 이벤트:" << colorReset << std::endl;
    printEventLog(memManager.getEvents(), 5);
    std::cout << std::endl;

    printSeparator('-', 70);
}

void Visualizer::printStack(const std::vector<MemoryBlock>& blocks) {
    bool hasStack = false;

    for (const auto& block : blocks) {
        if (block.type == MemoryType::STACK && block.isAllocated) {
            hasStack = true;
            std::cout << "│ ";
            printMemoryBlock(block);
            std::cout << std::endl;
        }
    }

    if (!hasStack) {
        std::cout << "│ (비어있음)" << std::endl;
    }
}

void Visualizer::printHeap(const std::vector<MemoryBlock>& blocks) {
    bool hasHeap = false;

    for (const auto& block : blocks) {
        if ((block.type == MemoryType::HEAP || block.type == MemoryType::SMART_PTR)
            && block.isAllocated) {
            hasHeap = true;
            std::cout << "│ ";
            printMemoryBlock(block);
            std::cout << std::endl;
        }
    }

    if (!hasHeap) {
        std::cout << "│ (비어있음)" << std::endl;
    }
}

void Visualizer::printMemoryBlock(const MemoryBlock& block) const {
    std::string color = getBlockColor(block);

    std::cout << color;

    // 이름
    std::cout << std::setw(15) << std::left << block.name;

    // 타입 정보
    if (block.isObject && block.classInfo) {
        std::cout << " <" << std::setw(10) << std::left << block.classInfo->className << ">";
    }
    else if (block.isPointer) {
        std::cout << " [" << std::setw(10) << std::left << getPointerTypeString(block.pointerType) << "]";
    }
    else {
        std::cout << std::setw(13) << " ";
    }

    // 크기
    std::cout << " " << std::setw(4) << std::right << block.size << "B";

    // 주소
    std::cout << " @0x" << std::hex << reinterpret_cast<uintptr_t>(block.address) << std::dec;

    // shared_ptr 참조 카운트
    if (block.pointerType == PointerType::SHARED && block.refCount > 0) {
        std::cout << " (refs:" << block.refCount << ")";
    }

    std::cout << colorReset;
}

void Visualizer::printPointerConnections(const std::vector<MemoryBlock>& blocks) {
    bool hasConnections = false;

    for (const auto& block : blocks) {
        if (block.isPointer && block.isAllocated && block.pointsTo != -1) {
            hasConnections = true;

            // 타겟 블록 찾기
            const MemoryBlock* target = nullptr;
            for (const auto& b : blocks) {
                if (b.id == block.pointsTo) {
                    target = &b;
                    break;
                }
            }

            std::cout << "  " << colorYellow << block.name << colorReset;
            std::cout << " ──> ";

            if (target) {
                std::string targetColor = getBlockColor(*target);
                std::cout << targetColor << target->name << colorReset;

                if (target->isObject && target->classInfo) {
                    std::cout << " <" << target->classInfo->className << ">";
                }
            }
            else {
                std::cout << colorRed << "(dangling)" << colorReset;
            }

            std::cout << std::endl;
        }
    }

    if (!hasConnections) {
        std::cout << "  (포인터 연결 없음)" << std::endl;
    }
}

void Visualizer::printEventLog(const std::vector<MemoryEvent>& events, int count) {
    int displayed = 0;

    for (auto it = events.rbegin(); it != events.rend() && displayed < count; ++it) {
        const auto& event = *it;
        displayed++;

        std::string eventColor;
        std::string eventType;

        switch (event.type) {
        case MemoryEvent::EventType::ALLOCATE:
            eventColor = colorGreen;
            eventType = "[ALLOC]    ";
            break;
        case MemoryEvent::EventType::DEALLOCATE:
            eventColor = colorRed;
            eventType = "[FREE]     ";
            break;
        case MemoryEvent::EventType::ASSIGN:
            eventColor = colorYellow;
            eventType = "[ASSIGN]   ";
            break;
        case MemoryEvent::EventType::COPY:
            eventColor = colorBlue;
            eventType = "[COPY]     ";
            break;
        case MemoryEvent::EventType::MOVE:
            eventColor = colorMagenta;
            eventType = "[MOVE]     ";
            break;
        case MemoryEvent::EventType::CONSTRUCT:
            eventColor = colorCyan;
            eventType = "[CONSTRUCT]";
            break;
        case MemoryEvent::EventType::DESTRUCT:
            eventColor = colorYellow;
            eventType = "[DESTRUCT] ";
            break;
        case MemoryEvent::EventType::LEAK:
            eventColor = colorRed + colorBold;
            eventType = "[LEAK!]    ";
            break;
        }

        std::cout << "  " << eventColor << eventType << colorReset;
        std::cout << " " << event.description << std::endl;
    }

    if (events.empty()) {
        std::cout << "  (이벤트 없음)" << std::endl;
    }
}

void Visualizer::printLeakWarnings(const std::vector<int>& leakIds, const MemoryManager& memManager) {
    std::cout << colorBold << colorRed;
    std::cout << "⚠ 메모리 누수 감지! " << leakIds.size() << "개 블록" << colorReset << std::endl;

    for (int id : leakIds) {
        const MemoryBlock* block = memManager.findBlock(id);
        if (block) {
            std::cout << "  - " << colorRed << block->name
                << " (" << block->size << " bytes, @0x"
                << std::hex << reinterpret_cast<uintptr_t>(block->address) << std::dec << ")"
                << colorReset << std::endl;
        }
    }
}

std::string Visualizer::getBlockColor(const MemoryBlock& block) const {
    if (!block.isAllocated) {
        return colorReset;
    }

    if (block.isObject) {
        if (block.type == MemoryType::STACK) {
            return colorCyan;  // 스택 객체
        }
        else {
            return colorMagenta;  // 힙 객체
        }
    }

    if (block.isPointer) {
        if (block.pointerType == PointerType::RAW) {
            return colorYellow;
        }
        else if (block.pointerType == PointerType::UNIQUE) {
            return colorGreen;
        }
        else {
            return colorBlue;
        }
    }

    if (block.type == MemoryType::STACK) {
        return colorBlue;
    }
    else if (block.type == MemoryType::HEAP) {
        return colorRed;
    }
    else {
        return colorMagenta;
    }
}

std::string Visualizer::getPointerTypeString(PointerType type) const {
    switch (type) {
    case PointerType::RAW: return "raw*";
    case PointerType::UNIQUE: return "unique_ptr";
    case PointerType::SHARED: return "shared_ptr";
    default: return "unknown";
    }
}

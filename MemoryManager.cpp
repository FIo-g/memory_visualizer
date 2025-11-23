#include "MemoryManager.h"
#include <algorithm>
#include <cstdlib>

MemoryManager::MemoryManager()
    : nextId(1), stackDepth(0), currentTime(0.0f) {
}

MemoryManager::~MemoryManager() {
}

// 스택 변수 생성
int MemoryManager::createStackVariable(const std::string& name, size_t size) {
    MemoryBlock block;
    block.id = nextId++;
    block.name = name;
    block.size = size;
    block.type = MemoryType::STACK;
    block.address = reinterpret_cast<void*>(static_cast<uintptr_t>(0x7FFF0000 + stackDepth * 8));  // 시뮬레이션 주소
    block.isAllocated = true;
    block.lifetime = 0;

    // 스택 위치 설정 (위에서 아래로 쌓임)
    block.x = 100.0f;
    block.y = 100.0f + stackDepth * 70.0f;
    block.targetX = block.x;
    block.targetY = block.y;

    blocks.push_back(block);
    stackDepth++;

    addEvent(MemoryEvent::EventType::ALLOCATE, block.id,
        "스택 변수 생성: " + name + " (" + std::to_string(size) + " bytes)");

    return block.id;
}

// 클래스 객체 생성 (스택)
int MemoryManager::createClassObject(const std::string& name, ClassInfo* classInfo) {
    if (!classInfo) return -1;

    MemoryBlock block;
    block.id = nextId++;
    block.name = name;
    block.size = classInfo->totalSize;
    block.type = MemoryType::STACK;
    block.address = reinterpret_cast<void*>(static_cast<uintptr_t>(0x7FFF0000 + stackDepth * 8));
    block.isAllocated = true;
    block.lifetime = 0;
    block.isObject = true;
    block.classInfo = classInfo;
    block.isExpanded = false;

    // 스택 위치 설정
    block.x = 100.0f;
    block.y = 100.0f + stackDepth * 70.0f;
    block.targetX = block.x;
    block.targetY = block.y;

    blocks.push_back(block);
    stackDepth++;

    addEvent(MemoryEvent::EventType::CONSTRUCT, block.id,
        "객체 생성: " + name + " (클래스: " + classInfo->className + ", " +
        std::to_string(classInfo->totalSize) + " bytes)");

    return block.id;
}

// 힙 메모리 할당
int MemoryManager::allocateHeap(const std::string& name, size_t size, PointerType ptrType) {
    MemoryBlock block;
    block.id = nextId++;
    block.name = name;
    block.size = size;
    block.type = (ptrType == PointerType::RAW) ? MemoryType::HEAP : MemoryType::SMART_PTR;
    block.address = reinterpret_cast<void*>(static_cast<uintptr_t>(rand() % 0x10000000 + 0x10000000));  // 시뮬레이션 주소
    block.isAllocated = true;
    block.lifetime = 0;
    block.isPointer = false;  // 이건 힙에 할당된 실제 데이터
    block.pointerType = ptrType;
    block.refCount = (ptrType == PointerType::SHARED) ? 1 : 0;

    // 힙 위치 설정
    int heapCount = 0;
    for (const auto& b : blocks) {
        if (b.type == MemoryType::HEAP || b.type == MemoryType::SMART_PTR) {
            heapCount++;
        }
    }

    block.x = 500.0f + (heapCount % 3) * 120.0f;
    block.y = 100.0f + (heapCount / 3) * 70.0f;
    block.targetX = block.x;
    block.targetY = block.y;

    blocks.push_back(block);

    std::string ptrTypeStr = (ptrType == PointerType::RAW) ? "raw pointer" :
        (ptrType == PointerType::UNIQUE) ? "unique_ptr" : "shared_ptr";
    addEvent(MemoryEvent::EventType::ALLOCATE, block.id,
        "힙 메모리 할당: " + name + " (" + ptrTypeStr + ", " + std::to_string(size) + " bytes)");

    return block.id;
}

// 클래스 객체 할당 (힙)
int MemoryManager::allocateClassObject(const std::string& name, ClassInfo* classInfo, PointerType ptrType) {
    if (!classInfo) return -1;

    MemoryBlock block;
    block.id = nextId++;
    block.name = name;
    block.size = classInfo->totalSize;
    block.type = (ptrType == PointerType::RAW) ? MemoryType::HEAP : MemoryType::SMART_PTR;
    block.address = reinterpret_cast<void*>(static_cast<uintptr_t>(rand() % 0x10000000 + 0x10000000));
    block.isAllocated = true;
    block.lifetime = 0;
    block.isPointer = false;
    block.pointerType = ptrType;
    block.refCount = (ptrType == PointerType::SHARED) ? 1 : 0;
    block.isObject = true;
    block.classInfo = classInfo;
    block.isExpanded = false;

    // 힙 위치 설정
    int heapCount = 0;
    for (const auto& b : blocks) {
        if (b.type == MemoryType::HEAP || b.type == MemoryType::SMART_PTR) {
            heapCount++;
        }
    }

    block.x = 500.0f + (heapCount % 3) * 120.0f;
    block.y = 100.0f + (heapCount / 3) * 70.0f;
    block.targetX = block.x;
    block.targetY = block.y;

    blocks.push_back(block);

    std::string ptrTypeStr = (ptrType == PointerType::RAW) ? "raw pointer" :
        (ptrType == PointerType::UNIQUE) ? "unique_ptr" : "shared_ptr";
    addEvent(MemoryEvent::EventType::CONSTRUCT, block.id,
        "힙에 객체 할당: " + name + " (클래스: " + classInfo->className + ", " +
        ptrTypeStr + ", " + std::to_string(classInfo->totalSize) + " bytes)");

    return block.id;
}

// 메모리 해제
bool MemoryManager::deallocate(int blockId) {
    MemoryBlock* block = findBlock(blockId);
    if (!block || !block->isAllocated) {
        return false;
    }

    // shared_ptr의 경우 참조 카운트 확인
    if (block->pointerType == PointerType::SHARED && block->refCount > 1) {
        decreaseRefCount(blockId);
        return true;
    }

    block->isAllocated = false;
    addEvent(MemoryEvent::EventType::DEALLOCATE, blockId,
        "메모리 해제: " + block->name);

    // 이 블록을 가리키는 포인터들 처리
    for (auto& b : blocks) {
        if (b.isPointer && b.pointsTo == blockId) {
            b.pointsTo = -1;  // dangling pointer
        }
    }

    return true;
}

// 포인터 연결
bool MemoryManager::assignPointer(int pointerBlockId, int targetBlockId) {
    MemoryBlock* ptrBlock = findBlock(pointerBlockId);
    if (!ptrBlock) {
        return false;
    }

    // 기존에 가리키던 shared_ptr 참조 카운트 감소
    if (ptrBlock->pointerType == PointerType::SHARED && ptrBlock->pointsTo != -1) {
        decreaseRefCount(ptrBlock->pointsTo);
    }

    ptrBlock->pointsTo = targetBlockId;

    if (targetBlockId != -1) {
        MemoryBlock* targetBlock = findBlock(targetBlockId);
        if (targetBlock && ptrBlock->pointerType == PointerType::SHARED) {
            targetBlock->refCount++;
        }

        addEvent(MemoryEvent::EventType::ASSIGN, pointerBlockId,
            "포인터 할당: " + ptrBlock->name + " -> " +
            (targetBlock ? targetBlock->name : "null"));
    }
    else {
        addEvent(MemoryEvent::EventType::ASSIGN, pointerBlockId,
            "포인터를 nullptr로 설정: " + ptrBlock->name);
    }

    return true;
}

// shared_ptr 복사
int MemoryManager::copySharedPtr(int sourceBlockId, const std::string& newName) {
    MemoryBlock* sourceBlock = findBlock(sourceBlockId);
    if (!sourceBlock || !sourceBlock->isPointer || sourceBlock->pointerType != PointerType::SHARED) {
        return -1;
    }

    // 새 shared_ptr 생성
    MemoryBlock newBlock;
    newBlock.id = nextId++;
    newBlock.name = newName;
    newBlock.size = sizeof(void*);
    newBlock.type = MemoryType::STACK;
    newBlock.address = reinterpret_cast<void*>(static_cast<uintptr_t>(0x7FFF0000 + stackDepth * 8));
    newBlock.isAllocated = true;
    newBlock.isPointer = true;
    newBlock.pointerType = PointerType::SHARED;
    newBlock.pointsTo = sourceBlock->pointsTo;

    newBlock.x = 100.0f;
    newBlock.y = 100.0f + stackDepth * 70.0f;
    newBlock.targetX = newBlock.x;
    newBlock.targetY = newBlock.y;

    blocks.push_back(newBlock);
    stackDepth++;

    // 참조 카운트 증가
    if (sourceBlock->pointsTo != -1) {
        MemoryBlock* targetBlock = findBlock(sourceBlock->pointsTo);
        if (targetBlock) {
            targetBlock->refCount++;
            addEvent(MemoryEvent::EventType::COPY, newBlock.id,
                "shared_ptr 복사: " + newName + " (참조 카운트: " + std::to_string(targetBlock->refCount) + ")");
        }
    }

    return newBlock.id;
}

// unique_ptr 이동
bool MemoryManager::moveUniquePtr(int sourceBlockId, int targetBlockId) {
    MemoryBlock* sourceBlock = findBlock(sourceBlockId);
    MemoryBlock* targetBlock = findBlock(targetBlockId);

    if (!sourceBlock || !targetBlock ||
        sourceBlock->pointerType != PointerType::UNIQUE ||
        targetBlock->pointerType != PointerType::UNIQUE) {
        return false;
    }

    // 소유권 이동
    targetBlock->pointsTo = sourceBlock->pointsTo;
    sourceBlock->pointsTo = -1;

    addEvent(MemoryEvent::EventType::MOVE, targetBlockId,
        "unique_ptr 이동: " + sourceBlock->name + " -> " + targetBlock->name);

    return true;
}

// 스코프 종료
void MemoryManager::endScope() {
    if (stackDepth == 0) return;

    // 가장 최근 스택 변수 제거
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        if (it->type == MemoryType::STACK && it->isAllocated) {
            it->isAllocated = false;
            stackDepth--;

            // 스마트 포인터면 자동으로 힙 메모리도 해제
            if (it->isPointer && it->pointsTo != -1) {
                if (it->pointerType == PointerType::UNIQUE) {
                    deallocate(it->pointsTo);
                }
                else if (it->pointerType == PointerType::SHARED) {
                    decreaseRefCount(it->pointsTo);
                }
            }

            addEvent(MemoryEvent::EventType::DEALLOCATE, it->id,
                "스코프 종료로 변수 해제: " + it->name);
            break;
        }
    }
}

// 메모리 누수 검사
std::vector<int> MemoryManager::detectLeaks() const {
    std::vector<int> leaks;

    for (const auto& block : blocks) {
        // 힙 메모리가 할당되어 있는데 가리키는 포인터가 없으면 누수
        if ((block.type == MemoryType::HEAP || block.type == MemoryType::SMART_PTR) &&
            block.isAllocated) {

            bool hasPointer = false;
            for (const auto& ptr : blocks) {
                if (ptr.isPointer && ptr.isAllocated && ptr.pointsTo == block.id) {
                    hasPointer = true;
                    break;
                }
            }

            if (!hasPointer && block.pointerType == PointerType::RAW) {
                leaks.push_back(block.id);
            }
        }
    }

    return leaks;
}

// 블록 찾기
MemoryBlock* MemoryManager::findBlock(int id) {
    for (auto& block : blocks) {
        if (block.id == id) {
            return &block;
        }
    }
    return nullptr;
}

const MemoryBlock* MemoryManager::findBlock(int id) const {
    for (const auto& block : blocks) {
        if (block.id == id) {
            return &block;
        }
    }
    return nullptr;
}

// 초기화
void MemoryManager::reset() {
    blocks.clear();
    events.clear();
    nextId = 1;
    stackDepth = 0;
    currentTime = 0.0f;
}

// 업데이트
void MemoryManager::update(float deltaTime) {
    currentTime += deltaTime;

    // 애니메이션 업데이트
    for (auto& block : blocks) {
        if (block.isAllocated) {
            block.lifetime++;

            // 부드러운 이동
            float lerpSpeed = 5.0f * deltaTime;
            block.x += (block.targetX - block.x) * lerpSpeed;
            block.y += (block.targetY - block.y) * lerpSpeed;
        }
    }
}

// 이벤트 추가
void MemoryManager::addEvent(MemoryEvent::EventType type, int blockId, const std::string& description) {
    events.emplace_back(type, blockId, description, currentTime);

    // 최대 100개 이벤트만 유지
    if (events.size() > 100) {
        events.erase(events.begin());
    }
}

// shared_ptr 참조 카운트 감소
void MemoryManager::decreaseRefCount(int blockId) {
    MemoryBlock* block = findBlock(blockId);
    if (block && block->refCount > 0) {
        block->refCount--;

        addEvent(MemoryEvent::EventType::ASSIGN, blockId,
            "참조 카운트 감소: " + block->name + " (현재: " + std::to_string(block->refCount) + ")");

        // 참조 카운트가 0이 되면 자동 해제
        if (block->refCount == 0) {
            deallocate(blockId);
        }
    }
}

#pragma once
#include "MemoryBlock.h"
#include <vector>
#include <unordered_map>
#include <string>

// 메모리 관리 시뮬레이터
class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    // 스택 변수 생성
    int createStackVariable(const std::string& name, size_t size);

    // 클래스 객체 생성 (스택)
    int createClassObject(const std::string& name, ClassInfo* classInfo);

    // 힙 메모리 할당
    int allocateHeap(const std::string& name, size_t size, PointerType ptrType = PointerType::RAW);

    // 클래스 객체 할당 (힙)
    int allocateClassObject(const std::string& name, ClassInfo* classInfo, PointerType ptrType = PointerType::RAW);

    // 메모리 해제
    bool deallocate(int blockId);

    // 포인터 연결
    bool assignPointer(int pointerBlockId, int targetBlockId);

    // shared_ptr 복사 (참조 카운트 증가)
    int copySharedPtr(int sourceBlockId, const std::string& newName);

    // unique_ptr 이동
    bool moveUniquePtr(int sourceBlockId, int targetBlockId);

    // 스코프 종료 시뮬레이션 (스택 변수 정리)
    void endScope();

    // 메모리 누수 검사
    std::vector<int> detectLeaks() const;

    // 모든 메모리 블록 가져오기
    const std::vector<MemoryBlock>& getMemoryBlocks() const { return blocks; }

    // 이벤트 히스토리 가져오기
    const std::vector<MemoryEvent>& getEvents() const { return events; }

    // 특정 블록 찾기
    MemoryBlock* findBlock(int id);
    const MemoryBlock* findBlock(int id) const;

    // 초기화
    void reset();

    // 업데이트 (애니메이션용)
    void update(float deltaTime);

private:
    std::vector<MemoryBlock> blocks;        // 모든 메모리 블록
    std::vector<MemoryEvent> events;        // 이벤트 히스토리
    int nextId;                             // 다음 블록 ID
    int stackDepth;                         // 현재 스택 깊이
    float currentTime;                      // 현재 시간

    // 이벤트 추가
    void addEvent(MemoryEvent::EventType type, int blockId, const std::string& description);

    // shared_ptr 참조 카운트 감소
    void decreaseRefCount(int blockId);
};

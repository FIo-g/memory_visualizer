#pragma once
#include <string>
#include <memory>
#include <vector>

// 메모리 블록 타입
enum class MemoryType {
    STACK,      // 스택 메모리
    HEAP,       // 힙 메모리 (new로 할당)
    SMART_PTR   // 스마트 포인터로 관리되는 힙 메모리
};

// 포인터 타입
enum class PointerType {
    RAW,        // raw pointer
    UNIQUE,     // unique_ptr
    SHARED      // shared_ptr
};

// 클래스 정보
class ClassInfo {
public:
    std::string className;                   // 클래스 이름
    std::vector<std::string> memberNames;    // 멤버 변수 이름들
    std::vector<std::string> memberTypes;    // 멤버 변수 타입들
    std::vector<size_t> memberSizes;         // 멤버 변수 크기들
    size_t totalSize;                        // 전체 크기
    bool hasConstructor;                     // 생성자 유무
    bool hasDestructor;                      // 소멸자 유무

    ClassInfo()
        : className(""), totalSize(0),
        hasConstructor(false), hasDestructor(false) {
    }
};

// 메모리 블록 클래스
class MemoryBlock {
public:
    int id;                     // 고유 식별자
    std::string name;           // 변수 이름
    size_t size;                // 크기 (바이트)
    MemoryType type;            // 메모리 타입
    void* address;              // 시뮬레이션 주소
    bool isAllocated;           // 할당 여부
    int lifetime;               // 생존 시간 (애니메이션용)

    // 포인터 관련
    bool isPointer;             // 포인터 여부
    PointerType pointerType;    // 포인터 타입
    int pointsTo;               // 가리키는 블록의 id (-1이면 nullptr)
    int refCount;               // shared_ptr의 참조 카운트

    // 클래스 객체 관련
    bool isObject;              // 클래스 객체인지
    ClassInfo* classInfo;       // 클래스 정보 (nullptr이면 기본 타입)
    bool isExpanded;            // 멤버 변수 펼쳐보기

    // 시각화 관련
    float x, y;                 // 화면 상의 위치
    float targetX, targetY;     // 목표 위치 (애니메이션)
    bool isHighlighted;         // 강조 표시 여부

    MemoryBlock()
        : id(-1), name(""), size(0), type(MemoryType::STACK),
        address(nullptr), isAllocated(false), lifetime(0),
        isPointer(false), pointerType(PointerType::RAW),
        pointsTo(-1), refCount(0),
        isObject(false), classInfo(nullptr), isExpanded(false),
        x(0), y(0), targetX(0), targetY(0), isHighlighted(false) {
    }
};

// 메모리 이벤트 (히스토리 추적용)
class MemoryEvent {
public:
    enum class EventType {
        ALLOCATE,       // 메모리 할당
        DEALLOCATE,     // 메모리 해제
        ASSIGN,         // 포인터 할당
        COPY,           // 복사
        MOVE,           // 이동
        LEAK,           // 메모리 누수 감지
        CONSTRUCT,      // 객체 생성
        DESTRUCT        // 객체 소멸
    };

    EventType type;
    int blockId;
    std::string description;
    float timestamp;

    MemoryEvent(EventType t, int id, const std::string& desc, float time)
        : type(t), blockId(id), description(desc), timestamp(time) {
    }
};

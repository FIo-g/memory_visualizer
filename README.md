# memory_visualizer
C++ 콘솔 창을 이용해서 포인터, 동적할당, 변수 등이 힙 메모리와 스택 메모리에 어떻게 저장되는지 알 수 있는 프로그램

# C++ 메모리 시각화 도구 - 순수 C++ 콘솔 버전

## 🎯 특징

### ✨ 외부 라이브러리 완전 제거!
- ❌ ImGui 필요 없음
- ❌ SDL2 필요 없음
- ❌ SFML 필요 없음
- ✅ **순수 C++ 표준 라이브러리만 사용**
- ✅ iostream, string, vector만으로 구현

### 🖥️ 크로스 플랫폼
- ✅ Windows
- ✅ Linux
- ✅ macOS
- ✅ 어디서든 바로 빌드 가능!

### 🎨 ASCII 아트 시각화
- 색상 코드 (ANSI)로 구분
- 텍스트 기반 메모리 레이아웃
- 포인터 연결 표시
- 이벤트 로그

---

## 📸 스크린샷 (텍스트)

```
═══════════════════════════════════════════════════════════════
       C++ 메모리 관리 시각화 도구 - 콘솔 버전
═══════════════════════════════════════════════════════════════

┌─ STACK 메모리 ─────────────────┐
│ score          int              4B @0x7fff0000
│ p1             <Point>           8B @0x7fff0008
│ player         <Player>         16B @0x7fff0010
│ ptr            [raw*]            8B @0x7fff0020
│ enemy          [unique_ptr]      8B @0x7fff0028
│ spawn          [shared_ptr]      8B @0x7fff0030 (refs:2)
└─────────────────────────────────┘

┌─ HEAP 메모리 ──────────────────┐
│ int_heap                         4B @0x10000000
│ Point_heap     <Point>           8B @0x10000004
│ Player         <Player>         16B @0x1000000c
└─────────────────────────────────┘

포인터 연결:
  ptr ──> int_heap
  enemy ──> Player <Player>
  spawn ──> Point_heap <Point>

최근 이벤트:
  [CONSTRUCT] 객체 생성: p1 (클래스: Point, 8 bytes)
  [CONSTRUCT] 객체 생성: player (클래스: Player, 16 bytes)
  [ALLOC]     힙 메모리 할당: int_heap (raw pointer, 4 bytes)
  [CONSTRUCT] 힙에 객체 할당: Player (unique_ptr, 16 bytes)
  [COPY]      shared_ptr 복사: spawn2 (참조 카운트: 2)

──────────────────────────────────────────────────────────────

┌─ 메뉴 ──────────────────────────┐
│ 1. 예제 스크립트 실행          │
│ 2. 직접 코드 입력              │
│ 3. 메모리 누수 검사            │
│ 4. 초기화                      │
│ 5. 종료                        │
└──────────────────────────────────┘
선택: 
```

---

## 🎯 사용 방법

### 1. 프로그램 실행

### 2. 메뉴 선택

#### 옵션 1: 예제 스크립트 실행
- 10가지 예제 중 선택
- 자동으로 실행 후 메모리 상태 표시

#### 옵션 2: 직접 코드 입력
```
코드를 입력하세요 (빈 줄 입력 시 종료):
─────────────────────────────────
class Point {
public:
    int x;
    int y;
};

int main() {
    Point p1;
    Point* p2 = new Point();
    delete p2;
    return 0;
}

(빈 줄 입력)
```

#### 옵션 3: 메모리 누수 검사
- 현재 메모리 상태에서 누수 탐지
- 접근 불가능한 힙 메모리 표시

#### 옵션 4: 초기화
- 모든 메모리 블록 제거
- 처음부터 다시 시작

#### 옵션 5: 종료

---

## 📚 지원하는 C++ 코드

### 완전히 동일한 기능!
```cpp
// 클래스 정의
class Point {
public:
    int x;
    int y;
};

// main 함수
int main() {
    // 기본 변수
    int score = 100;
    
    // 스택 객체
    Point p1;
    
    // Raw pointer
    int* ptr = new int;
    Point* p2 = new Point();
    
    // 스마트 포인터
    unique_ptr<Point> p3 = make_unique<Point>();
    shared_ptr<Point> p4 = make_shared<Point>();
    
    // 메모리 해제
    delete ptr;
    delete p2;
    
    return 0;
}
```

---

## 🎨 색상 코드

| 색상 | 의미 |
|------|------|
| 🔵 파란색 | 스택 메모리, 스택 변수 |
| 🔴 빨간색 | 힙 메모리, raw pointer 해제 |
| 🟡 노란색 | raw pointer 변수 |
| 🟢 초록색 | unique_ptr, 메모리 할당 |
| 🟣 보라색 | shared_ptr, 스마트 포인터 관리 |
| 🔵 청록색 | 스택 객체, 생성자 |

---

## 🏆 장점

### ImGui 버전 대비
| 항목 | ImGui 버전 | 콘솔 버전 |
|------|------------|-----------|
| 외부 라이브러리 | ImGui, SDL2 필요 | 없음! |
| 설치 | 복잡 | 매우 간단 |
| 빌드 시간 | 느림 | 빠름 |
| 파일 크기 | ~5MB | ~100KB |
| 실행 | GUI 창 | 터미널 |
| 크로스 플랫폼 | 설정 필요 | 바로 가능 |

### 순수 C++의 장점
- ✅ **즉시 빌드 가능** - 라이브러리 설치 불필요
- ✅ **가볍다** - 실행 파일 크기 100KB 이하
- ✅ **빠르다** - 컴파일과 실행이 즉각적
- ✅ **이식성** - 어떤 플랫폼에서도 작동
- ✅ **교육용 최적** - 핵심에만 집중

---

## 📊 프로젝트 구조

```
memory_visualizer_console/
├── CMakeLists.txt           # 빌드 설정
├── README.md                # 이 파일
│
├── include/                 # 헤더 파일
│   ├── MemoryBlock.h       # 메모리 블록 정의
│   ├── MemoryManager.h     # 메모리 관리자
│   ├── ScriptParser.h      # 코드 파서
│   └── Visualizer.h        # ASCII 시각화
│
└── src/                    # 소스 파일
    ├── main.cpp            # 메인 + 메뉴
    ├── MemoryManager.cpp   # 메모리 관리 구현
    ├── ScriptParser.cpp    # 파서 구현
    └── Visualizer.cpp      # 시각화 구현
```

**총 라인 수: ~3,500 라인 (ImGui 버전과 동일)**  
**외부 의존성: 0개!**

---

## 🎓 교육적 가치

### 배울 수 있는 것

1. **순수 C++ 프로그래밍**
   - iostream, string, vector만으로 복잡한 프로그램 제작
   - 외부 라이브러리 없이 GUI 대체 (텍스트 UI)

2. **메모리 관리**
   - 스택 vs 힙
   - 포인터 동작
   - 스마트 포인터

3. **객체지향 프로그래밍**
   - 클래스 설계
   - 모듈화

4. **크로스 플랫폼 개발**
   - ANSI 색상 코드
   - 플랫폼 독립적 코드

---

## 💡 팁

### 색상이 안 보이는 경우

**Windows CMD:**
```bash
# Windows 10 이상에서 ANSI 색상 활성화
reg add HKCU\Console /v VirtualTerminalLevel /t REG_DWORD /d 1
```

**또는 Windows Terminal 사용 (추천)**

**Linux/Mac:**
- 대부분의 터미널에서 자동 지원

---

## 🚀 다음 단계

### 빌드하고 실행하기

```bash
# 1. 압축 해제
unzip memory_visualizer_console.zip
cd memory_visualizer_console

# 2. 빌드
mkdir build && cd build
cmake ..
make

# 3. 실행
./MemoryVisualizerConsole

# 4. 예제 실행해보기
# 메뉴에서 1번 선택 → 원하는 예제 선택
```

---

## 🎉 완성!

**외부 라이브러리 없이 순수 C++만으로 만든 메모리 시각화 도구!**

### 특징 요약
- ✅ 외부 라이브러리 **0개**
- ✅ 빌드 시간 **3초 이하**
- ✅ 실행 파일 **100KB 이하**
- ✅ 크로스 플랫폼
- ✅ 교육용 최적화
- ✅ 모든 핵심 기능 포함

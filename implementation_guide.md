# implementation_guide.md
## 목적
현재 `main.cpp` 하나에 모든 로직(게임, 렌더, 입력, 충돌, AI(ONNX), 리소스 로딩/해제)이 몰려 있다.
이를 **Modern C++ (C++17 범위)** 스타일로 리팩토링하여 다음을 달성한다.

- `main.cpp`는 엔트리포인트 + App 실행만 담당
- raylib / onnxruntime 사용은 유지하되, **RAII**, 모듈화, 명확한 책임 분리
- 전역 mutable 상태 제거, C 스타일 메모리 관리 제거
- 자원 누수/이중 해제/NULL data 체크 같은 취약한 패턴 제거
- 적절한 주석(특히 모델 입력/출력 shape, 전처리, 좌표계, 충돌 판정)

---

## 현재 코드의 주요 안티패턴(반드시 제거)
1) 전역 상태 난립  
- `onnx_session`, `onnx_env`, `onnx_session_options`, `onnx_input_values`, `frame_history`,
  `aiActionProb`, `binaryTextures`, RNG 등 다수 전역.
- 해결: `App`/`Game`/`AiAgent` 등 객체 멤버로 캡슐화.

2) C 스타일 메모리 관리 (malloc/free, bool** hitmask)
- `CreateHitmask()`가 `malloc`로 2중 포인터를 만들고 manual free.
- 해결: `std::vector<std::vector<uint8_t>>` 또는 1D contiguous buffer로 교체.
  (pixel hit 여부는 bool 대신 `uint8_t` 권장)

3) raylib 리소스 수동 Unload 산재
- 텍스처/이미지/사운드/렌더텍스처 등 Unload가 main 끝에 몰려 있고 중간에도 산재.
- 해결: RAII 래퍼(이동 가능, 복사 불가) 도입.

4) Image 처리의 위험한 패턴
- `frame_history`에 Image를 push하고 `data != NULL` 체크하며 수동 Unload.
- `preProcessImage()`에서 `LoadImageColors()` 이후 pixels를 수정하지만 `UpdateImageData` 없이 `ImageFromImage` 호출 등 불명확.
- 해결: Image 소유권/수명 규칙을 명확히 하고, “어떤 함수가 Unload 책임을 가지는지” 일관화.
  가능하면 raylib `Image`도 RAII로 감싼다.

5) getAIAction()가 너무 많은 책임
- 화면 캡처 crop → 전처리 → history 관리 → 텐서 구성 → 디버그 텍스처 생성 → 추론 → 샘플링까지 전부 포함.
- 해결: Ai 파이프라인을 분리:
  - `FrameProcessor` (crop + resize + binary)
  - `FrameStack` (최근 4프레임 유지)
  - `OnnxPolicy` (onnx session + infer)
  - `AiDebugView` (binaryTextures 업데이트 등)

6) 매직 넘버/하드코딩
- cropRect {0,108,width,512}, lowerStart 260, 다양한 상수.
- 해결: `Config` 구조체로 모으고 의미 주석 추가.

7) type에 과도한 auto 사용
- 해결: 가능한 명시적 타입 사용

---

## 빌드/컴파일 조건
- 현재 Makefile:
  - `-std=c++17` 유지
  - 단일 `main.cpp`에서 다중 소스로 변경 예정
- 리팩토링 후 Makefile은 아래처럼 갱신:
  - `SRC = src/main.cpp src/app/App.cpp ...`
  - 혹은 `SRC := $(wildcard src/**/*.cpp)` (GNU make 호환 시)
- include 경로는 기존 `(BREW_PREFIX)/include` 기반 유지

---

## 목표 디렉토리 구조(권장)
src/
main.cpp
app/
App.h
App.cpp
Config.h
game/
Game.h
Game.cpp
Bird.h
Bird.cpp
Pipe.h
Pipe.cpp
Collision.h
Collision.cpp
ai/
AiAgent.h
AiAgent.cpp
OnnxPolicy.h
OnnxPolicy.cpp
Preprocess.h
Preprocess.cpp
FrameStack.h
FrameStack.cpp
gfx/
RaylibContext.h
RaylibContext.cpp
Resources.h
Resources.cpp
DebugOverlay.h
DebugOverlay.cpp
util/
Rng.h
Time.h
Log.h


---

## 핵심 설계(필수)

### 1) Config 분리
`Config.h`에 다음을 구조체로 정의하고 main/app에서 주입한다.

- screen: width/height/fps/baseY
- physics: gravity/jumpVelocity/maxVelocityY
- pipe: gap/velocity/spawn rule
- ai:
  - input_size(84), frame_stack(4), display_size(40)
  - crop rect (y=108, h=512) 등
  - model path

### 2) RaylibContext (RAII)
- 생성자: `InitWindow`, `SetTargetFPS`, `InitAudioDevice`
- 소멸자: `CloseAudioDevice`, `CloseWindow`
- 복사 금지, 이동 가능(필요 시)
- “프레임 시작/종료”는 별도 유틸:
  - `struct DrawingScope { DrawingScope(){BeginDrawing();} ~DrawingScope(){EndDrawing();} };`
  - RenderTexture도 유사하게 `TextureModeScope`

### 3) Resources (RAII)
아래 리소스를 래핑한다:
- `Texture2D`, `Sound`, `RenderTexture2D`, `Image` (가능하면 전부)
패턴:
- 소멸자에서 `Unload*`
- 복사 금지, move만 허용
- `id > 0` 등의 방어 로직은 래퍼 내부로 숨김

### 4) Game 모듈
- `Game`이 상태를 소유:
  - `Bird`, `std::vector<Pipe>`, score/highscore, gameState
- 입력/업데이트/충돌/스폰 로직 분리:
  - `Game::update(dt, input, optional<AiAction>)`
  - `Game::render(renderer, overlay, ...)`

### 5) Collision 모듈
현재의 pixel-perfect hitmask 충돌을 모듈로 분리:
- hitmask는 `std::vector<uint8_t>`(size = w*h)로 저장하고 index로 접근
- `HitMask` 구조체:
  - `int w,h; std::vector<uint8_t> solid;`
  - `bool at(int x,int y) const`
- `Collision::pixelPerfect(birdMask, pipeMask, intersectionRect, offsets...)`

### 6) AI 모듈
#### 6.1 AiAgent 책임
- 게임 렌더 결과(또는 캡처된 Image)를 받아 action(0/1)을 리턴
- 내부에:
  - `FrameStack` (최근 4프레임)
  - `Preprocess` (crop/resize/grayscale/binary/flip)
  - `OnnxPolicy` (session + infer)
  - `AiDebugOverlay` (확률/프레임 텍스처 표시용 데이터)

#### 6.2 OnnxPolicy
- 전역 `Ort::Env`, `Ort::SessionOptions`, `Ort::Session` 제거
- 클래스 멤버로 소유
- 입력/출력 노드 이름, input shape 관리
- `std::array<int64_t,4> input_shape{1,4,S,S}`
- `infer(const std::vector<float>& input) -> std::array<float,2>`
- 예외 처리:
  - 실패 시 throw (상위에서 catch 후 AI 비활성화)
  - 로그에 model path 포함

#### 6.3 RNG 정리
현재 RNG가 전역이며 섞여 있음.
- `util/Rng.h`에 `std::mt19937` 하나를 소유하는 클래스를 만들고 주입.
- `generatePipe`는 RNG를 인자로 받는다.

#### 6.4 성능 주의(가능한 최소 개선)
- 현재 `getAIAction`은 매번 `ImageDrawPixel`로 84*84 픽셀을 찍어 비용 큼.
- 리팩토링 1차에서는 기능 동일성 우선하되,
  - 텐서 구성은 “직접 float buffer 채우기” 중심으로,
  - 디버그용 `binaryTextures` 업데이트는 “필요할 때만” 수행(프레임스택이 꽉 찼을 때).
- 절대: 매 프레임 불필요한 `GenImageColor` 다중 생성/해제 반복은 줄인다.

#### 6.5 권고 사항
- type 작성에 있어서 auto 사용은 자제하도록 한다.

---



## main.cpp에서 남길 내용(필수)
- `Config config = Config::default();` (또는 파일에서 로드)
- `App app(config);`
- `return app.run();`

---

## App::run() 루프 구조(가이드)
- A키로 AI 토글은 App 레벨에서 관리
- `RenderTexture` 캡처는 렌더러에서 수행 후, `Image`를 AiAgent에 전달
- Update 흐름:
  1. input 수집
  2. (aiControl) 현재 프레임 캡처 → AiAgent::act() → action
  3. Game update(action)
  4. draw game + overlay

---

## 주석 작성 규칙(필수)
- “무엇(what)”보다 “왜(why)”에 집중
- 아래는 반드시 주석:
  - cropRect의 의미(왜 y=108부터 자르는지)
  - AI input 구성: (N,C,H,W) = (1,4,84,84) 의미
  - flip 처리 이유(좌표계 차이인지, 학습 데이터 정의인지)
  - score 증가 조건(파이프 중심 통과 로직)

---

## 리팩토링 단계(실행 순서)
1) 기존 코드 컴파일/실행 확인 (리팩토링 전 스냅샷)
2) `Config`, `RaylibContext`, `Resources` 뼈대 추가
3) `Game` 모듈로 게임 상태/업데이트/렌더 분리
4) hitmask를 RAII + vector 기반으로 교체 (malloc/free 제거)
5) AI를 `AiAgent`/`OnnxPolicy`로 분리하고 전역 제거
6) Makefile을 다중 소스 빌드로 변경
7) 정리:
   - unused include 제거
   - const/참조/이동 semantics 정리
   - 함수 길이 축소(대형 함수 분해)
8) 최종 동작 확인:
   - 수동 플레이(SPACE)
   - AI 토글(A)
   - restart, 충돌, 점수 동작 동일

---

## Makefile 변경 지시(필수)
- `SRC = main.cpp`를 다중 파일로 교체.
예:
SRC =
src/main.cpp
src/app/App.cpp
src/game/Game.cpp
src/game/Collision.cpp
src/ai/AiAgent.cpp
src/ai/OnnxPolicy.cpp
src/gfx/RaylibContext.cpp
src/gfx/Resources.cpp

- include는 `-Isrc` 추가 가능:
  - `INCLUDES += -Isrc`

---

## 결과물 요구사항(체크리스트)
- [ ] 전역 mutable 상태 0개(상수 제외)
- [ ] malloc/free 사용 0개
- [ ] raylib 자원은 모두 RAII로 자동 해제
- [ ] `main.cpp`는 wiring만 수행
- [ ] AI 파이프라인은 독립 모듈이며, onnx 실패 시 게임은 정상 실행(단 AI OFF)
- [ ] 디버그 오버레이(확률/프레임 표시) 로직은 gfx/overlay로 분리
- [ ] 주석: 모델 I/O, crop/flip, 충돌 판정, 점수 로직에 설명 존재

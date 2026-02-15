# Rendering Flow Diagram

아래 다이어그램은 `App::Run()` 루프에서 한 프레임이 처리되는 순서와,
특히 `frame -> texture -> render target -> screen` 경로를 보여줍니다.

## 1) Main Loop (한 프레임 단위)

```mermaid
flowchart TD
    A[App::Run loop 시작] --> B[HandleGlobalInput]
    B --> C[Update]
    C --> D[Draw]
    D --> E[EndDrawing 후 화면 표시]
    E --> F{WindowShouldClose?}
    F -- No --> B
    F -- Yes --> G[루프 종료]
```

## 2) Update 단계 (AI ON/OFF 분기 포함)

```mermaid
flowchart TD
    U0[Update 시작] --> U1{state == Playing?}
    U1 -- No --> UEND[Update 종료]
    U1 -- Yes --> U2{AI control + model?}

    U2 -- Yes --> U3[RenderSceneToTarget]
    U3 --> U31[BeginTextureMode frameTarget]
    U31 --> U32[DrawGameScene 를 오프스크린 타겟에 그림]
    U32 --> U33[EndTextureMode]
    U33 --> U4[LoadImageFromTexture frameTarget.texture]
    U4 --> U5[AiAgent::Act 전처리+추론]
    U5 --> U6[flapRequested 결정]

    U2 -- No --> U7[SPACE 입력 시 flapRequested=true]

    U6 --> U8[선택 시 점프 사운드]
    U7 --> U8
    U8 --> U9[game::Game::Update 물리/애니메이션/파이프/점수]
    U9 --> U10[CheckPipeCollisions 픽셀충돌]
    U10 --> UEND
```

## 3) Draw 단계 (실제 화면 렌더)

```mermaid
flowchart TD
    D0[Draw 시작] --> D1{AI control + model?}
    D1 -- Yes --> D2[UpdateDebugTextures]
    D1 -- No --> D3[BeginDrawing]
    D2 --> D3
    D3 --> D4[ClearBackground]
    D4 --> D5[DrawGameScene 를 윈도우 백버퍼에 그림]
    D5 --> D6[HUD/텍스트/AI 확률/디버그 프레임 표시]
    D6 --> D7[EndDrawing]
    D7 --> D8[백버퍼 스왑 -> 모니터 표시]
```

## 4) Frame -> Texture -> Render Target 경로 요약

```mermaid
flowchart LR
    S[게임 월드 상태 Game::Bird Pipes Score] --> R0[DrawGameScene]
    R0 --> RT[RenderTexture frameTarget GPU]
    RT --> I[LoadImageFromTexture CPU Image]
    I --> P[Preprocess 84x84 gray/binary]
    P --> N[ONNX policy inference]
    N --> A[action flap/no flap]
    A --> U[다음 Update 물리 반영]
    U --> SCR[Draw 시 최종 화면 백버퍼 렌더]
    SCR --> M[모니터 출력]
```


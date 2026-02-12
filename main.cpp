#include <raylib.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cmath>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <memory>
#include <string>
#include <array>
#include <fstream>
#include <random>
#include <chrono>

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine generator(seed);
std::uniform_int_distribution<int> distribution(2, 10);
std::random_device rd;
std::mt19937 gen(rd());

const int SCREEN_WIDTH = 288;
const int SCREEN_HEIGHT = 512;
const int FPS = 30;
const int AI_INPUT_SIZE = 84;
const int AI_FRAME_DISPLAY_SIZE = 40;
const float baseY = SCREEN_HEIGHT * 0.79f;


std::unique_ptr<Ort::Session> onnx_session;
Ort::Env onnx_env;
Ort::SessionOptions onnx_session_options;
std::vector<float> onnx_input_values;
std::array<int64_t, 4> onnx_input_shape = {1, 4, AI_INPUT_SIZE, AI_INPUT_SIZE};
size_t onnx_input_tensor_size = 1 * 4 * AI_INPUT_SIZE * AI_INPUT_SIZE;
const char* onnx_input_name = "input";
const char* onnx_output_name = "output";
std::vector<const char*> onnx_input_names = {onnx_input_name};
std::vector<const char*> onnx_output_names = {onnx_output_name};
std::vector<Image> frame_history;

float aiActionProb[2] = {0.5f, 0.5f};
RenderTexture2D aiFrameTextures[4];
Texture2D binaryTextures[4];

struct Bird {
  Vector2 position;
  float velocity;
  Texture2D textures[3];
  int frame;
  int frameCounter;
  Rectangle bounds;
  bool is_flapped;
  int frameIndex;
};

const int BIRD_ANIMATION_FRAMES[4] = {0, 1, 2, 1};
const int BIRD_ANIMATION_LENGTH = 4;

struct Pipe {
  int x_upper;
  int y_upper;
  int x_lower;
  int y_lower;
};

struct ProcessedImages {
    Image original;
    Image rotated;
};

bool** CreateHitmask(Image image) {
    bool** hitmask = (bool**)malloc(image.height * sizeof(bool*));
    Color* pixels = LoadImageColors(image);
    
    for (int y = 0; y < image.height; y++) {
        hitmask[y] = (bool*)malloc(image.width * sizeof(bool));
        for (int x = 0; x < image.width; x++) {
            Color pixel = pixels[y * image.width + x];
            hitmask[y][x] = pixel.a > 0;
        }
    }
    UnloadImageColors(pixels);
    return hitmask;
}

void FreeHitmask(bool**& hitmask, int height) {
    if (hitmask != nullptr) {
        for (int y = 0; y < height; y++) {
            free(hitmask[y]);
            hitmask[y] = nullptr;
        }
        free(hitmask);
        hitmask = nullptr;
    }
}

Pipe generatePipe(int screenWidth, float baseY, int pipeGapSize, int pipeHeight) {
   int x = screenWidth + 10;
  
   int gapY = distribution(generator) * 10 + baseY / 5;
   int lowerUp = distribution(generator) * 5 * (-1);
   int lowerStart = gapY + pipeGapSize;
   int diff = 0;
   if (lowerStart > 260) {
     lowerStart = 260 + lowerUp;
     diff = lowerStart - (gapY + pipeGapSize);
   }
   return {x, gapY - pipeHeight + diff, x, lowerStart};
   }


void initializeGame(Bird& bird, std::vector<Pipe>& pipes, int& score, int& gameState, 
                    const Texture2D& pipeTex, int pipeGapSize) {
  bird.position = {(float)SCREEN_WIDTH / 5, (float)SCREEN_HEIGHT / 2};
  bird.velocity = 0;
  bird.frame = 0;
  bird.frameCounter = 0;
  bird.is_flapped = false;
  bird.frameIndex = 0;
  score = 0;
  gameState = 0;
  pipes.clear();
  pipes.push_back(generatePipe(SCREEN_WIDTH, SCREEN_HEIGHT * 0.79f, pipeGapSize, pipeTex.height));
  pipes.push_back(generatePipe(SCREEN_WIDTH * 1.5f, SCREEN_HEIGHT * 0.79f, pipeGapSize, pipeTex.height));
}

ProcessedImages preProcessImage(Image original, int targetWidth, int targetHeight) {

    Image resized = ImageCopy(original);
    ImageResize(&resized, targetWidth, targetHeight);
    ImageColorGrayscale(&resized);
    Color* pixels = LoadImageColors(resized);

    for (int i = 0; i < targetWidth * targetHeight; i++) {

        if (pixels[i].r > 1) {
            pixels[i].r = 255;
            pixels[i].g = 255;
            pixels[i].b = 255;
        } else {
            pixels[i].r = 0;
            pixels[i].g = 0;
            pixels[i].b = 0;
        }
    }
    
    Image result = ImageFromImage(resized, (Rectangle){0, 0, (float)targetWidth, (float)targetHeight});
    UnloadImage(resized);
    UnloadImageColors(pixels);
    
    Image original_version = ImageCopy(result);
    Image rotated_version = ImageCopy(result);
    
    ProcessedImages processed;
    processed.original = original_version;
    processed.rotated = rotated_version;
    
    UnloadImage(result);
    
    return processed;
}

bool initONNX(const char* modelPath) {
    try {
        
        if (!FileExists(modelPath)) {
            std::cerr << "ONNX model file not found: " << modelPath << std::endl;
            std::cerr << "ONNX File Check" << std::endl;
            return false;
        }
        
        onnx_env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "FlappyBird-RL");
        onnx_session_options.SetIntraOpNumThreads(1);
        onnx_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        
        onnx_session = std::make_unique<Ort::Session>(onnx_env, modelPath, onnx_session_options);
        
        onnx_input_values.resize(onnx_input_tensor_size);
        std::fill(onnx_input_values.begin(), onnx_input_values.end(), 0.0f);

        for (auto& img : frame_history) {
            UnloadImage(img);
        }
        frame_history.clear();
        
        std::cout << "ONNX model loaded successfully: " << modelPath << std::endl;
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing ONNX: " << e.what() << std::endl;
        return false;
    }
}


int getAIAction(Image currentFrame) {
    try {
        if (!onnx_session) {
            printf("ERROR: ONNX session is problem\n");
            return 0;
        }
        
        bool verbose = false;
        if (verbose) {
            std::cout << "Entering getAIAction" << std::endl;
        }
        
        Image cutCopy = ImageCopy(currentFrame);
        Rectangle cropRect = { 0, 108, (float)cutCopy.width, 512 };
        ImageCrop(&cutCopy, cropRect);

        ProcessedImages processedFrames = preProcessImage(cutCopy, AI_INPUT_SIZE, AI_INPUT_SIZE);
        UnloadImage(cutCopy);

        frame_history.push_back(processedFrames.rotated);
        
        if (frame_history.size() > 4) {
          if (frame_history[0].data != NULL) {
            UnloadImage(frame_history[0]);
            frame_history[0].data = NULL; 
          }
          frame_history.erase(frame_history.begin());
        }
        
        std::fill(onnx_input_values.begin(), onnx_input_values.end(), 0.0f);
        UnloadImage(processedFrames.original);
        
        for (size_t f = 0; f < frame_history.size(); f++) {
            if (f >= frame_history.size() || !frame_history[f].data) {
                continue;
            }
            
            size_t channel_idx = f;
            
            Color* pixels = LoadImageColors(frame_history[f]);
            if (!pixels) {
                continue;
            }
            
            Image binaryImage = GenImageColor(AI_INPUT_SIZE, AI_INPUT_SIZE, BLACK);
            if (!binaryImage.data) {
                UnloadImageColors(pixels);
                continue;
            }
            
            Image binaryImageFlipped = GenImageColor(AI_INPUT_SIZE, AI_INPUT_SIZE, BLACK);
            if (!binaryImageFlipped.data) {
                UnloadImageColors(pixels);
                UnloadImage(binaryImage);
                continue;
            }
            
            for (int y = 0; y < AI_INPUT_SIZE; y++) {
                for (int x = 0; x < AI_INPUT_SIZE; x++) {
                    int pixelIndex = y * AI_INPUT_SIZE + x;
                    if (pixelIndex < 0 || pixelIndex >= AI_INPUT_SIZE * AI_INPUT_SIZE) {
                        continue;
                    }
                    
                    float pixelValue = (pixels[pixelIndex].r > 0) ? 255.0f : 0.0f;
                    
                    Color pixelColor = (pixelValue > 127.0f) ? WHITE : BLACK;
                    ImageDrawPixel(&binaryImage, x, y, pixelColor);
                    
                    int flipped_y = AI_INPUT_SIZE - 1 - y;
                    
                    ImageDrawPixel(&binaryImageFlipped, x, flipped_y, pixelColor);
                    size_t tensorIndex = channel_idx * AI_INPUT_SIZE * AI_INPUT_SIZE + flipped_y * AI_INPUT_SIZE + x;
                    if (tensorIndex < onnx_input_values.size()) {
                        onnx_input_values[tensorIndex] = pixelValue;
                    } 
                }
            }
            UnloadImageColors(pixels);
            
            
            if (frame_history.size() == 4) {
                
                if (channel_idx < 4) {
                    Image displayImage = ImageCopy(binaryImageFlipped);
                    
                    if (displayImage.data != NULL && displayImage.width > 0 && displayImage.height > 0) {
                        ImageResize(&displayImage, AI_FRAME_DISPLAY_SIZE, AI_FRAME_DISPLAY_SIZE);
                        
                        if (binaryTextures[channel_idx].id > 0) {
                            UnloadTexture(binaryTextures[channel_idx]);
                        }
                        binaryTextures[channel_idx] = LoadTextureFromImage(displayImage);
                        UnloadImage(displayImage);
                    } 
                } 
            }
            UnloadImage(binaryImage);
            UnloadImage(binaryImageFlipped);
        }

        static int frame_counter = 0;
        frame_counter++;

        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, 
            onnx_input_values.data(), 
            onnx_input_tensor_size, 
            onnx_input_shape.data(), 
            onnx_input_shape.size()
        );
        auto output_tensors = onnx_session->Run(Ort::RunOptions{nullptr}, onnx_input_names.data(), &input_tensor, 1, onnx_output_names.data(), 1);
        float* output_data = output_tensors.front().GetTensorMutableData<float>();
        aiActionProb[0] = output_data[0];
        aiActionProb[1] = output_data[1];

        
        std::discrete_distribution<> d({aiActionProb[0], aiActionProb[1]});
        int action = d(gen);
        return action;

    } catch (const Ort::Exception& e) {
        printf("ERROR: ONNX Runtime exception: %s\n", e.what());
        return 0; 
    } catch (const std::exception& e) {
        printf("ERROR: Standard exception: %s\n", e.what());
        return 0;
    } catch (...) {
        printf("ERROR: Unknown exception in ONNX inference\n");
        return 0;
    }
}

int main() {
  bool debugMode = false;
  SetTraceLogLevel(debugMode ? LOG_INFO : LOG_WARNING);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "FlappyBird-RL");
  SetTargetFPS(FPS);
  
  InitAudioDevice();
  Sound jumpSound = LoadSound("assets/audio/jump.mp3");

  Texture2D bg = LoadTexture("assets/sprites/background-black.png");
  Texture2D base = LoadTexture("assets/sprites/base.png");
  Texture2D pipeTex = LoadTexture("assets/sprites/pipe-green.png");
  Texture2D pipeTexReverse = LoadTexture("assets/sprites/pipe-green-reverse.png");

  Bird bird;
  bird.textures[0] = LoadTexture("assets/sprites/redbird-upflap.png");
  bird.textures[1] = LoadTexture("assets/sprites/redbird-midflap.png");
  bird.textures[2] = LoadTexture("assets/sprites/redbird-downflap.png");

  Image birdUpImage = LoadImage("assets/sprites/redbird-upflap.png");
  Image birdMidImage = LoadImage("assets/sprites/redbird-midflap.png");
  Image birdDownImage = LoadImage("assets/sprites/redbird-downflap.png");
  Image pipeImage = LoadImage("assets/sprites/pipe-green.png");
  Image pipeReverseImage = LoadImage("assets/sprites/pipe-green-reverse.png");
  
  for (int i = 0; i < 4; i++) {
    aiFrameTextures[i] = LoadRenderTexture(AI_FRAME_DISPLAY_SIZE, AI_FRAME_DISPLAY_SIZE);
    BeginTextureMode(aiFrameTextures[i]);
    ClearBackground(BLACK);
    EndTextureMode();
  }
  
  bool** birdHitmasks[3];
  birdHitmasks[0] = CreateHitmask(birdUpImage);
  birdHitmasks[1] = CreateHitmask(birdMidImage);
  birdHitmasks[2] = CreateHitmask(birdDownImage);
  bool** pipeHitmask = CreateHitmask(pipeImage);
  bool** pipeReverseHitmask = CreateHitmask(pipeReverseImage);


  float gravity = 1.0f;
  float jumpVelocity = -9.0f;
  float maxVelocityY = 10.0f;
  
  int gameState = 0;
  std::vector<Pipe> pipes;
  int score = 0;
  int highscore = 0;
  int pipeGapSize = 110;
  float pipeVelocityX = -4.0f;
  
  bool aiControl = false;
  bool onnxInitialized = false;
  
  bool showAIFlap = false;
  int aiFlapCounter = 0;
  const int FLAP_DISPLAY_FRAMES = 15;
  
  const char* modelPath = "trained_models/flappy_bird_rl.onnx";
  onnxInitialized = initONNX(modelPath);
  if (onnxInitialized) {
    std::cout << "AI control enabled! Press 'A' to get into AI control." << std::endl;
  } else {
    std::cout << "Could not initialize ONNX model. AI control disabled." << std::endl;
  }

  Font font = GetFontDefault();

  initializeGame(bird, pipes, score, gameState, pipeTex, pipeGapSize);
  
  RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  
  for (int i = 0; i < 4; i++) {
    Image emptyImage = GenImageColor(AI_FRAME_DISPLAY_SIZE, AI_FRAME_DISPLAY_SIZE, BLACK);
    binaryTextures[i] = LoadTextureFromImage(emptyImage);
    UnloadImage(emptyImage);
  }
  
  while (!WindowShouldClose()) {
    if (onnxInitialized && IsKeyPressed(KEY_A)) {
      aiControl = !aiControl;
      
      if (aiControl) {
        for (auto& img : frame_history) {
          if (img.data != NULL) {
            UnloadImage(img);
          }
        }
        frame_history.clear();
        
        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawTexture(bg, 0, 0, WHITE);
        DrawTexture(base, 0, baseY, WHITE);
        for (auto& pipe : pipes) {
          DrawTexture(pipeTexReverse, pipe.x_upper, pipe.y_upper, WHITE);
          DrawTexture(pipeTex, pipe.x_lower, pipe.y_lower, WHITE);
        }
        
        DrawTexture(bird.textures[bird.frame], bird.position.x, bird.position.y, WHITE);
        EndTextureMode();
        
        Image screenshot = LoadImageFromTexture(target.texture);
        
        getAIAction(screenshot);
        
        for (int i = 0; i < 3; i++) {
          getAIAction(screenshot);
        }
        
        UnloadImage(screenshot);
      }
    }
    
    float baseX = -(fmod((-0 + 100), (float)base.width - (float)bg.width));
    
    int action = 0;
    
    if (IsKeyPressed(KEY_SPACE)) {
      if (gameState == 0)
        gameState = 1;
      else if (gameState == 2)
        initializeGame(bird, pipes, score, gameState, pipeTex, pipeGapSize);
    }
    
    if (gameState == 1) {
      if (aiControl && onnxInitialized) {
        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawTexture(bg, 0, 0, WHITE);
        
        for (auto& pipe : pipes) {
          DrawTexture(pipeTexReverse, pipe.x_upper, pipe.y_upper, WHITE);
          DrawTexture(pipeTex, pipe.x_lower, pipe.y_lower, WHITE);
        }
        
        DrawTexture(base, 0, baseY, WHITE);
        
        DrawTexture(bird.textures[bird.frame], bird.position.x, bird.position.y, WHITE);
        EndTextureMode();
        
        Image screenshot = LoadImageFromTexture(target.texture);
        
        action = getAIAction(screenshot);
        
        UnloadImage(screenshot);
        
        if (action == 1) {
          bird.velocity = jumpVelocity;
          bird.is_flapped = true;
          PlaySound(jumpSound);
          showAIFlap = true;
          aiFlapCounter = FLAP_DISPLAY_FRAMES;
        }
        
      } else {
        if (IsKeyPressed(KEY_SPACE)) {
          action = 1;
          bird.velocity = jumpVelocity;
          bird.is_flapped = true;
          PlaySound(jumpSound);
        }
      }
      
      if (bird.velocity < maxVelocityY && !bird.is_flapped) {
        bird.velocity += gravity;
      }
      if (bird.is_flapped) {
        bird.is_flapped = false;
      }
      bird.position.y = std::max(0.0f, bird.position.y + bird.velocity);
      
      // bird timing 부분
      if (++bird.frameCounter >= 3) {
        int oldIndex = bird.frameIndex;
        int oldFrame = bird.frame;
        
        bird.frameIndex = (bird.frameIndex + 1) % BIRD_ANIMATION_LENGTH;
        bird.frame = BIRD_ANIMATION_FRAMES[bird.frameIndex];
        bird.frameCounter = 0;
        
      }
      
      for (auto& pipe : pipes) {
        pipe.x_upper += pipeVelocityX;
        pipe.x_lower += pipeVelocityX;
      }
      
      if (!pipes.empty() && (0 < pipes[0].x_lower && pipes[0].x_lower < 5)) {
        Pipe newPipe = generatePipe(SCREEN_WIDTH, baseY, pipeGapSize, pipeTex.height);
        pipes.push_back(newPipe);
      }

      if (!pipes.empty() && pipes[0].x_lower < -pipeTex.width) {
        pipes.erase(pipes.begin());
      }
      
      float bird_center_x = bird.position.x + bird.textures[bird.frame].width / 2;
      for (auto& pipe : pipes) {
        float pipe_center_x = pipe.x_upper + pipeTex.width / 2;
        if (pipe_center_x < bird_center_x && bird_center_x < pipe_center_x + 5) {
          score++;
          highscore = std::max(score, highscore);
          break;
        }
      }
      
      if (bird.position.y + bird.textures[bird.frame].height >= baseY) {
        gameState = 2;
      }
      
      for (auto& pipe : pipes) {
        Vector2 birdPos = bird.position;
        Vector2 upperPipePos = {pipe.x_upper, pipe.y_upper};
        Vector2 lowerPipePos = {pipe.x_lower, pipe.y_lower};
        
        Rectangle birdRect = {birdPos.x, birdPos.y, 
                           (float)bird.textures[bird.frame].width, 
                           (float)bird.textures[bird.frame].height};
        Rectangle upperPipeRect = {upperPipePos.x, upperPipePos.y, 
                                 (float)pipeTex.width, (float)pipeTex.height};
        Rectangle lowerPipeRect = {lowerPipePos.x, lowerPipePos.y, 
                                 (float)pipeTex.width, (float)pipeTex.height};
        
        bool bboxCollision = CheckCollisionRecs(birdRect, upperPipeRect) || 
                            CheckCollisionRecs(birdRect, lowerPipeRect);
        
        if (!bboxCollision) {
          continue;
        }
        
        if (CheckCollisionRecs(birdRect, upperPipeRect)) {
          Rectangle cropped = {
            std::max(birdRect.x, upperPipeRect.x),
            std::max(birdRect.y, upperPipeRect.y),
            std::min(birdRect.x + birdRect.width, upperPipeRect.x + upperPipeRect.width) - std::max(birdRect.x, upperPipeRect.x),
            std::min(birdRect.y + birdRect.height, upperPipeRect.y + upperPipeRect.height) - std::max(birdRect.y, upperPipeRect.y)
          };
          
          int min_x1 = cropped.x - birdRect.x;
          int min_y1 = cropped.y - birdRect.y;
          int min_x2 = cropped.x - upperPipeRect.x;
          int min_y2 = cropped.y - upperPipeRect.y;
          
          for (int y = 0; y < cropped.height && y + min_y1 < bird.textures[bird.frame].height && y + min_y2 < pipeTex.height; y++) {
            for (int x = 0; x < cropped.width && x + min_x1 < bird.textures[bird.frame].width && x + min_x2 < pipeTex.width; x++) {
              if (birdHitmasks[bird.frame][y + min_y1][x + min_x1] && pipeReverseHitmask[y + min_y2][x + min_x2]) {
                gameState = 2;
                break;
              }
            }
            if (gameState == 2) break;
          }
        }
        
        if (gameState != 2 && CheckCollisionRecs(birdRect, lowerPipeRect)) {
          Rectangle cropped = {
            std::max(birdRect.x, lowerPipeRect.x),
            std::max(birdRect.y, lowerPipeRect.y),
            std::min(birdRect.x + birdRect.width, lowerPipeRect.x + lowerPipeRect.width) - std::max(birdRect.x, lowerPipeRect.x),
            std::min(birdRect.y + birdRect.height, lowerPipeRect.y + lowerPipeRect.height) - std::max(birdRect.y, lowerPipeRect.y)
          };
          
          int min_x1 = cropped.x - birdRect.x;
          int min_y1 = cropped.y - birdRect.y;
          int min_x2 = cropped.x - lowerPipeRect.x;
          int min_y2 = cropped.y - lowerPipeRect.y;
          
          for (int y = 0; y < cropped.height && y + min_y1 < bird.textures[bird.frame].height && y + min_y2 < pipeTex.height; y++) {
            for (int x = 0; x < cropped.width && x + min_x1 < bird.textures[bird.frame].width && x + min_x2 < pipeTex.width; x++) {
              if (birdHitmasks[bird.frame][y + min_y1][x + min_x1] && pipeHitmask[y + min_y2][x + min_x2]) {
                gameState = 2;
                break;
              }
            }
            if (gameState == 2) break;
          }
        }
        
        if (gameState == 2) {
          break;
        }
      }
    }
    
    BeginDrawing();
    ClearBackground(BLACK);
    
    DrawTexture(bg, 0, 0, WHITE);
    
    for (auto& pipe : pipes) {
      DrawTexture(pipeTexReverse, pipe.x_upper, pipe.y_upper, WHITE);
      DrawTexture(pipeTex, pipe.x_lower, pipe.y_lower, WHITE);
    }
    
    DrawTexture(base, baseX, baseY, WHITE);
    DrawTexture(bird.textures[bird.frame], bird.position.x, bird.position.y, WHITE);
    
    char scoreText[20];
    char highscoreText[20];
    
    if (onnxInitialized) {
      char aiStatusText[50];
      sprintf(aiStatusText, "AI: %s [A key]", aiControl ? "ON" : "OFF");
      DrawText(aiStatusText, 10, 10, 20, aiControl ? GREEN : RED);
      
      if (aiControl && showAIFlap) {
        DrawText("AI FLAP!", SCREEN_WIDTH/2 - MeasureText("AI FLAP!", 24)/2, 50, 24, YELLOW);
        
        aiFlapCounter--;
        if (aiFlapCounter <= 0) {
          showAIFlap = false;
        }
      }
      
      if (aiControl) {
        DrawRectangle(SCREEN_WIDTH - 130, 10, 120, 30, Fade(BLACK, 0.7f));
        
        char probText[50];
        sprintf(probText, "Do Nothing: %.2f", aiActionProb[0]);
        DrawText(probText, SCREEN_WIDTH - 125, 15, 10, aiActionProb[0] > aiActionProb[1] ? GREEN : WHITE);
        
        sprintf(probText, "Flap: %.2f", aiActionProb[1]);
        DrawText(probText, SCREEN_WIDTH - 125, 30, 10, aiActionProb[1] > aiActionProb[0] ? GREEN : WHITE);
        
        DrawRectangle(SCREEN_WIDTH - 125, 25, (int)(110 * aiActionProb[0]), 2, 
                      aiActionProb[0] > aiActionProb[1] ? GREEN : WHITE);
        DrawRectangle(SCREEN_WIDTH - 125, 40, (int)(110 * aiActionProb[1]), 2, 
                      aiActionProb[1] > aiActionProb[0] ? GREEN : WHITE);
        
        int frameX = 10;
        for (size_t i = 0; i < 4; i++) {
          char frameText[10];
          sprintf(frameText, "%zu", i);
          DrawText(frameText, frameX + AI_FRAME_DISPLAY_SIZE/2 - 5, 40, 10, WHITE);
          
          if (binaryTextures[i].id > 0) {
            DrawTexture(binaryTextures[i], frameX, 80, WHITE);
          } else {
            DrawRectangleLines(frameX, 80, AI_FRAME_DISPLAY_SIZE, AI_FRAME_DISPLAY_SIZE, RED);
          }
          
          frameX += AI_FRAME_DISPLAY_SIZE + 5;
        }
      }
    }
    
    if (gameState == 2) {
      DrawText("Press SPACE to restart", SCREEN_WIDTH/2 - MeasureText("Press SPACE to restart", 20)/2, 
               SCREEN_HEIGHT/2 + 30/2 + 10, 20, WHITE);
    }
    
    if (gameState == 0) {
      DrawText("Press SPACE to start", SCREEN_WIDTH/2 - MeasureText("Press SPACE to start", 20)/2, 
               SCREEN_HEIGHT/2, 20, WHITE);
    }
    
    EndDrawing();
  }

  for (int i = 0; i < 3; i++) {
    FreeHitmask(birdHitmasks[i], birdUpImage.height);
  }
  FreeHitmask(pipeHitmask, pipeImage.height);
  FreeHitmask(pipeReverseHitmask, pipeReverseImage.height);

  UnloadImage(birdUpImage);
  UnloadImage(birdMidImage);
  UnloadImage(birdDownImage);
  UnloadImage(pipeImage);
  UnloadImage(pipeReverseImage);

  UnloadTexture(bg);
  UnloadTexture(base);
  UnloadTexture(pipeTex);
  UnloadTexture(pipeTexReverse);
  UnloadTexture(bird.textures[0]);
  UnloadTexture(bird.textures[1]);
  UnloadTexture(bird.textures[2]);
  
  for (int i = 0; i < 4; i++) {
    UnloadRenderTexture(aiFrameTextures[i]);
  }

  for (auto& img : frame_history) {
    UnloadImage(img);
  }
  frame_history.clear();
  onnx_session.reset();

  for (int i = 0; i < 4; i++) {
    UnloadTexture(binaryTextures[i]);
  }

  UnloadRenderTexture(target);
  
  UnloadSound(jumpSound);
  CloseAudioDevice();

  CloseWindow();
  return 0;
}
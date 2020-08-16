#include <iostream>
#include <mutex>
#include <condition_variable>
#include <sys/types.h>
#include <thread>
#include <stdio.h>

#include "Rom.h"
#include "GUI.h"
#include "Gameboy.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"

const u_int8_t GAME_WIDTH = 160;
const u_int8_t GAME_HEIGHT = 144;
const u_int8_t GAME_SCALE = 2;
const u_int16_t TILE_MAP_WIDTH = 256;
const u_int16_t TILE_MAP_HEIGHT = TILE_MAP_WIDTH;
const u_int16_t TILE_SET_WIDTH = 8 * 16;
const u_int16_t TILE_SET_HEIGHT = TILE_SET_WIDTH;
const u_int16_t TILE_SET_TILE_COUNT = 256;
const u_int8_t TILE_SET_ROW_COUNT = 16;
const u_int8_t TILE_WIDTH = 8;
const u_int8_t TILE_HEIGHT = TILE_WIDTH;
const u_int16_t TEXTURE_WIDTH = TILE_MAP_WIDTH * 2;
const u_int16_t TEXTURE_HEIGHT = GAME_HEIGHT + TILE_MAP_HEIGHT + (TILE_SET_HEIGHT * 2); 
const ImVec4 BACKGROUND_COLOUR = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const u_int8_t PALETTE_SIZE = 20;
const u_int8_t PALETTE_PADDING = 5;
const u_int16_t TILE_SET_UPDATE_INTERVAL = 120;

float* audioSamples;
std::mutex audioMutex;
std::condition_variable audioCondition;
bool audioPlaying = false;

static void audioCallback(void *udata, Uint8 *stream, int requestedByteLength) {
    std::unique_lock<std::mutex> audioLock(audioMutex);
    SDL_memset(stream, 0, requestedByteLength);
    if(!audioPlaying) return;

    u_int16_t requestedFloatLength = requestedByteLength / sizeof(float);
    u_int16_t sampleCount = requestedFloatLength > SAMPLE_PLAY_COUNT ? SAMPLE_PLAY_COUNT : requestedFloatLength;
    SDL_MixAudio(stream, (Uint8*) audioSamples, sampleCount * sizeof(float), SDL_MIX_MAXVOLUME);

    audioPlaying = false;
    audioLock.unlock();
    audioCondition.notify_one();
}

class DesktopGUI : GUI {

private:
    bool running = true;
    bool buttonsDown[8] = {[0 ... 7] = false};
    struct config* config;

public:
    Pixels texturePixels;
    u_int16_t fps = 0;

    DesktopGUI(struct config* config) : texturePixels(TEXTURE_WIDTH, TEXTURE_HEIGHT) {
        this->config = config;
        SDL_AudioSpec audioFormat;
        audioFormat.freq = SAMPLE_RATE;
        audioFormat.format = AUDIO_F32;

        audioFormat.channels = 1;
        audioFormat.samples = SAMPLE_PLAY_COUNT;
        audioFormat.callback = audioCallback;
        audioFormat.userdata = NULL;

        SDL_OpenAudio(&audioFormat, NULL);
        SDL_PauseAudio(0);
    }

    void displayBuffer(unsigned int* pixels) {
        Pixels gamePixels(GAME_WIDTH, GAME_HEIGHT, pixels);
        texturePixels.paste(0, 0, &gamePixels);
        texturePixels.paste(0, GAME_HEIGHT, config->tileMap_0);
        texturePixels.paste(TILE_MAP_WIDTH, GAME_HEIGHT, config->tileMap_1);
    };

    void displayFPS(uint16 fps) {
        this->fps = fps;
    }

    bool isOpen() {
        return running;
    }

    bool isDown(uint8 button) {
        return buttonsDown[button];
    }

    void playAudio(float *samples, uint16 count) {
        audioSamples = samples;
        audioPlaying = true;

        std::unique_lock<std::mutex> audioLock(audioMutex);
        audioCondition.wait(audioLock, []{ return !audioPlaying; });
    }

    void stop() {
        running = false;
    }

    void setButtonDown(u_int8_t button, bool state) {
        buttonsDown[button] = state;
    }
};

static void startEmulator(DesktopGUI* gui, std::string romPath, config* config)
{
    Gameboy gameboy = Gameboy(romPath, (GUI*) gui, config);
    gameboy.run();
}

static void updateTileSet(TileSet* tileSet, bool alternateBank, Pixels* texturePixels, u_int16_t textureX, u_int16_t textureY) {
    static uint16 frameCount = 0;
    static uint8 tileSetCount = 0;

    tileSetCount += 1;

    if(tileSetCount == 4) {
        tileSetCount = 0;
        frameCount += 1;
    }

    if(frameCount < TILE_SET_UPDATE_INTERVAL) return;

    palette tileSetPalette;
    tileSetPalette.isColour = true;
    tileSetPalette.colours[0] = 0xFFFFFFFF;
    tileSetPalette.colours[1] = 0xFFd3d3d3;
    tileSetPalette.colours[2] = 0xFFa9a9a9;
    tileSetPalette.colours[3] = 0xFF000000;

    uint16 x = 0;
    uint16 y = 0;
    Pixels pixels(TILE_SET_WIDTH, TILE_SET_HEIGHT);

    for(uint16 tileIndex = 0; tileIndex < TILE_SET_TILE_COUNT; tileIndex++) {
        if(tileIndex % TILE_SET_ROW_COUNT == 0 && tileIndex > 0) {
            y += TILE_HEIGHT;
            x = 0;
        }

        for(uint8 tileLine = 0; tileLine < TILE_HEIGHT; tileLine++) {
            Tile* tile = tileSet->getTile(tileIndex, false, alternateBank, false);
            tile->drawLine(&pixels, tileSetPalette, tileLine, x, y + tileLine, false, false);
            delete tile;
        }

        x += TILE_WIDTH;
    }

    texturePixels->paste(textureX, textureY, &pixels);

    if(frameCount > TILE_SET_UPDATE_INTERVAL) {
        frameCount = 0;
    }
}
 
int main(int argc, char *argv[]) {
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("MBoy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsClassic();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    std::string romPath (argv[1]);
    config emulatorConfig;
    DesktopGUI gui(&emulatorConfig);
    std::thread emulatorThread (startEmulator, &gui, romPath, &emulatorConfig);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            bool isDown = false;
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch(event.type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    if(io.WantCaptureKeyboard) break;
                    isDown = event.type == SDL_KEYDOWN;
                    if(event.key.keysym.sym == SDLK_z) gui.setButtonDown(BUTTON_A, isDown);
                    if(event.key.keysym.sym == SDLK_x) gui.setButtonDown(BUTTON_B, isDown);
                    if(event.key.keysym.sym == SDLK_RETURN) gui.setButtonDown(BUTTON_START, isDown);
                    if(event.key.keysym.sym == SDLK_BACKSPACE) gui.setButtonDown(BUTTON_SELECT, isDown);
                    if(event.key.keysym.sym == SDLK_LEFT) gui.setButtonDown(BUTTON_LEFT, isDown);
                    if(event.key.keysym.sym == SDLK_UP) gui.setButtonDown(BUTTON_UP, isDown);
                    if(event.key.keysym.sym == SDLK_RIGHT) gui.setButtonDown(BUTTON_RIGHT, isDown);
                    if(event.key.keysym.sym == SDLK_DOWN) gui.setButtonDown(BUTTON_DOWN, isDown);
                    break;

                case SDL_QUIT:
                    running = false;
                    break;

                default:
                    break;
            }
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::Begin("Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Performance");
        ImGui::Checkbox("Turbo", &emulatorConfig.turbo);
        ImGui::Checkbox("Disable Tile Map Cache", &emulatorConfig.disableTileMapCache);
        ImGui::Checkbox("Disable Tile Set Cache", &emulatorConfig.disableTileSetCache);
        ImGui::Text("Display");
        ImGui::Checkbox("Background", &emulatorConfig.background);
        ImGui::Checkbox("Window", &emulatorConfig.window);
        ImGui::Checkbox("Sprites", &emulatorConfig.sprites);
        ImGui::Text("Audio");
        ImGui::Checkbox("Enable", &emulatorConfig.audio);
        ImGui::Checkbox("Square 1", &emulatorConfig.square1);
        ImGui::Checkbox("Square 2", &emulatorConfig.square2);
        ImGui::Checkbox("Wave", &emulatorConfig.wave);
        ImGui::End();

        ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Game - %d", gui.fps);
        ImGui::Text("Render - %.0f", ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, gui.texturePixels.data);
        ImVec2 start = ImVec2(0.0f, 0.0f);
        ImVec2 end = ImVec2(GAME_WIDTH / (TEXTURE_WIDTH * 1.0f), GAME_HEIGHT / (TEXTURE_HEIGHT * 1.0f));
        ImGui::Image((void*)(intptr_t)texture, ImVec2(GAME_WIDTH * GAME_SCALE, GAME_HEIGHT * GAME_SCALE), start, end);
        ImGui::End();
        
        if(emulatorConfig.backgroundColourPalettes != nullptr
            && emulatorConfig.spriteColourPalettes != nullptr) {
            ImGui::Begin("Colour Palettes", nullptr, ImGuiWindowFlags_NoResize);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            
            float originalX = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + PALETTE_PADDING);
            ImGui::Text("Background");

            ImGui::SameLine();
            ImGui::SetCursorPosX(originalX + ((PALETTE_SIZE + PALETTE_PADDING) * 4) + PALETTE_PADDING);
            ImGui::Text("Sprites");

            const ImVec2 p = ImGui::GetCursorScreenPos();
            float x = p.x + PALETTE_PADDING, y = p.y + PALETTE_PADDING;

            for(uint8 i = 0; i < 8; i++) {
                x = p.x + PALETTE_PADDING;
                for(uint8 j = 0; j < 4; j++) {
                    draw_list->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + PALETTE_SIZE + 1, y + PALETTE_SIZE + 1),
                        0xFFFFFFFF);
                    
                    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + PALETTE_SIZE, y + PALETTE_SIZE),
                        emulatorConfig.backgroundColourPalettes[i].colours[j]);

                    x += PALETTE_SIZE + PALETTE_PADDING;
                }

                y += PALETTE_SIZE + PALETTE_PADDING;
            }

            float columnStart = x + PALETTE_PADDING;
            x = columnStart, y = p.y + PALETTE_PADDING;

            for(uint8 i = 0; i < 8; i++) {
                x = columnStart;
                for(uint8 j = 0; j < 4; j++) {
                    draw_list->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + PALETTE_SIZE + 1, y + PALETTE_SIZE + 1),
                        0xFFFFFFFF);

                    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + PALETTE_SIZE, y + PALETTE_SIZE),
                        emulatorConfig.spriteColourPalettes[i].colours[j]);

                    x += PALETTE_SIZE + PALETTE_PADDING;
                }
                y += PALETTE_SIZE + PALETTE_PADDING;
            }
            
            ImGui::SetWindowSize(ImVec2(
                x - ImGui::GetWindowPos().x + PALETTE_PADDING,
                y - ImGui::GetWindowPos().y + PALETTE_PADDING));

            ImGui::End();
        }

        if(emulatorConfig.tileMap_0 != nullptr) {
            ImGui::Begin("Tile Maps", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImVec2 start = ImVec2(0.0f, GAME_HEIGHT / (TEXTURE_HEIGHT * 1.0f));
            ImVec2 end = ImVec2(1.0f, (GAME_HEIGHT + TILE_MAP_HEIGHT) / (TEXTURE_HEIGHT * 1.0f));
            ImGui::Image((void*)(intptr_t)texture, ImVec2(TILE_MAP_WIDTH * 2, TILE_MAP_HEIGHT), start, end);
            ImGui::End();
        }
 
        if(emulatorConfig.tileSet_0 != nullptr) {
            ImGui::Begin("Tile Sets", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            updateTileSet(emulatorConfig.tileSet_0, false, &gui.texturePixels, 0, GAME_HEIGHT + TILE_MAP_HEIGHT);
            updateTileSet(emulatorConfig.tileSet_0, true, &gui.texturePixels, TILE_SET_WIDTH, GAME_HEIGHT + TILE_MAP_HEIGHT);
            updateTileSet(emulatorConfig.tileSet_1, false, &gui.texturePixels, 0, GAME_HEIGHT + TILE_MAP_HEIGHT + TILE_SET_HEIGHT);
            updateTileSet(emulatorConfig.tileSet_1, true, &gui.texturePixels, TILE_SET_WIDTH, GAME_HEIGHT + TILE_MAP_HEIGHT + TILE_SET_HEIGHT);
            ImVec2 start = ImVec2(0.0f, (GAME_HEIGHT + TILE_MAP_HEIGHT) / (TEXTURE_HEIGHT * 1.0f));
            ImVec2 end = ImVec2((TILE_SET_WIDTH * 2) / (TEXTURE_WIDTH * 1.0f), 1.0f);
            ImGui::Image((void*)(intptr_t)texture, ImVec2(TILE_SET_WIDTH * 2, TILE_SET_HEIGHT * 2), start, end);
            ImGui::End();
        }

        ImGui::Render();
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
        glClearColor(BACKGROUND_COLOUR.x, BACKGROUND_COLOUR.y, BACKGROUND_COLOUR.z, BACKGROUND_COLOUR.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    gui.stop();
    emulatorThread.join();

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
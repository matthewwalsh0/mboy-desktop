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

const u_int8_t WIDTH = 160;
const u_int8_t HEIGHT = 144;
const u_int8_t SCALE = 2;
const ImVec4 BACKGROUND_COLOUR = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const u_int8_t PALETTE_SIZE = 20;
const u_int8_t PALETTE_PADDING = 5;

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

public:
    unsigned int* pixels;
    u_int16_t fps = 0;

    DesktopGUI() {
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
        this->pixels = pixels;
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

static GLuint pixelsToTexture(unsigned int* data, u_int16_t width, u_int16_t height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return texture;
}

static void renderTileMap(Pixels* tileMap) {
    GLuint tileMapTexture = pixelsToTexture(tileMap->data, tileMap->width, tileMap->height);
    ImGui::Image((void*)(intptr_t)tileMapTexture, ImVec2(tileMap->width, tileMap->height));
}

static void renderTileSet(TileSet* tileSet, bool alternateBank, u_int8_t cacheId) {
    static GLuint cachedTextures[4] = {0};
    static uint8 frameCounts[4] = {0};

    GLuint cachedTexture = cachedTextures[cacheId];
    uint8 frameCount = frameCounts[cacheId];

    frameCounts[cacheId] += 1;

    if(frameCount >= 120 || cachedTexture == 0) {
        Pixels pixels(8 * 16, 8 * 16);
        uint16 x = 0;
        uint16 y = 0;

        palette tileSetPalette;
        tileSetPalette.isColour = true;
        tileSetPalette.colours[0] = 0xFFFFFFFF;
        tileSetPalette.colours[1] = 0xFFd3d3d3;
        tileSetPalette.colours[2] = 0xFFa9a9a9;
        tileSetPalette.colours[3] = 0xFF000000;

        for(uint16 i = 0; i < 256; i++) {
            if(i % 16 == 0 && i > 0) {
                y += 8;
                x = 0;
            }

            for(uint8 j = 0; j < 8; j++) {
                tileSet
                    ->getTile(i, false, alternateBank, false)
                    ->drawLine(&pixels, tileSetPalette, j, x, y + j, false, false);
            }

            x += 8;
        }

        GLuint newTexture = pixelsToTexture(pixels.data, pixels.width, pixels.height);
        cachedTextures[cacheId] = newTexture;
        cachedTexture = newTexture;

        frameCounts[cacheId] = 0;
    }

    ImGui::Image((void*)(intptr_t)cachedTexture, ImVec2(8 * 16, 8 * 16));
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
    DesktopGUI gui;
    config emulatorConfig;
    std::thread emulatorThread (startEmulator, &gui, romPath, &emulatorConfig);

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
        ImGui::Checkbox("Turbo", &emulatorConfig.turbo);
        ImGui::End();

        ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Game - %d", gui.fps);
        ImGui::Text("Render - %.0f", ImGui::GetIO().Framerate);
        ImGui::End();

        if(gui.pixels != nullptr) {
            ImGui::Begin("Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            GLuint gameTexture = pixelsToTexture(gui.pixels, WIDTH, HEIGHT);
            ImGui::Image((void*)(intptr_t)gameTexture, ImVec2(WIDTH * SCALE, HEIGHT * SCALE));
            ImGui::End();
        }
        
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
            renderTileMap(emulatorConfig.tileMap_0);
            ImGui::SameLine();
            renderTileMap(emulatorConfig.tileMap_1);
            ImGui::End();
        }
 
        if(emulatorConfig.tileSet_0 != nullptr) {
            ImGui::Begin("Tile Sets", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            renderTileSet(emulatorConfig.tileSet_0, false, 0);
            ImGui::SameLine();
            renderTileSet(emulatorConfig.tileSet_0, true, 1);
            renderTileSet(emulatorConfig.tileSet_1, false, 2);
            ImGui::SameLine();
            renderTileSet(emulatorConfig.tileSet_1, true, 3);
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
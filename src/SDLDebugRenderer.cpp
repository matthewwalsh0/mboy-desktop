#include "SDLDebugRenderer.h"

const u_int16_t WINDOW_WIDTH = 1280;
const u_int16_t WINDOW_HEIGHT = 720;
const u_int8_t GAME_SCALE = 2;
const ImVec4 BACKGROUND_COLOUR = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const u_int8_t PALETTE_SIZE = 20;
const u_int8_t PALETTE_PADDING = 5;
const u_int16_t TILE_SET_UPDATE_INTERVAL = 120;

static void updateTileSet(TileSet* tileSet, bool alternateBank, Pixels* texturePixels, u_int16_t textureX, u_int16_t textureY) {
    static u_int16_t frameCount = 0;
    static u_int8_t tileSetCount = 0;

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

    u_int16_t x = 0;
    u_int16_t y = 0;
    Pixels pixels(TILE_SET_WIDTH, TILE_SET_HEIGHT);

    for(u_int16_t tileIndex = 0; tileIndex < TILE_SET_TILE_COUNT; tileIndex++) {
        if(tileIndex % TILE_SET_ROW_COUNT == 0 && tileIndex > 0) {
            y += TILE_HEIGHT;
            x = 0;
        }

        for(u_int8_t tileLine = 0; tileLine < TILE_HEIGHT; tileLine++) {
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

SDLDebugRenderer::SDLDebugRenderer() {
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("MBoy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, window_flags);
    glContext = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsClassic();
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL2_Init();

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

bool SDLDebugRenderer::draw(DesktopGUI* gui) {
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
                if(event.key.keysym.sym == SDLK_z) gui->setButtonDown(BUTTON_A, isDown);
                if(event.key.keysym.sym == SDLK_x) gui->setButtonDown(BUTTON_B, isDown);
                if(event.key.keysym.sym == SDLK_RETURN) gui->setButtonDown(BUTTON_START, isDown);
                if(event.key.keysym.sym == SDLK_BACKSPACE) gui->setButtonDown(BUTTON_SELECT, isDown);
                if(event.key.keysym.sym == SDLK_LEFT) gui->setButtonDown(BUTTON_LEFT, isDown);
                if(event.key.keysym.sym == SDLK_UP) gui->setButtonDown(BUTTON_UP, isDown);
                if(event.key.keysym.sym == SDLK_RIGHT) gui->setButtonDown(BUTTON_RIGHT, isDown);
                if(event.key.keysym.sym == SDLK_DOWN) gui->setButtonDown(BUTTON_DOWN, isDown);
                if(event.key.keysym.sym == SDLK_ESCAPE) return false;
                break;

            case SDL_QUIT:
                return false;

            default:
                break;
        }
    }

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    ImGui::Begin("Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Performance");
    ImGui::Checkbox("Turbo", &gui->config->turbo);
    ImGui::Checkbox("Disable Tile Map Cache", &gui->config->disableTileMapCache);
    ImGui::Checkbox("Disable Tile Set Cache", &gui->config->disableTileSetCache);
    ImGui::Text("Display");
    ImGui::Checkbox("Background", &gui->config->background);
    ImGui::Checkbox("Window", &gui->config->window);
    ImGui::Checkbox("Sprites", &gui->config->sprites);
    ImGui::Text("Audio");
    ImGui::Checkbox("Enable", &gui->config->audio);
    ImGui::Checkbox("Square 1", &gui->config->square1);
    ImGui::Checkbox("Square 2", &gui->config->square2);
    ImGui::Checkbox("Wave", &gui->config->wave);
    ImGui::Checkbox("Noise", &gui->config->noise);
    ImGui::End();

    ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Game - %d", gui->fps);
    ImGui::Text("Render - %.0f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Begin("Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, gui->texturePixels.data);
    ImVec2 start = ImVec2(0.0f, 0.0f);
    ImVec2 end = ImVec2(GAME_WIDTH / (TEXTURE_WIDTH * 1.0f), GAME_HEIGHT / (TEXTURE_HEIGHT * 1.0f));
    ImGui::Image((void*)(intptr_t)texture, ImVec2(GAME_WIDTH * GAME_SCALE, GAME_HEIGHT * GAME_SCALE), start, end);
    ImGui::End();
    
    if(gui->config->backgroundColourPalettes != nullptr
        && gui->config->spriteColourPalettes != nullptr) {
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

        for(u_int8_t i = 0; i < 8; i++) {
            x = p.x + PALETTE_PADDING;
            for(u_int8_t j = 0; j < 4; j++) {
                draw_list->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + PALETTE_SIZE + 1, y + PALETTE_SIZE + 1),
                    0xFFFFFFFF);
                
                draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + PALETTE_SIZE, y + PALETTE_SIZE),
                    gui->config->backgroundColourPalettes[i].colours[j]);

                x += PALETTE_SIZE + PALETTE_PADDING;
            }

            y += PALETTE_SIZE + PALETTE_PADDING;
        }

        float columnStart = x + PALETTE_PADDING;
        x = columnStart, y = p.y + PALETTE_PADDING;

        for(u_int8_t i = 0; i < 8; i++) {
            x = columnStart;
            for(u_int8_t j = 0; j < 4; j++) {
                draw_list->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + PALETTE_SIZE + 1, y + PALETTE_SIZE + 1),
                    0xFFFFFFFF);

                draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + PALETTE_SIZE, y + PALETTE_SIZE),
                    gui->config->spriteColourPalettes[i].colours[j]);

                x += PALETTE_SIZE + PALETTE_PADDING;
            }
            y += PALETTE_SIZE + PALETTE_PADDING;
        }
        
        ImGui::SetWindowSize(ImVec2(
            x - ImGui::GetWindowPos().x + PALETTE_PADDING,
            y - ImGui::GetWindowPos().y + PALETTE_PADDING));

        ImGui::End();
    }

    if(gui->config->tileMap_0 != nullptr) {
        ImGui::Begin("Tile Maps", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImVec2 start = ImVec2(0.0f, GAME_HEIGHT / (TEXTURE_HEIGHT * 1.0f));
        ImVec2 end = ImVec2(1.0f, (GAME_HEIGHT + TILE_MAP_HEIGHT) / (TEXTURE_HEIGHT * 1.0f));
        ImGui::Image((void*)(intptr_t)texture, ImVec2(TILE_MAP_WIDTH * 2, TILE_MAP_HEIGHT), start, end);
        ImGui::End();
    }

    if(gui->config->tileSet_0 != nullptr) {
        ImGui::Begin("Tile Sets", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        updateTileSet(gui->config->tileSet_0, false, &gui->texturePixels, 0, GAME_HEIGHT + TILE_MAP_HEIGHT);
        updateTileSet(gui->config->tileSet_0, true, &gui->texturePixels, TILE_SET_WIDTH, GAME_HEIGHT + TILE_MAP_HEIGHT);
        updateTileSet(gui->config->tileSet_1, false, &gui->texturePixels, 0, GAME_HEIGHT + TILE_MAP_HEIGHT + TILE_SET_HEIGHT);
        updateTileSet(gui->config->tileSet_1, true, &gui->texturePixels, TILE_SET_WIDTH, GAME_HEIGHT + TILE_MAP_HEIGHT + TILE_SET_HEIGHT);
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

    return true;
}

SDLDebugRenderer::~SDLDebugRenderer() {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
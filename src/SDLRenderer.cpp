#include "SDLRenderer.h"

const u_int8_t GAME_SCALE = 3;

SDLRenderer::SDLRenderer() {
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("MBoy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GAME_WIDTH * GAME_SCALE, GAME_HEIGHT * GAME_SCALE, window_flags);
    glContext = SDL_GL_CreateContext(window);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, TEXTURE_WIDTH, TEXTURE_HEIGHT);

    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);
}

bool SDLRenderer::draw(DesktopGUI* gui) {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        bool isDown = false;

        switch(event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
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

    SDL_Rect gameRect;
    gameRect.x = 0,
    gameRect.y = 0;
    gameRect.w = GAME_WIDTH;
    gameRect.h = GAME_HEIGHT;

    SDL_UpdateTexture(texture, NULL, gui->texturePixels.data, TEXTURE_WIDTH * sizeof(unsigned int));
    SDL_RenderCopy(renderer, texture, &gameRect, NULL);
    SDL_RenderPresent(renderer);
    SDL_GL_SwapWindow(window);

    return true;
}

SDLRenderer::~SDLRenderer() {
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
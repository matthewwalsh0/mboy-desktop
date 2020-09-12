#ifndef SDLRENDERER_H
#define SDLRENDERER_H

#include "Renderer.h"
#include "DesktopGUI.h"
#include "SDL.h"
#include "SDL_opengl.h"

class SDLRenderer : Renderer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_GLContext glContext;
public:
    SDLRenderer();
    ~SDLRenderer();
    bool draw(DesktopGUI* gui) override;
};

#endif
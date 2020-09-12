#ifndef SDLDEBUGRENDERER_H
#define SDLDEBUGRENDERER_H

#include "Renderer.h"
#include "DesktopGUI.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"

class SDLDebugRenderer : Renderer {
private:
    SDL_Window* window;
    SDL_GLContext glContext;
    ImGuiIO io;
    GLuint texture;
public:
    SDLDebugRenderer();
    ~SDLDebugRenderer();
    bool draw(DesktopGUI* gui) override;
};

#endif

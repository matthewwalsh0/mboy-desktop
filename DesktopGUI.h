#ifndef DESKTOPGUI_H
#define DESKTOPGUI_H

#include "GUI.h"
#include "Pixels.h"
#include "Config.h"

const u_int8_t GAME_WIDTH = 160;
const u_int8_t GAME_HEIGHT = 144;
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

class DesktopGUI : GUI {

private:
    bool running = true;
    bool buttonsDown[8] = {[0 ... 7] = false};

public:
    Pixels texturePixels;
    u_int16_t fps = 0;
    struct config* config;

    void displayBuffer(unsigned int* pixels) override;
    void displayFPS(uint16 fps) override;
    bool isOpen() override;
    bool isDown(uint8 button) override;
    void playAudio(float *samples, uint16 count) override;

    DesktopGUI(struct config* config);
    void setButtonDown(u_int8_t button, bool state);
    void stop();
};

#endif
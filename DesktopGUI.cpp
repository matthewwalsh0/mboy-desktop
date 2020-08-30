#include "DesktopGUI.h"
#include "Audio.h"
#include "Pixels.h"

DesktopGUI::DesktopGUI(struct config* config) : texturePixels(TEXTURE_WIDTH, TEXTURE_HEIGHT) {
    this->config = config;
    initAudio();
}

void DesktopGUI::displayBuffer(unsigned int* pixels) {
    Pixels gamePixels(GAME_WIDTH, GAME_HEIGHT, pixels);
    texturePixels.paste(0, 0, &gamePixels);
    texturePixels.paste(0, GAME_HEIGHT, this->config->tileMap_0);
    texturePixels.paste(TILE_MAP_WIDTH, GAME_HEIGHT, this->config->tileMap_1);
};

void DesktopGUI::displayFPS(u_int16_t fps) {
    this->fps = fps;
}

bool DesktopGUI::isOpen() {
    return running;
}

bool DesktopGUI::isDown(u_int8_t button) {
    return buttonsDown[button];
}

void DesktopGUI::playAudio(float *samples, u_int16_t count) {
    processAudio(samples, count);
}

void DesktopGUI::stop() {
    running = false;
}

void DesktopGUI::setButtonDown(u_int8_t button, bool state) {
    buttonsDown[button] = state;
}
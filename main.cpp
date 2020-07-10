#include <iostream>
#include "Rom.h"
#include "MiniFB.h"
#include "GUI.h"
#include "Gameboy.h"

const unsigned int SCALE = 3;
const unsigned int WIDTH = 160;
const unsigned int HEIGHT = 144;

class MiniFBGUI : GUI {

private:
    mfb_window* window;
    bool open = true;
    unsigned int pixels[WIDTH * SCALE * HEIGHT * SCALE] = {0};

public:
    MiniFBGUI(mfb_window* window) {
        this->window = window;
    }

    void displayBuffer(unsigned int* pixels) {
        for(int y = 0; y < HEIGHT; y++) {
            for(int x = 0; x < WIDTH; x++) {
                for(int y2 = 0; y2 < SCALE; y2++) {
                    for(int x2 = 0; x2 < SCALE; x2++) {
                        int sourceIndex = y * WIDTH + x;
                        int targetIndex = ((y * SCALE + y2) * (WIDTH * SCALE)) + (x * SCALE) + x2;
                        this->pixels[targetIndex] = pixels[sourceIndex];
                    }
                }
            }
        }
        
        this->open = !mfb_update(window, this->pixels);
    };

    void displayFPS(uint16 fps) {
        std::cout << "FPS: " << fps << "\n";
    }

    bool isOpen() {
        return this->open;
    }
};
 
int main(int argc, char *argv[]) {
    std::string path (argv[1]);
    Rom rom(path);
    struct mfb_window *window = mfb_open_ex(rom.name.c_str(), WIDTH * SCALE, HEIGHT * SCALE, WF_RESIZABLE);

    if (!window) return 0;

    MiniFBGUI gui(window);
    Gameboy(rom, (GUI*) &gui).run();

    return 0;
}
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <sys/types.h>

#include "MiniFB.h"
#include "SDL.h"
#include "Rom.h"
#include "GUI.h"
#include "Gameboy.h"

const u_int8_t SCALE = 4;
const u_int8_t WIDTH = 160;
const u_int8_t HEIGHT = 144;

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

class MiniFBGUI : GUI {

private:
    mfb_window* window;
    bool open = true;
    unsigned int pixels[WIDTH * SCALE * HEIGHT * SCALE] = {0};

public:
    MiniFBGUI(mfb_window* window) {
        this->window = window;

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

    void playAudio(float *samples, uint16 count) {
        audioSamples = samples;
        audioPlaying = true;

        std::unique_lock<std::mutex> audioLock(audioMutex);
        audioCondition.wait(audioLock, []{ return !audioPlaying; });
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
#include "Audio.h"
#include "SDL.h"
#include "APU.h"
#include <iostream>
#include <mutex>
#include <condition_variable>

float* audioSamples;
std::mutex audioMutex;
std::condition_variable audioCondition;
bool audioPlaying = false;

static void audioCallback(void *udata, u_int8_t *stream, int requestedByteLength) {
    std::unique_lock<std::mutex> audioLock(audioMutex);
    SDL_memset(stream, 0, requestedByteLength);
    if(!audioPlaying) return;

    u_int16_t requestedFloatLength = requestedByteLength / sizeof(float);
    u_int16_t sampleCount = requestedFloatLength > SAMPLE_PLAY_COUNT ? SAMPLE_PLAY_COUNT : requestedFloatLength;
    SDL_MixAudio(stream, (u_int8_t*) audioSamples, sampleCount * sizeof(float), SDL_MIX_MAXVOLUME);

    audioPlaying = false;
    audioLock.unlock();
    audioCondition.notify_one();
}

void initAudio() {
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

void processAudio(float *samples, u_int16_t count) {
    audioSamples = samples;
    audioPlaying = true;

    std::unique_lock<std::mutex> audioLock(audioMutex);
    audioCondition.wait(audioLock, []{ return !audioPlaying; });
}
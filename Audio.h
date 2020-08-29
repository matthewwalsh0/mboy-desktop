#ifndef AUDIO_H
#define AUDIO_H

#include <sys/types.h>

void initAudio();
void processAudio(float *samples, u_int16_t count);

#endif
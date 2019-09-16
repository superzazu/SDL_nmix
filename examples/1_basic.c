// 1_basic.c: demonstrates how to create two simple sound sources
//            and play them simultaneously

#include <stdio.h>
#include <SDL.h>
#include "../SDL_nmix.h"

static void sine_callback(void* userdata, void* _stream, int len) {
    float* stream = (float*) _stream;
    double* x = (double*) userdata;

    float freq = 440.f;
    for (size_t i = 0; i < len / sizeof(float); i++) {
        *x += (double) 1 / 44100;
        stream[i] = sinf(freq * (*x) * 2 * M_PI);
    }
}

static inline float sign(float x) {
    if (x < 0) return -1;
    if (x > 0) return 1;
    return 0;
}

static void square_callback(void* userdata, void* _stream, int len) {
    Sint16* stream = (Sint16*) _stream;
    double* x = (double*) userdata;

    float freq = 440.f / 2;
    for (size_t i = 0; i < len / sizeof(Sint16); i += 2) {
        *x += (double) 1 / 44100;
        stream[i] = sign(sinf(freq * (*x) * 2 * M_PI)) * 5000;
        stream[i + 1] = stream[i];
    }
}

int main(void) {
    if (NMIX_OpenAudio(NMIX_DEFAULT_DEVICE, NMIX_DEFAULT_FREQUENCY, NMIX_DEFAULT_SAMPLES) != 0) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
        return -1;
    }

    // creating a new source, with float samples, 1 channel (mono), and "x"
    // is passed as userdata.
    double x = 0;
    NMIX_Source* source1 = NMIX_NewSource(
        AUDIO_F32SYS, 1, 44100, sine_callback, &x);
    if (source1 == NULL) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
    }
    NMIX_SetPan(source1, -.2); // a bit on the left
    NMIX_SetGain(source1, .5); // 50% volume

    // creating a new stereo source with Sint16 samples
    double x2 = 0;
    NMIX_Source* source2 = NMIX_NewSource(
        AUDIO_S16SYS, 2, 44100, square_callback, &x2);
    if (source2 == NULL) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
    }
    NMIX_SetPan(source2, .9); // 90% on the right
    NMIX_SetGain(source2, .7);

    NMIX_Play(source1);
    NMIX_Play(source2);

    SDL_Delay(5 * 1000);

    NMIX_FreeSource(source1);
    NMIX_FreeSource(source2);
    NMIX_CloseAudio();
    SDL_Quit();
    return 0;
}

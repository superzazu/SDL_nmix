// 3_stresstest.c: plays a lot of sources simultaneously. This was
//                 written primarily to check that performance is acceptable
//                 even when a lot of sources are played at once.

#include <stdio.h>
#include <math.h>
#include <SDL.h>
#include "../SDL_nmix.h"

#define NB_VOICES 32

static void sine_callback(void* userdata, void* _stream, int len) {
  float* stream = (float*) _stream;
  double* x = (double*) userdata;

  float freq = 440.f;
  for (size_t i = 0; i < len / sizeof(float); i += 2) {
    *x += (double) 1 / 44100;
    stream[i] = sinf(freq * (*x) * 2 * M_PI);
    stream[i + 1] = stream[i];
  }
}

int main(int argc, char** argv) {
  if (NMIX_OpenAudio(NMIX_DEFAULT_DEVICE, NMIX_DEFAULT_FREQUENCY,
          NMIX_DEFAULT_SAMPLES) != 0) {
    fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
    return -1;
  }

  NMIX_SetMasterGain(0); // mute audio output

  double x = 0;

  NMIX_Source* sources[NB_VOICES] = {NULL};
  for (int i = 0; i < NB_VOICES; i++) {
    sources[i] = NMIX_NewSource(AUDIO_F32SYS, 2, 44100, sine_callback, &x);
    if (sources[i] == NULL) {
      fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
    }

    NMIX_Play(sources[i]);
  }

  SDL_Delay(30 * 1000);

  for (int i = 0; i < NB_VOICES; i++) {
    NMIX_FreeSource(sources[i]);
  }

  NMIX_CloseAudio();
  SDL_Quit();
  return 0;
}

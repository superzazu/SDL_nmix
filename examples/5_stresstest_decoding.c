// 5_stresstest_decoding.c: plays 32 streams of ogg files simultaneously.

#include <stdio.h>
#include <SDL.h>
#include "../SDL_nmix.h"
#include "../SDL_nmix_file.h"

#define NB_VOICES 8

int main(int argc, char** argv) {
  if (NMIX_OpenAudio(NMIX_DEFAULT_DEVICE, NMIX_DEFAULT_FREQUENCY,
          NMIX_DEFAULT_SAMPLES) != 0) {
    fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
    return -1;
  }
  Sound_Init();

  NMIX_SetMasterGain(0);

  NMIX_FileSource* sources[NB_VOICES] = {NULL};
  for (int i = 0; i < NB_VOICES; i++) {
    SDL_RWops* f = SDL_RWFromFile("../music.ogg", "rb");
    sources[i] = NMIX_NewFileSource(f, "ogg", 0);
    if (sources[i] == NULL) {
      fprintf(stderr, "Cannot open file: %s\n", SDL_GetError());
      return 1;
    }
    NMIX_SetLoop(sources[i], 1);
    NMIX_Play(sources[i]->source);
  }

  SDL_Delay(30 * 1000);

  for (int i = 0; i < NB_VOICES; i++) {
    NMIX_FreeFileSource(sources[i]);
  }

  NMIX_CloseAudio();
  Sound_Quit();
  SDL_Quit();
  return 0;
}

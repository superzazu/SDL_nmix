#include "SDL_nmix.h"

static SDL_AudioSpec mixer = {0};
static SDL_AudioDeviceID audio_device = 0;
static NMIX_Source* playing_sources = NULL; // linked list of sources playing
static float master_gain = 1.f;

static SDL_INLINE float clampf(float x, float min, float max) {
  return x < min ? min : (x > max ? max : x);
}

// mixes two samples "a" and "b" together
static SDL_INLINE float mix_samples(float a, float b) {
  // there are a lot of better ways to mix two samples together,
  // however this turns out to be simple to implement, quick to execute,
  // and good enough for a lot of cases.
  return clampf(a + b, -1, 1);
}

// apply linear panning to two samples (pan must be between -1 and 1)
static SDL_INLINE void apply_panning(float pan, float* left, float* right) {
  float amplitude = pan / 2 + 0.5;
  *left *= 1 - amplitude;
  *right *= amplitude;
}

// the callback used by SDL_nmix to mix all the sources together
static void SDLCALL nmix_callback(
    void* userdata, Uint8* _buffer, int buffer_size) {
  SDL_memset(_buffer, 0, buffer_size);
  int const sample_size = SDL_AUDIO_SAMPLELEN(mixer.format);

  if (playing_sources == NULL) {
    return;
  }

  // retrieving audio data for each voice currently playing
  NMIX_Source* s = playing_sources;
  while (s != NULL) {
    NMIX_Source* next = s->next;

    int bytes_written = 0;
    while (bytes_written < buffer_size) {
      // calculating the copy size
      int copy_size = SDL_AudioStreamAvailable(s->stream);

      if (copy_size > s->out_buffer_size) {
        copy_size = s->out_buffer_size;
      }
      if (bytes_written + copy_size > buffer_size) {
        copy_size = buffer_size - bytes_written;
      }

      // retrieving bytes from converter
      int bytes_read = SDL_AudioStreamGet(s->stream, s->out_buffer, copy_size);

      // copying those bytes to buffer, mixing them with existing samples
      float* const buffer = (float*) (_buffer + bytes_written);
      float* const out_buffer = (float*) s->out_buffer;
      for (int i = 0; i < bytes_read / sample_size; i += 2) {
        float sl = out_buffer[i] * s->gain * master_gain;
        float sr = out_buffer[i + 1] * s->gain * master_gain;
        apply_panning(s->pan, &sl, &sr);

        buffer[i] = mix_samples(buffer[i], sl);
        buffer[i + 1] = mix_samples(buffer[i + 1], sr);
      }

      bytes_written += bytes_read;

      // if there's no more data available in converter, send
      // more data to it
      if (SDL_AudioStreamAvailable(s->stream) == 0) {
        if (!s->eof) {
          // retrieve new data
          s->callback(s->userdata, s->in_buffer, s->in_buffer_size);

          // and push it to the converter
          if (SDL_AudioStreamPut(s->stream, s->in_buffer, s->in_buffer_size) !=
              0) {
            fprintf(stderr, "SDL_nmix: FATAL: %s\n", SDL_GetError());
            return;
          }
        } else {
          // end of file, we remove the source from playing_sources
          // list by calling NMIX_Pause.
          NMIX_Pause(s);

          // no more data to write, skip remaining bytes
          bytes_written = buffer_size;
        }
      }
    }

    s = next;
  }
}

int NMIX_OpenAudio(const char* device, int rate, int samples) {
  if (audio_device != 0) {
    SDL_SetError("NMIX device is already opened.");
    return -1;
  }

  SDL_version linked;
  SDL_GetVersion(&linked);
  if (SDL_VERSIONNUM(linked.major, linked.minor, linked.patch) <
      SDL_VERSIONNUM(2, 0, 7)) {
    SDL_SetError("SDL_nmix requires SDL 2.0.7 or later.");
    return -1;
  }

  if (!SDL_WasInit(SDL_INIT_AUDIO)) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
      return -1;
    }
  }

  // SDL_nmix uses float32 (AUDIO_F32SYS) samples for mixing, and only
  // supports stereo output
  SDL_AudioSpec wanted_spec = {0};
  wanted_spec.freq = rate;
  wanted_spec.format = AUDIO_F32SYS;
  wanted_spec.channels = 2;
  wanted_spec.samples = samples;
  wanted_spec.callback = nmix_callback;
  wanted_spec.userdata = NULL;

  audio_device = SDL_OpenAudioDevice(
      device, 0, &wanted_spec, &mixer, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (audio_device == 0) {
    return -1;
  }

  NMIX_PausePlayback(SDL_FALSE);

  return 0;
}

int NMIX_CloseAudio(void) {
  if (audio_device == 0) {
    SDL_SetError("NMIX device is already closed.");
    return -1;
  }
  SDL_PauseAudioDevice(audio_device, 1);
  SDL_CloseAudioDevice(audio_device);
  audio_device = 0;
  return 0;
}

void NMIX_PausePlayback(SDL_bool pause_on) {
  SDL_PauseAudioDevice(audio_device, pause_on);
}

float NMIX_GetMasterGain(void) {
  return master_gain;
}

void NMIX_SetMasterGain(float gain) {
  master_gain = clampf(gain, 0, 2);
}

SDL_AudioSpec* NMIX_GetAudioSpec(void) {
  return &mixer;
}

SDL_AudioDeviceID NMIX_GetAudioDevice(void) {
  return audio_device;
}

NMIX_Source* NMIX_NewSource(SDL_AudioFormat format, Uint8 channels, int rate,
    NMIX_SourceCallback callback, void* userdata) {
  if (audio_device == 0) {
    SDL_SetError("Please open NMIX device before creating sources.");
    return NULL;
  }

  NMIX_Source* source = SDL_malloc(sizeof(NMIX_Source));
  if (source == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  source->format = format;
  source->channels = channels;
  source->rate = rate;
  source->pan = 0.f;
  source->gain = 1.f;
  source->callback = callback;
  source->userdata = userdata;
  source->eof = SDL_FALSE;

  // allocating the internal buffers:
  // note: we allocate roughly the size needed to store all the samples
  // that correspond to one nmix callback.
  int in_nb_samples = source->rate * mixer.samples / mixer.freq;
  source->in_buffer_size =
      in_nb_samples * source->channels * SDL_AUDIO_SAMPLELEN(source->format);
  source->in_buffer = SDL_malloc(source->in_buffer_size);
  if (source->in_buffer == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  source->out_buffer_size = mixer.size;
  source->out_buffer = SDL_malloc(source->out_buffer_size);
  if (source->out_buffer == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  source->stream = SDL_NewAudioStream(source->format, source->channels,
      source->rate, mixer.format, mixer.channels, mixer.freq);
  if (source->stream == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  source->prev = NULL;
  source->next = NULL;

  return source;
}

void NMIX_FreeSource(NMIX_Source* source) {
  if (source == NULL) {
    return;
  }

  SDL_LockAudioDevice(audio_device);
  if (NMIX_IsPlaying(source)) {
    NMIX_Pause(source);
  }

  SDL_free(source->in_buffer);
  SDL_free(source->out_buffer);
  SDL_FreeAudioStream(source->stream);
  SDL_free(source);
  SDL_UnlockAudioDevice(audio_device);
}

int NMIX_Play(NMIX_Source* source) {
  if (source == NULL) {
    return -1;
  }

  SDL_LockAudioDevice(audio_device);

  // cannot play a source that is already being played
  if (NMIX_IsPlaying(source)) {
    SDL_UnlockAudioDevice(audio_device);
    SDL_SetError("source is already playing");
    return -1;
  }

  source->eof = SDL_FALSE;

  // no sources currently playing, so we set the first source
  if (playing_sources == NULL) {
    playing_sources = source;
  } else {
    // find the last source currently playing
    NMIX_Source* v = playing_sources;
    while (v->next) {
      v = v->next;
    }

    // append the source at the end of the list
    v->next = source;
    source->prev = v;
  }

  SDL_UnlockAudioDevice(audio_device);

  return 0;
}

void NMIX_Pause(NMIX_Source* source) {
  if (source == NULL) {
    return;
  }

  SDL_LockAudioDevice(audio_device);

  if (!NMIX_IsPlaying(source)) {
    SDL_UnlockAudioDevice(audio_device);
    return;
  }

  // we remove the source from the list
  if (source->next) {
    source->next->prev = source->prev;
  }

  if (source->prev) {
    source->prev->next = source->next;
  } else {
    // first element in list
    playing_sources = source->next;
  }

  source->prev = NULL;
  source->next = NULL;

  SDL_UnlockAudioDevice(audio_device);
}

SDL_bool NMIX_IsPlaying(NMIX_Source* source) {
  if (source == NULL) {
    return SDL_FALSE;
  }
  return source == playing_sources || source->prev != NULL;
}

float NMIX_GetPan(NMIX_Source* source) {
  if (source == NULL) {
    return 0;
  }
  return source->pan;
}

void NMIX_SetPan(NMIX_Source* source, float pan) {
  if (source == NULL) {
    return;
  }
  source->pan = clampf(pan, -1, 1);
}

float NMIX_GetGain(NMIX_Source* source) {
  if (source == NULL) {
    return 0;
  }
  return source->gain;
}

void NMIX_SetGain(NMIX_Source* source, float gain) {
  if (source == NULL) {
    return;
  }
  source->gain = clampf(gain, 0, 2);
}

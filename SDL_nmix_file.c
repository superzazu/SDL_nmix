#include "SDL_nmix_file.h"

// this function is used for debug to analyse performance
// static int Fake_Sound_Decode(Sound_Sample* sample) {
//     SDL_memset(sample->buffer, 0, sample->buffer_size);
//     return sample->buffer_size;
// }

static void sdlsound_callback(void* userdata, void* _buffer, int buffer_size) {
  NMIX_FileSource* s = (NMIX_FileSource*) userdata;
  Uint8* buffer = (Uint8*) _buffer;

  // SDL_sound uses an internal buffer "s->sample->buffer" with a fixed size
  // "s->sample->buffer_size", so we keep track of "where we are at" in
  // the buffer using the pointer "s->buffer".
  // The variable "s->bytes_left" represents the number of bytes left
  // to read in the SDL_sound internal buffer.

  int bytes_written = 0;
  while (bytes_written < buffer_size) {
    // we calculate how many bytes we should copy
    int copy_size = buffer_size - bytes_written;
    if (copy_size > s->bytes_left) {
      copy_size = s->bytes_left;
    }

    // we copy "copy_size" bytes to the buffer and advance our pointer
    SDL_memcpy(buffer + bytes_written, s->buffer, copy_size);
    s->bytes_left -= copy_size;
    s->buffer += copy_size;
    bytes_written += copy_size;

    // if we copied all bytes from the internal buffer, we try to fetch
    // more data
    if (s->bytes_left == 0) {
      // we reset the buffer pointer + bytes_left values
      s->bytes_left = s->sample->buffer_size;
      s->buffer = s->sample->buffer;

      // we check if we have reached EOF and set the flag accordingly
      if (s->predecoded) {
        // if the source is predecoded, 0 bytes left means we're at EOF
        s->source->eof = SDL_TRUE;
      } else {
        // if the source is streamed, we check EOF with SDL_sound
        // if we're not at EOF, we decode more data
        if ((s->sample->flags & SOUND_SAMPLEFLAG_EOF) ||
            (s->sample->flags & SOUND_SAMPLEFLAG_EAGAIN)) {
          s->source->eof = SDL_TRUE;
        } else {
          s->bytes_left = Sound_Decode(s->sample);
        }
      }

      // if we are at EOF, we either rewind it (if loop is on)
      // or set all remaining bytes of buffer to silence
      if (s->source->eof) {
        if (s->loop_on) {
          NMIX_Rewind(s);
        } else {
          // set all remaining bytes to silence (0) and return
          SDL_memset(buffer + bytes_written, 0, buffer_size - bytes_written);
          break;
        }
      }
    }
  }
}

NMIX_FileSource* NMIX_NewFileSource(
    SDL_RWops* rw, const char* ext, SDL_bool predecode) {
  if (NMIX_GetAudioDevice() == 0) {
    SDL_SetError("Please open NMIX device before creating sources.");
    return NULL;
  }

  NMIX_FileSource* s = SDL_malloc(sizeof(NMIX_FileSource));
  if (s == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  s->rw = rw;
  s->ext = ext;

  SDL_AudioSpec* spec = NMIX_GetAudioSpec();

  s->sample = Sound_NewSample(s->rw, s->ext, NULL, spec->size);
  if (s->sample == NULL) {
    SDL_free(s);
    SDL_SetError("SDL_sound error: %s", Sound_GetError());
    return NULL;
  }

  s->source = NMIX_NewSource(s->sample->actual.format,
      s->sample->actual.channels, s->sample->actual.rate, sdlsound_callback, s);
  if (s->source == NULL) {
    Sound_FreeSample(s->sample);
    SDL_free(s);
    return NULL;
  }

  s->buffer = s->sample->buffer;
  s->bytes_left = 0;

  s->loop_on = SDL_FALSE;

  s->predecoded = predecode;
  if (predecode) {
    // we predecode the whole file: Sound_DecodeAll will resize
    // its internal buffer and store all data inside.
    s->bytes_left = Sound_DecodeAll(s->sample);
    s->buffer = s->sample->buffer;
  }

  return s;
}

Sint32 NMIX_GetDuration(NMIX_FileSource* s) {
  if (s == NULL) {
    return -1;
  }

  return Sound_GetDuration(s->sample);
}

int NMIX_Seek(NMIX_FileSource* s, int ms) {
  if (s == NULL) {
    return -1;
  }

  if (Sound_Seek(s->sample, ms) == 0) {
    SDL_SetError("Error while seeking source: %s", Sound_GetError());
    return -1;
  }

  return 0;
}

int NMIX_Rewind(NMIX_FileSource* s) {
  if (s == NULL) {
    return -1;
  }

  SDL_LockAudioDevice(NMIX_GetAudioDevice());
  s->source->eof = SDL_FALSE;
  s->bytes_left = 0;
  s->buffer = s->sample->buffer;

  if (s->predecoded) {
    s->bytes_left = s->sample->buffer_size;
  } else {
    if (Sound_Rewind(s->sample) == 0) {
      SDL_SetError("Error while rewinding source: %s", Sound_GetError());
      SDL_UnlockAudioDevice(NMIX_GetAudioDevice());
      return -1;
    }
  }
  SDL_UnlockAudioDevice(NMIX_GetAudioDevice());
  return 0;
}

SDL_bool NMIX_GetLoop(NMIX_FileSource* s) {
  if (s == NULL) {
    return SDL_FALSE;
  }

  return s->loop_on;
}

void NMIX_SetLoop(NMIX_FileSource* s, SDL_bool loop_on) {
  if (s == NULL) {
    return;
  }

  s->loop_on = loop_on;
}

void NMIX_FreeFileSource(NMIX_FileSource* s) {
  if (s == NULL) {
    return;
  }

  SDL_LockAudioDevice(NMIX_GetAudioDevice());
  if (s->sample != NULL) {
    Sound_FreeSample(s->sample);
  }
  if (s->source != NULL) {
    NMIX_FreeSource(s->source);
  }
  SDL_free(s);
  SDL_UnlockAudioDevice(NMIX_GetAudioDevice());
}

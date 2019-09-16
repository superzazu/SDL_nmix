#include "SDL_nmix_file.h"

// this function is used for debug to analyse performance
static int Fake_Sound_Decode(Sound_Sample* sample) {
    SDL_memset(sample->buffer, 0, sample->buffer_size);
    return sample->buffer_size;
}

static void sdlsound_callback(void* userdata, void* _stream, int bytes) {
    NMIX_FileSource* s = (NMIX_FileSource*) userdata;
    Uint8* stream = (Uint8*) _stream;

    // SDL_sound uses an internal buffer `s->sample->buffer` with a fixed size
    // `s->sample->buffer_size` ; so we keep track of "where we are at" in
    // the buffer using the pointer `s->buffer`.
    // The variable `s->bytes_left` represents the number of bytes left
    // to read in the SDL_sound internal buffer.

    int bytes_written = 0;
    while (bytes_written < bytes) {
        // we calculate how many bytes we should copy
        int copy_size = bytes - bytes_written;
        if (copy_size > s->bytes_left) {
            copy_size = s->bytes_left;
        }

        // we copy "copy_size" bytes to the stream and advance our pointer
        SDL_memcpy(stream + bytes_written, s->buffer, copy_size);
        s->bytes_left -= copy_size;
        s->buffer += copy_size;
        bytes_written += copy_size;

        // we fetch more data if needed
        if (s->bytes_left == 0) {
            // we reset the buffer pointer + bytes_left values
            s->bytes_left = s->sample->buffer_size;
            s->buffer = s->sample->buffer;

            // if the source is predecoded, that means we reached the end of
            // source
            if (s->predecoded) {
                s->source->remove = SDL_TRUE;
            }
            else {
                if ((s->sample->flags & SOUND_SAMPLEFLAG_EOF) ||
                    (s->sample->flags & SOUND_SAMPLEFLAG_EAGAIN)) {
                    s->source->remove = SDL_TRUE;
                }
                else {
                    // the source is streamed, so we try to access more data
                    s->bytes_left = Sound_Decode(s->sample);
                }
            }

            // if we arrived at the end of the source, we rewind it
            if (s->source->remove) {
                NMIX_Rewind(s);
                if (s->loop_on) {
                    s->source->remove = SDL_FALSE;
                }
                else {
                    // set all remaining bytes to silence (0) and return
                    SDL_memset(stream + bytes_written, 0,
                        bytes - bytes_written);
                    return;
                }
            }
        }
    }
}

NMIX_FileSource* NMIX_NewFileSource(SDL_RWops* rw, const char* ext,
        SDL_bool predecode) {
    if (NMIX_GetAudioDevice() == 0) {
        SDL_SetError("Please open NMIX device before creating sources.");
        return NULL;
    }

    NMIX_FileSource* s = SDL_malloc(sizeof(NMIX_FileSource));
    if (s == NULL) {
        SDL_SetError("Failed to allocate memory while loading sound");
        return NULL;
    }

    s->rw = rw;
    s->ext = ext;

    SDL_AudioSpec* spec = NMIX_GetAudioSpec();
    Sound_AudioInfo format = {spec->format, spec->channels, spec->freq};

    s->sample = Sound_NewSample(s->rw, s->ext, &format, spec->size);
    if (s->sample == NULL) {
        SDL_free(s);
        SDL_SetError("SDL_sound error: %s", Sound_GetError());
        return NULL;
    }

    s->source = NMIX_NewSource(
        spec->format,
        spec->channels,
        spec->freq,
        sdlsound_callback,
        s);
    if (s->source == NULL) {
        Sound_FreeSample(s->sample);
        SDL_free(s);
        return NULL;
    }

    s->buffer = s->sample->buffer;
    s->bytes_left = 0;

    s->loop_on = SDL_FALSE;

    // we predecode the whole file if necessary: Sound_DecodeAll will resize
    // its internal buffer and store all data inside.
    s->predecoded = predecode;
    if (predecode) {
        s->bytes_left = Sound_DecodeAll(s->sample);
        s->buffer = s->sample->buffer;
    }

    return s;
}

int NMIX_Rewind(NMIX_FileSource* s) {
    if (s == NULL) {
        return -1;
    }

    SDL_LockAudioDevice(NMIX_GetAudioDevice());
    s->bytes_left = 0;
    s->buffer = s->sample->buffer;

    if (s->predecoded) {
        s->bytes_left = s->sample->buffer_size;
    }
    else {
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

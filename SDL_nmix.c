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
static void nmix_callback(void* userdata, Uint8* _stream, int stream_len) {
    float* stream = (float*) _stream;

    if ((Uint32) stream_len != mixer.size) {
        fprintf(stderr, "PANIC: expected an audio buffer of %u samples,"
            " but got a buffer of %u.\n", mixer.size, (Uint32) stream_len);
        return;
    }

    int nb_samples = stream_len / SDL_AUDIO_SAMPLELEN(mixer.format) /
        mixer.channels;

    // retrieving audio data for each voice currently playing
    for (NMIX_Source* s = playing_sources; s != NULL; s = s->next) {
        s->callback(s->userdata, s->in_buffer, s->in_buffer_len);

        // if needed, we convert the audio data using SDL_AudioStream:
        if (s->needs_conversion) {
            int result = SDL_AudioStreamPut(s->stream, s->in_buffer,
                s->in_buffer_len);
            if (result != 0) {
                return;
            }
            int nb_bytes = SDL_AudioStreamGet(s->stream, s->out_buffer,
                s->out_buffer_len);
            if (nb_bytes != s->out_buffer_len) {
                return;
            }
        }
    }

    // mixing voices together (note: does not work on non-stereo audio spec)
    int ch = mixer.channels;
    for (int sample_no = 0; sample_no < nb_samples * ch; sample_no += ch) {
        float left = 0;
        float right = 0;

        // read sample for each voice currently playing:
        for (NMIX_Source* s = playing_sources; s != NULL; s = s->next) {
            // retrieve left/right samples from the source and apply gain/pan
            float* const buffer = (float*) s->out_buffer;
            float sl = buffer[sample_no] * s->gain;
            float sr = buffer[sample_no + 1] * s->gain;
            apply_panning(s->pan, &sl, &sr);

            // mix source samples with the others
            left = mix_samples(left, sl);
            right = mix_samples(right, sr);
        }

        // apply global gain
        stream[sample_no] = clampf(left * master_gain, -1, 1);
        stream[sample_no + 1] = clampf(right * master_gain, -1, 1);
    }

    // removing the sources that must be stopped from the playing_sources list
    for (NMIX_Source* s = playing_sources; s != NULL;) {
        NMIX_Source* next = s->next;
        if (s->remove) {
            NMIX_Pause(s);
        }
        s = next;
    }
}

int NMIX_OpenAudio(const char* device, int freq, int samples) {
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

    // internally, SDL_nmix uses float (AUDIO_F32SYS) samples
    // and only supports stereo mixing
    SDL_AudioSpec wanted_spec = {0};
    wanted_spec.freq = freq;
    wanted_spec.format = NMIX_DEFAULT_FORMAT;
    wanted_spec.channels = NMIX_DEFAULT_CHANNELS;
    wanted_spec.samples = samples;
    wanted_spec.callback = nmix_callback;
    wanted_spec.userdata = NULL;

    audio_device = SDL_OpenAudioDevice(device, 0, &wanted_spec, &mixer, 0);
    if (audio_device == 0) {
        return -1;
    }

    NMIX_PausePlayback(SDL_FALSE);

    return 0;
}

int NMIX_CloseAudio() {
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

float NMIX_GetMasterGain() {
    return master_gain;
}

void NMIX_SetMasterGain(float gain) {
    master_gain = clampf(gain, 0, 2);
}

SDL_AudioSpec* NMIX_GetAudioSpec() {
    return &mixer;
}

SDL_AudioDeviceID NMIX_GetAudioDevice() {
    return audio_device;
}

NMIX_Source* NMIX_NewSource(SDL_AudioFormat format, int channels, int freq,
        NMIX_SourceCallback callback, void* userdata) {
    if (audio_device == 0) {
        SDL_SetError("Please open NMIX device before creating sources.");
        return NULL;
    }

    NMIX_Source* source = SDL_malloc(sizeof(NMIX_Source));
    if (source == NULL) {
        SDL_SetError("Failed to allocate memory for NMIX_Source");
        return NULL;
    }

    source->format = format;
    source->channels = channels;
    source->freq = freq;
    source->pan = 0.f;
    source->gain = 1.f;
    source->callback = callback;
    source->userdata = userdata;

    // allocating the internal buffers:
    source->in_buffer_len = mixer.samples * SDL_AUDIO_SAMPLELEN(format) *
        channels;
    source->in_buffer = SDL_malloc(source->in_buffer_len);
    if (source->in_buffer == NULL) {
        SDL_SetError("Failed to allocate memory for NMIX_Source");
        return NULL;
    }
    SDL_memset(source->in_buffer, 0, source->in_buffer_len);

    source->stream = NULL;
    source->out_buffer_len = 0;
    source->out_buffer = source->in_buffer;
    source->needs_conversion = SDL_FALSE;

    if (source->format != mixer.format || source->channels != mixer.channels) {
        source->needs_conversion = SDL_TRUE;

        source->out_buffer_len = mixer.samples *
            SDL_AUDIO_SAMPLELEN(mixer.format) * mixer.channels;
        source->out_buffer = SDL_malloc(source->out_buffer_len);
        if (source->out_buffer == NULL) {
            SDL_SetError("Failed to allocate memory for NMIX_Source");
            return NULL;
        }
        SDL_memset(source->out_buffer, 0, source->out_buffer_len);

        source->stream = SDL_NewAudioStream(
            source->format,
            source->channels,
            source->freq,
            mixer.format,
            mixer.channels,
            mixer.freq);
        if (source->stream == NULL) {
            SDL_SetError("Failed to allocate memory for NMIX_Source");
            return NULL;
        }
    }

    source->prev = NULL;
    source->next = NULL;
    source->remove = SDL_FALSE;

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
    if (source->needs_conversion) {
        SDL_FreeAudioStream(source->stream);
        SDL_free(source->out_buffer);
    }
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

    // no sources currently playing, so we set the first source
    if (playing_sources == NULL) {
        playing_sources = source;
    }
    else {
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
    }
    else {
        // first element in list
        playing_sources = source->next;
    }

    source->prev = NULL;
    source->next = NULL;
    source->remove = SDL_FALSE;

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

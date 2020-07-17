/** \file SDL_nmix.h */

/**
 * \mainpage SDL_nmix
 *
 * SDL_nmix is a lightweight audio mixer for the SDL (2.0.7+) that supports
 * playback of both static and streaming sources. The code is written in C99
 * and available under the zlib license. It was made primarily for game
 * development. Features:

 * - stereo audio mixer
 * - only two files to copy to your project, under 800 lines of code
 * - free and open source under zlib license
 * - cross-platform: tested on macOS, debian, Windows and web (thanks to
 *   emscripten)
 * - a binding to [SDL_sound](https://hg.icculus.org/icculus/SDL_sound/) is
 *   provided, to decode the most usual file formats
 *   (ogg/wav/flac/mp3/mod/xm/it/etc), with seamless looping. The files can be
 *   either preloaded into memory or streamed.
 * - automatic audio conversion on the fly
 * - linear panning + gain setting on each source
 * - a global gain setting
 *
 * The library depends on the SDL (2.0.7+) which you can find
 * [here](https://www.libsdl.org/). Just copy SDL_nmix.c/.h in your project,
 * and you're good to go. If you want to use the binding to SDL_sound, copy
 * `SDL_nmix_file.c` and `SDL_nmix_file.h` too.
 *
 * Checkout the folder "examples" for some examples, or jump right in:
 *
 * \sa NMIX_OpenAudio
 * \sa NMIX_NewSource
 * \sa NMIX_Play
 *
 */

#ifndef SDL_NMIX_H
#define SDL_NMIX_H

#include <stdio.h>
#include <SDL.h>

// SDL_nmix requires SDL 2.0.7+ because of its use of SDL_AudioStream
#if !SDL_VERSION_ATLEAST(2, 0, 7)
#error SDL_nmix requires SDL 2.0.7 or later.
#endif

#define NMIX_VER_MAJOR 1 /**< SDL_nmix version (major). */
#define NMIX_VER_MINOR 1 /**< SDL_nmix version (minor). */
#define NMIX_VER_PATCH 0 /**< SDL_nmix version (patch). */

#define NMIX_DEFAULT_FREQUENCY 44100 /**< The default sampling rate. */
#define NMIX_DEFAULT_SAMPLES \
  4096 /**< The default audio buffer size \
          (in sample frames) */
#define NMIX_DEFAULT_DEVICE \
  NULL /**< The default audio device to use (NULL \
            requests the most reasonable default). */

/**
 *  This macro returns the number of bytes per sample for a given
 *  SDL_AudioFormat.
 */
#define SDL_AUDIO_SAMPLELEN(x) (((x) &SDL_AUDIO_MASK_BITSIZE) / 8)

/**
 *  This function is called when SDL_nmix needs more data for a source.
 *
 *  \param userdata An application-specific parameter saved in
 *                  the NMIX_Source structure
 *  \param stream A pointer to the audio buffer.
 *  \param len The length of that buffer in bytes.
 *
 *  Once the callback returns, the buffer will no longer be valid.
 *  Stereo samples are stored in a LRLRLR ordering.
 *
 */
typedef void(SDLCALL* NMIX_SourceCallback)(
    void* userdata, void* stream, int stream_size);

/**
 * \struct NMIX_Source
 * \brief Represents a sound source that can be played.
 *
 * Every field in this struct should be considered read-only, you should
 * call NMIX_* functions to modify the source state.
 *
 * \sa NMIX_NewSource
 */
typedef struct NMIX_Source {
  int rate; /**< The sampling rate of the source (samples per second). */
  SDL_AudioFormat format; /**< The source format. */
  Uint8 channels; /**< The number of channels of the source. */
  float pan; /**< The panning of the source (-1 < pan < 1, default = 0). */
  float gain; /**< The gain of the source (0 < gain < 2, default = 1). */

  NMIX_SourceCallback callback; /**< Callback used to retrieve data. */
  void* userdata; /**< User-defined pointer that is passed to callback. */
  SDL_bool eof; /**< Flag set if the source has no more data to play.
                     This flag must be set to 1 in the NMIX_SourceCallback
                     for SDL_nmix to stop the source playback. */

  void* in_buffer; /**< Internal audio buffer modified by the callback. */
  int in_buffer_size; /**< Size in bytes of in_buffer. */
  SDL_AudioStream* stream; /**< Used to convert audio data on the fly. */
  void* out_buffer; /**< Internal audio buffer holding the converted data. */
  int out_buffer_size; /**< Size in bytes of out_buffer. */

  struct NMIX_Source* prev; /**< Previous source in list. */
  struct NMIX_Source* next; /**< Next source in list. */
} NMIX_Source;

/**
 * \fn int NMIX_OpenAudio(const char* device, int freq, int samples)
 * \brief Opens an audio device and initializes SDL_nmix.
 *
 * This should be called once before any other SDL_nmix call.
 * Passing in a device name of NULL requests the most reasonable default.
 *
 * If you're unsure about what to pass here, you can use SDL_nmix defaults:
 *
 * \code
 * int result = NMIX_OpenAudio(NMIX_DEFAULT_DEVICE, NMIX_DEFAULT_FREQUENCY,
 *                             NMIX_DEFAULT_SAMPLES);
 * \endcode
 *
 *    \param device A UTF-8 string reported by SDL_GetAudioDeviceName() (or
 *                  NULL to get the most reasonable default)
 *    \param rate The sampling rate (samples per second)
 *    \param samples Audio buffer size in sample frames (total samples
 *           divided by channel count)
 *   \return zero on success, -1 on error. You can retrieve the error message
 *           with a call to SDL_GetError()
 *
 * \sa NMIX_CloseAudio
 */
int NMIX_OpenAudio(const char* device, int rate, int samples);

/**
 * \fn int NMIX_CloseAudio()
 * \brief Closes the audio device.
 *
 * This function should be called once at the end of the program,
 * before SDL_Quit().
 *
 *   \return zero on success, -1 on error. You can retrieve the error message
 *           with a call to SDL_GetError()
 *
 * \sa NMIX_OpenAudio
 */
int NMIX_CloseAudio();

/**
 * \fn void NMIX_PausePlayback(SDL_bool pause_on)
 * \brief Pauses (or resumes) the playback of the audio device.
 *
 * This function affects the SDL audio device, meaning that it impacts every
 * NMIX_Source.
 *
 *    \param pause_on Whether the playback should be paused or not.
 */
void NMIX_PausePlayback(SDL_bool pause_on);

/**
 * \fn float NMIX_GetMasterGain()
 * \brief Returns the master gain.
 *
 * This function returns the master gain. The default is 1 (= 100%).
 *
 *   \return The master gain (between 0 and 2)
 *
 * \sa NMIX_SetMasterGain
 */
float NMIX_GetMasterGain();

/**
 * \fn void NMIX_SetMasterGain(float gain)
 * \brief Sets the master gain.
 *
 * This function sets the master gain.
 * The default is 1 (=100%), 0 mutes the sound.
 *
 *    \param gain The master gain (between 0 and 2)
 *
 * \sa NMIX_GetMasterGain
 */
void NMIX_SetMasterGain(float gain);

/**
 * \fn SDL_AudioSpec* NMIX_GetAudioSpec()
 * \brief Returns the internal audio spec used by SDL_nmix.
 *
 *   \return The audio spec
 *
 */
SDL_AudioSpec* NMIX_GetAudioSpec();

/**
 * \fn SDL_AudioDeviceID NMIX_GetAudioDevice()
 * \brief Returns the internal audio device id used by SDL_nmix.
 *
 *   \return The audio device id
 *
 */
SDL_AudioDeviceID NMIX_GetAudioDevice();

/**
 * \fn NMIX_Source* NMIX_NewSource(SDL_AudioFormat format, Uint8 channels,
 *         int rate, NMIX_SourceCallback callback, void* userdata);
 * \brief Creates a new NMIX_Source.
 *
 * The audio format of the source (and the number of channels) must be
 * passed along with a callback function that will be called when SDL_nmix
 * needs more data during the playback of the source.
 *
 * This is an example of callback for a mono source with AUDIO_F32SYS format:
 *
 * \code
 * void my_callback(void* userdata, void* _stream, int stream_size) {
 *     float* stream = (float*) _stream;
 *
 *     for (size_t i = 0; i < stream_size / sizeof(float); i++) {
 *         stream[i] = random_float();
 *     }
 * }
 * \endcode
 *
 * The callback must completely initialize the buffer: this buffer is not
 * initialized before the callback is called. If there is nothing to play,
 * the callback should fill the buffer with silence.
 *
 * SDL_nmix internally uses AUDIO_F32SYS for mixing the sources together. If
 * your source is in another format, SDL_nmix will automatically convert your
 * audio data on the fly.
 *
 * If the source has more than 1 channel, the audio data must be
 * interleaved (LRLRLR ordering).
 *
 *    \param format The format of the source samples
 *    \param channels The number of channels of the source
 *    \param rate The sampling rate of the source
 *    \param callback Callback function, called when more audio data is needed
 *    \param userdata Userdata passed to callback
 *   \return zero on success, -1 on error. You can retrieve the error message
 *           with a call to SDL_GetError()
 *
 * \sa NMIX_SourceCallback
 * \sa NMIX_FreeSource
 */
NMIX_Source* NMIX_NewSource(SDL_AudioFormat format, Uint8 channels, int rate,
    NMIX_SourceCallback callback, void* userdata);

/**
 * \fn void NMIX_FreeSource(NMIX_Source* source)
 * \brief Frees a NMIX_Source from memory.
 *
 * If the source is currently playing, NMIX_FreeSource will automatically
 * pause it before freeing it.
 *
 *    \param source The source to free
 *
 * \sa NMIX_NewSource
 */
void NMIX_FreeSource(NMIX_Source* source);

/**
 * \fn int NMIX_Play(NMIX_Source* source)
 * \brief Plays a NMIX_Source.
 *
 * Note that you cannot play a single source multiple times simultaneously.
 *
 *    \param source The source to be played
 *   \return zero on success, -1 on error. You can retrieve the error message
 *           with a call to SDL_GetError()
 *
 * \sa NMIX_Pause
 */
int NMIX_Play(NMIX_Source* source);

/**
 * \fn void NMIX_Pause(NMIX_Source* source)
 * \brief Pauses a NMIX_Source.
 *
 * A NMIX_Source can be resumed with a call to NMIX_Play.
 *
 *    \param source The source to be paused
 *
 * \sa NMIX_Play
 */
void NMIX_Pause(NMIX_Source* source);

/**
 * \fn SDL_bool NMIX_IsPlaying(NMIX_Source* source)
 * \brief Returns whether a NMIX_Source is currently playing.
 *
 *    \param source The source to query
 *   \return 1 if the source is playing, 0 if the source is not playing
 *
 * \sa NMIX_Play
 */
SDL_bool NMIX_IsPlaying(NMIX_Source* source);

/**
 * \fn float NMIX_GetPan(NMIX_Source* source)
 * \brief Returns the panning of a NMIX_Source.
 *
 * Default panning is 0 (center), and can be set from -1 (left) to 1 (right).
 *
 *    \param source The source to query
 *   \return The panning setting for this source (between -1 and 1)
 *
 * \sa NMIX_SetPan
 */
float NMIX_GetPan(NMIX_Source* source);

/**
 * \fn void NMIX_SetPan(NMIX_Source* source, float pan)
 * \brief Sets linear stereo panning of a NMIX_Source.
 *
 * This panning will be applied while mixing all sources together, which means
 * that all sources (even mono sources) can be panned.
 *
 *    \param source The source to pan
 *    \param pan The panning setting for this source (between -1 and 1)
 *
 * \sa NMIX_GetPan
 */
void NMIX_SetPan(NMIX_Source* source, float pan);

/**
 * \fn float NMIX_GetGain(NMIX_Source* source)
 * \brief Returns the gain of a NMIX_Source.
 *
 * Default gain is 1 (100%), and can be set from 0 (muted) to 2 (200%).
 *
 *    \param source The source to query
 *   \return The gain setting for this source (between 0 and 2)
 *
 * \sa NMIX_SetGain
 */
float NMIX_GetGain(NMIX_Source* source);

/**
 * \fn void NMIX_SetGain(NMIX_Source* source, float gain)
 * \brief Sets the gain of a NMIX_Source.
 *
 * Default gain is 1 (100%), and can be set from 0 (muted) to 2 (200%).
 *
 *    \param source The source to pan
 *    \param gain The gain setting for this source (between 0 and 2)
 *
 * \sa NMIX_GetGain
 */
void NMIX_SetGain(NMIX_Source* source, float gain);

#endif // SDL_NMIX_H

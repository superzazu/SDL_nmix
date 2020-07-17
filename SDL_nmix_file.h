/** \file SDL_nmix_file.h */

#ifndef SDL_NMIX_FILE_H
#define SDL_NMIX_FILE_H

#include <SDL_sound.h>
#include "SDL_nmix.h"

/**
 * \struct NMIX_FileSource
 * \brief Represents a source that is decoded from a file.
 *
 * The decoding is done by SDL_sound.
 * Every field in this struct should be considered read-only, you should
 * call NMIX_* functions to modify the source state.
 *
 * \sa NMIX_NewFileSource
 */
typedef struct NMIX_FileSource {
  SDL_RWops* rw; /**< Pointer to the SDL_RWops used when decoding. */
  const char* ext; /**< The file extension. */
  Sound_Sample* sample; /**< The SDL_sound struct used to decode. */
  NMIX_Source* source; /**< The NMIX_Source source. */
  SDL_bool loop_on; /**< Whether the source should be looped or not. */
  Uint8* buffer; /**< Pointer to the SDL_sound buffer, at the current
                      position of decoding. */
  int bytes_left; /**< Number of bytes left to read in SDL_sound buffer. */
  SDL_bool predecoded; /**< Set if the source is pre-decoded in memory. */
} NMIX_FileSource;

/**
 * \fn NMIX_FileSource* NMIX_NewFileSource(SDL_RWops* rw, const char *ext,
 *         SDL_bool predecode)
 * \brief Creates a new NMIX_FileSource.
 *
 * Creates a source that will be decoded by SDL_sound. The sound can be
 * either pre-decoded in memory or streamed while playing. Warning: do not
 * create multiple NMIX_FileSource with the same SDL_RWops. The SDL_RWops
 * will automatically be closed/freed on NMIX_FreeFileSource().
 *
 *    \param rw A SDL_RWops that points to the file to decode
 *    \param ext The file extension (without the point '.'), eg "ogg"
 *    \param predecode Whether the source should be predecoded in memory (1),
 *           or streamed while playing (0)
 *   \return zero on success, NULL on error. You can retrieve the error
 *           message with a call to SDL_GetError()
 *
 * \sa NMIX_FreeFileSource
 * \sa NMIX_GetDuration
 * \sa NMIX_Seek
 * \sa NMIX_Rewind
 * \sa NMIX_SetLoop
 */
NMIX_FileSource* NMIX_NewFileSource(
    SDL_RWops* rw, const char* ext, SDL_bool predecode);

/**
 * \fn Sint32 NMIX_GetDuration(NMIX_FileSource* s)
 * \brief Returns the duration (in milliseconds) of a NMIX_FileSource.
 *
 *    \param s The file source to query
 *   \return the duration in milliseconds, -1 on error. You can retrieve
 *           the error message with a call to SDL_GetError()
 */
Sint32 NMIX_GetDuration(NMIX_FileSource* s);

/**
 * \fn int NMIX_Seek(NMIX_FileSource* s, int ms)
 * \brief Modifies a NMIX_FileSource position.
 *
 *    \param s The file source to seek
 *   \param ms The new position in milliseconds from the beginning of the source
 *   \return zero on success, -1 on error. You can retrieve the error message
 *           with a call to SDL_GetError()
 */
int NMIX_Seek(NMIX_FileSource* s, int ms);

/**
 * \fn int NMIX_Rewind(NMIX_FileSource* s)
 * \brief Rewinds a NMIX_FileSource.
 *
 *    \param s The file source to rewind
 *   \return zero on success, -1 on error. You can retrieve the error message
 *           with a call to SDL_GetError()
 */
int NMIX_Rewind(NMIX_FileSource* s);

/**
 * \fn SDL_bool NMIX_GetLoop(NMIX_FileSource* s)
 * \brief Returns whether a NMIX_FileSource is looped or not.
 *
 *    \param s The file source to query
 *   \return Whether the file source is looped (1) or not (0)
 *
 * \sa NMIX_SetLoop
 */
SDL_bool NMIX_GetLoop(NMIX_FileSource* s);

/**
 * \fn void NMIX_SetLoop(NMIX_FileSource* s, SDL_bool loop_on)
 * \brief Sets whether a NMIX_FileSource is looped or not.
 *
 *    \param s The file source to query.
 *    \param loop_on Whether the file source is looped (1) or not (0).
 *
 * \sa NMIX_GetLoop
 */
void NMIX_SetLoop(NMIX_FileSource* s, SDL_bool loop_on);

/**
 * \fn void NMIX_FreeFileSource(NMIX_FileSource* s)
 * \brief Frees a NMIX_FileSource from memory.
 *
 * Note that the SDL_RWops is automatically closed/freed on call.
 *
 *    \param s The file source to free.
 *
 * \sa NMIX_NewFileSource
 */
void NMIX_FreeFileSource(NMIX_FileSource* s);

#endif // SDL_NMIX_FILE_H

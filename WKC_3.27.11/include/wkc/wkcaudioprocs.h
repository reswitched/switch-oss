/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef _WKCAUDIOPROCS_H_
#define _WKCAUDIOPROCS_H_

typedef bool (*wkcAudioInitializeProc)(void);
typedef void (*wkcAudioFinalizeProc)(void);
typedef void (*wkcAudioForceTerminateProc)(void);
typedef void* (*wkcAudioOpenProc)(int in_samplerate, int in_bitspersample, int in_channels, int in_endian);
typedef void (*wkcAudioCloseProc)(void* in_self);
typedef unsigned int (*wkcAudioWriteProc)(void* in_self, void* in_data, unsigned int in_len);
typedef bool (*wkcAudioWriteRawProc)(void* in_self, float** in_data, unsigned int in_channels, unsigned int in_frame_count, float* in_max_abs_value);
typedef int (*wkcAudioPreferredSampleRateProc)(void);

struct WKCAudioProcs_ {
    wkcAudioInitializeProc fAudioInitializeProc;
    wkcAudioFinalizeProc fAudioFinalizeProc;
    wkcAudioForceTerminateProc fAudioForceTerminateProc;
    wkcAudioOpenProc fAudioOpenProc;
    wkcAudioCloseProc fAudioCloseProc;
    wkcAudioWriteProc fAudioWriteProc;
    wkcAudioWriteRawProc fAudioWriteRawProc;
    wkcAudioPreferredSampleRateProc fAudioPreferredSampleRateProc;
};
typedef struct WKCAudioProcs_ WKCAudioProcs;

#endif // _WKCAUDIOPROCS_H_

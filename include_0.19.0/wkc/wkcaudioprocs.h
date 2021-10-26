/*
 *  wkcaudioprocs.h
 *
 *  Copyright(c) 2016-2017 ACCESS CO., LTD. All rights reserved.
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

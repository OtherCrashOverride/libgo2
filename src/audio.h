#pragma once

#include <stdint.h>


typedef struct go2_audio go2_audio_t;

typedef enum 
{
    Audio_Path_Off = 0,
    Audio_Path_Speaker,
    Audio_Path_Headphones,
    Audio_Path_SpeakerAndHeadphones,

    Audio_Path_MAX = 0x7fffffff
} go2_audio_path_t;


#ifdef __cplusplus
extern "C" {
#endif

go2_audio_t* go2_audio_create(int frequency);
void go2_audio_destroy(go2_audio_t* audio);
void go2_audio_submit(go2_audio_t* audio, const short* data, int frames);
uint32_t go2_audio_volume_get(go2_audio_t* audio);
void go2_audio_volume_set(go2_audio_t* audio, uint32_t value);
go2_audio_path_t go2_audio_path_get(go2_audio_t* audio);
void go2_audio_path_set(go2_audio_t* audio, go2_audio_path_t value);

#ifdef __cplusplus
}
#endif

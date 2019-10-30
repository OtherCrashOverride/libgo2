#pragma once


typedef struct go2_audio go2_audio_t;


#ifdef __cplusplus
extern "C" {
#endif

go2_audio_t* go2_audio_create(int frequency);
void go2_audio_destroy(go2_audio_t* audio);
void go2_audio_submit(go2_audio_t* audio, const short* data, int frames);

#ifdef __cplusplus
}
#endif

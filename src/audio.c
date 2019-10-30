#include "audio.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SOUND_SAMPLES_SIZE  (2048)
#define SOUND_CHANNEL_COUNT 2


typedef struct go2_audio
{
    ALsizei frequency;
    ALCdevice *device;
    ALCcontext *context;
    ALuint source;
    bool isAudioInitialized;
} go2_audio_t;


go2_audio_t* go2_audio_create(int frequency)
{
    go2_audio_t* result = malloc(sizeof(*result));
    if (!result)
    {
        printf("malloc failed.\n");
        goto out;
    }

    memset(result, 0, sizeof(*result));


    result->frequency = frequency;

	result->device = alcOpenDevice(NULL);
	if (!result->device)
	{
		printf("alcOpenDevice failed.\n");
		goto err_00;
	}

	result->context = alcCreateContext(result->device, NULL);
	if (!alcMakeContextCurrent(result->context))
	{
		printf("alcMakeContextCurrent failed.\n");
		goto err_01;
	}

	alGenSources((ALuint)1, &result->source);

	alSourcef(result->source, AL_PITCH, 1);
	alSourcef(result->source, AL_GAIN, 1);
	alSource3f(result->source, AL_POSITION, 0, 0, 0);
	alSource3f(result->source, AL_VELOCITY, 0, 0, 0);
	alSourcei(result->source, AL_LOOPING, AL_FALSE);

	//memset(audioBuffer, 0, AUDIOBUFFER_LENGTH * sizeof(short));

	const int BUFFER_COUNT = 4;
	for (int i = 0; i < BUFFER_COUNT; ++i)
	{
		ALuint buffer;
		alGenBuffers((ALuint)1, &buffer);
		alBufferData(buffer, AL_FORMAT_STEREO16, NULL, 0, frequency);
		alSourceQueueBuffers(result->source, 1, &buffer);
	}

	alSourcePlay(result->source);

    result->isAudioInitialized = true;

    return result;


err_01:
    alcCloseDevice(result->device);

err_00:
    free(result);

out:
    return NULL;
}

void go2_audio_destroy(go2_audio_t* audio)
{
    alDeleteSources(1, &audio->source);
    alcDestroyContext(audio->context);
    alcCloseDevice(audio->device);

    free(audio);
}

void go2_audio_submit(go2_audio_t* audio, const short* data, int frames)
{
    if (!audio->isAudioInitialized) return;


    if (!alcMakeContextCurrent(audio->context))
    {
        printf("alcMakeContextCurrent failed.\n");
        return;
    }

    ALint processed = 0;
    while(!processed)
    {
        alGetSourceiv(audio->source, AL_BUFFERS_PROCESSED, &processed);

        if (!processed)
        {
            sleep(0);
            //printf("Audio overflow.\n");
            //return;
        }
    }

    ALuint openALBufferID;
    alSourceUnqueueBuffers(audio->source, 1, &openALBufferID);

    ALuint format = AL_FORMAT_STEREO16;

    int dataByteLength = frames * sizeof(short) * SOUND_CHANNEL_COUNT;
    alBufferData(openALBufferID, format, data, dataByteLength, audio->frequency);

    alSourceQueueBuffers(audio->source, 1, &openALBufferID);

    ALint result;
    alGetSourcei(audio->source, AL_SOURCE_STATE, &result);

    if (result != AL_PLAYING)
    {
        alSourcePlay(audio->source);
    }
}

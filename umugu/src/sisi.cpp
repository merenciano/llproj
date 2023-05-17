#include "sisi.h"
#include <stdio.h>
#include <math.h>
#include <portaudio.h>

#define SAMPLE_RATE 44100
#define TABLE_SIZE 200

struct AudioCallbackData
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
};

static PaStreamParameters output_params;
static PaStream *stream;
static PaError err;
static AudioCallbackData data;

static void Terminate()
{
    Pa_Terminate();
    fprintf(stderr, "An error occurred while using the portaudio stream\n" );
    fprintf(stderr, "Error number: %d\n", err );
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    exit(err);
}

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    AudioCallbackData *data = (AudioCallbackData*)userData;
    float *out = (float*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused argument warning. */
    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = data->sine[data->left_phase];  /* left */
        *out++ = data->sine[data->right_phase];  /* right */
        data->left_phase += 1;
        if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
        data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
        if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
    }
    return 0;
}

namespace sisi
{
    void Init()
    {
        for(int i = 0; i < TABLE_SIZE; ++i)
        {
            data.sine[i] = (float) sin(((float)i/(float)TABLE_SIZE) * M_PI * 2.0f);
        }
        data.left_phase = 0;
        data.right_phase = 0;

        err = Pa_Initialize();
        if( err != paNoError ) Terminate();

        output_params.device = Pa_GetDefaultOutputDevice(); /* default output device */
        if (output_params.device == paNoDevice) {
            fprintf(stderr,"Error: No default output device.\n");
            Terminate();
        }
        output_params.channelCount = 2;                     /* stereo output */
        output_params.sampleFormat = paFloat32;             /* 32 bit floating point output */
        output_params.suggestedLatency = Pa_GetDeviceInfo( output_params.device )->defaultLowOutputLatency;
        output_params.hostApiSpecificStreamInfo = NULL;
    }

    void StartAudioStream()
    {
        err = Pa_OpenStream(&stream,
                            NULL,              /* No input. */
                            &output_params, /* As above. */
                            SAMPLE_RATE,
                            256,               /* Frames per buffer. */
                            paClipOff,         /* No out of range samples expected. */
                            patestCallback,
                            &data );
        if( err != paNoError ) Terminate();

        err = Pa_StartStream( stream );
        if( err != paNoError ) Terminate();
    }

    void Close()
    {
        err = Pa_CloseStream( stream );
        if( err != paNoError ) Terminate();
        Pa_Terminate();
    }
}

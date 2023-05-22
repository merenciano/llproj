#include "umugu.h"
#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <portaudio.h>

static PaStreamParameters output_params;
static PaStream *stream;
static PaError err;

static void Terminate()
{
	Pa_Terminate();
	fprintf(stderr, "An error occurred while using the portaudio stream\n" );
	fprintf(stderr, "Error number: %d\n", err );
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	exit(err);
}

static int AudioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	umugu::AudioCallbackData *data = (umugu::AudioCallbackData*)userData;
	float *out = (float*)outputBuffer;
	for (unsigned long i = 0; i < framesPerBuffer; i++) {
		*out = 0.0f;
		*(out + 1) = 0.0f;
		for (int j = 0; j < data->wave_count; ++j)
		{
			*out += data->sine[data->left_phase[j]];
			*(out + 1) += data->sine[data->right_phase[j]];
			data->left_phase[j] += data->freq[j];
			data->right_phase[j] += data->freq[j];
			if (data->left_phase[j] >= SAMPLE_RATE)
				data->left_phase[j] -= SAMPLE_RATE;

			if (data->right_phase[j] >= SAMPLE_RATE)
				data->right_phase[j] -= SAMPLE_RATE;
		}
		*out++ /= data->wave_count;
		*out++ /= data->wave_count;
	}

	return 0;
}

namespace umugu 
{
	AudioCallbackData data;
	Window window;

	void Init()
	{
		data.wave_count = 1;
		for (int i = 0; i < MAX_WAVES; ++i) {
			data.freq[i] = 440;
			data.left_phase[i] = 0;
			data.right_phase[i] = 0;
		}
		
		for (int i = 0; i < SAMPLE_RATE; ++i) {
			data.sine[i] = sinf((i/(float)SAMPLE_RATE) * M_PI * 2.0f);
		}

                err = Pa_Initialize();
                if (err != paNoError)
                        Terminate();

                output_params.device = Pa_GetDefaultOutputDevice();
                if (output_params.device == paNoDevice) {
                        fprintf(stderr, "Error: No default output device.\n");
                        Terminate();
                }

                output_params.channelCount = 2;
                output_params.sampleFormat = paFloat32;
                output_params.suggestedLatency =
                    Pa_GetDeviceInfo(output_params.device)
                        ->defaultLowOutputLatency;
                output_params.hostApiSpecificStreamInfo = NULL;

		InitUI();
        }

        void StartAudioStream() {
                err = Pa_OpenStream(&stream, NULL, &output_params, SAMPLE_RATE,
                                    FRAMES_PER_BUFFER, paClipOff, AudioCallback,
                                    &data);
                if (err != paNoError)
			Terminate();

		err = Pa_StartStream(stream);
		if (err != paNoError)
			Terminate();
        }

	bool PollEvents()
	{
		return PollWindowEvents();
	}

	void Render()
	{
		DrawUI();
	}

        void Close()
	{
		err = Pa_CloseStream(stream);
		if (err != paNoError)
			Terminate();
		Pa_Terminate();

		CloseUI();
	}
}

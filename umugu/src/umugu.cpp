#include "umugu.h"
#include "node.h"
#include "ui.h"
#include "node.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
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

static int AudioCallback(const void *in, void *out, unsigned long fpb, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	memcpy(out, umugu::Process(umugu::last), sizeof(umugu::WaveValue) * FRAMES_PER_BUFFER);

	/*
	umugu::AudioCallbackData *data = (umugu::AudioCallbackData*)userData;
	float *out = (float*)outputBuffer;
	for (unsigned long i = 0; i < framesPerBuffer; i++)
	{
		*out = 0.0f;
		*(out + 1) = 0.0f;
		for (int j = 0; j < data->wave_count; ++j)
		{
			*out += wave_table[data->wave_shape[j]][data->left_phase[j]];
			*(out + 1) += wave_table[data->wave_shape[j]][data->right_phase[j]];
			data->left_phase[j] += data->freq[j];
			data->right_phase[j] += data->freq[j];
			if (data->left_phase[j] >= SAMPLE_RATE)
			{
				data->left_phase[j] -= SAMPLE_RATE;
			}

			if (data->right_phase[j] >= SAMPLE_RATE)
			{
				data->right_phase[j] -= SAMPLE_RATE;
			}
		}
		*out++ /= data->wave_count;
		*out++ /= data->wave_count;
	}
	*/

	return 0;
}

namespace umugu 
{
	AudioCallbackData data;
	Window window;
	float plot_x[SAMPLE_RATE];

	const char *const SHAPE_NAMES[] = {
		"SINE",
		"SAW",
		"SQUARE",
		"TRIANGLE",
		"WHITE_NOISE"
	};

	float *WaveTable(WaveShape ws)
	{
		return wave_table[ws];
	}

	void Init()
	{
		data.wave_count = 1;
		for (int i = 0; i < MAX_WAVES; ++i)
		{
			data.freq[i] = 440;
			data.left_phase[i] = 0;
			data.right_phase[i] = 0;
			data.wave_shape[i] = WS_SINE;
		}


		float *x = plot_x;
		float value = 0.0f;
		float fsample_rate = (float)SAMPLE_RATE;

		do
		{
			*x = value++;
		}
		while (*x++ < fsample_rate);

		err = Pa_Initialize();
		if (err != paNoError)
		{
			Terminate();
		}

		output_params.device = Pa_GetDefaultOutputDevice();
		if (output_params.device == paNoDevice)
		{
			fprintf(stderr, "Error: No default output device.\n");
			Terminate();
		}

		output_params.channelCount = 2;
		output_params.sampleFormat = paFloat32;
		output_params.suggestedLatency = Pa_GetDeviceInfo(output_params.device)->defaultLowOutputLatency;
		output_params.hostApiSpecificStreamInfo = NULL;

		GenWaveTable();
		last = CreateNode<Osciloscope>();
		InitUI();
	}

	void StartAudioStream()
	{
		err = Pa_OpenStream(&stream, NULL, &output_params, SAMPLE_RATE,
							FRAMES_PER_BUFFER, paClipOff, AudioCallback,
							&data);
		if (err != paNoError)
		{
			Terminate();
		}

		err = Pa_StartStream(stream);
		if (err != paNoError)
		{
			Terminate();
		}
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
		{
			Terminate();
		}
		Pa_Terminate();
		CloseUI();
	}
}

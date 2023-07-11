#include "umugu.h"
#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <portaudio.h>

#define UMUGU_IMPLEMENTATION
#include "umugu_single.h"


umugu_unit graph_fx;

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

static int AudioCallback(const void *in, void *out, unsigned long fpb, const PaStreamCallbackTimeInfo *timeinfo, PaStreamCallbackFlags flags, void *data)
{
	void *wave = umugu_getwave(graph_fx);
	memcpy(out, wave, sizeof(umugu_wave) * UMUGU_FRAMES_PER_BUFFER);
	return 0;
}

namespace umugu 
{
	Window window;
	float plot_x[UMUGU_SAMPLE_RATE];

	const char *const SHAPE_NAMES[] = {
		"SINE",
		"SAW",
		"SQUARE",
		"TRIANGLE",
		"WHITE_NOISE"
	};

	void Init()
	{
		float *x = plot_x;
		float value = 0.0f;
		float fsample_rate = (float)UMUGU_SAMPLE_RATE;

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

		umugu_init();
		static umugu_osciloscope_data osc_data[2];
		osc_data[0]._phase = 0;
		osc_data[0].freq = 220;
		osc_data[0].shape = UMUGU_WS_SINE;

		osc_data[1]._phase = 0;
		osc_data[1].freq = 440;
		osc_data[1].shape = UMUGU_WS_SINE;


		graph_fx = umugu_newunit(UMUGU_NT_MIX, NULL, NULL);
		umugu_newunit(UMUGU_NT_OSCILOSCOPE, &osc_data[0], graph_fx);
		umugu_newunit(UMUGU_NT_OSCILOSCOPE, &osc_data[1], graph_fx);
		InitUI();
	}

	void StartAudioStream()
	{
		err = Pa_OpenStream(&stream, NULL, &output_params, UMUGU_SAMPLE_RATE,
							UMUGU_FRAMES_PER_BUFFER, paClipOff, AudioCallback,
							NULL);
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

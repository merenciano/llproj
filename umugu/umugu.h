/*
	TODO: 
	- Allocator (PerBufferAlloc, PersistentAlloc) Only those 2 are needed.
	- Data organization and serialization.
*/

#ifndef UMUGU_H
#define UMUGU_H

#ifndef UMUGU_DEF
#ifdef UMUGU_STATIC
#define UMUGU_DEF static
#else
#ifdef __cplusplus
#define UMUGU_DEF extern "C"
#else
#define UMUGU_DEF extern
#endif
#endif
#endif

#include <stdint.h>

#ifndef UMUGU_FRAMES_PER_BUFFER
#define UMUGU_FRAMES_PER_BUFFER 60
#endif

#ifndef UMUGU_SAMPLE_RATE
#define UMUGU_SAMPLE_RATE 44100
#endif

typedef int umugu_unit;

typedef union {
	char chars[8];
	char *label;
	int64_t hash;
} umugu_label;

typedef struct {
	umugu_unit unit, in;	
} umugu_link;

typedef enum {
	UMUGU_NT_OSCILOSCOPE = 0,
	UMUGU_NT_MIX,
	UMUGU_NT_COUNT
} umugu_type;

typedef enum {
	UMUGU_WS_SINE = 0,
	UMUGU_WS_SAW,
	UMUGU_WS_SQUARE,
	UMUGU_WS_TRIANGLE,
	UMUGU_WS_WHITE_NOISE,
	UMUGU_WS_COUNT
} umugu_shape;

typedef struct {
	float left, right;
} umugu_wave;

typedef struct {
	umugu_shape shape;
	int freq;
	int _phase;
} umugu_osciloscope_data;

#undef N
#define N 16
typedef struct {
	umugu_type type[N];
	umugu_wave *out[N];
	umugu_link link[N];
	void *data[N];
	umugu_label label[N]; // TODO: Make optional.
} umugu_scene;

UMUGU_DEF void umugu_init();
UMUGU_DEF void umugu_close();
UMUGU_DEF void umugu_start_stream();
UMUGU_DEF void umugu_stop_stream();
UMUGU_DEF umugu_unit umugu_newunit(umugu_type type, void *data, umugu_unit parent);
UMUGU_DEF umugu_wave *umugu_output();
UMUGU_DEF float *umugu_wave_table(umugu_shape shape); // TODO: Make osciloscope with this table optionals (for input audio).
UMUGU_DEF const umugu_scene *umugu_scene_data();
UMUGU_DEF int32_t umugu_scene_count();

#endif

#ifdef UMUGU_IMPLEMENTATION

#include <stdlib.h>
#include <math.h>

static umugu_wave umugu__mem[UMUGU_FRAMES_PER_BUFFER * 2];
static umugu_wave *umugu__mem_it;

static void umugu__alloc_free()
{
	umugu__mem_it = umugu__mem;
} 

static umugu_wave *umugu__alloc_wave_buffer()
{
	umugu_wave *ret = umugu__mem_it;
	umugu__mem_it += UMUGU_FRAMES_PER_BUFFER;
	return ret;
}

static void umugu__init_backend();
static void umugu__close_backend();

static float umugu__wave_table[UMUGU_WS_COUNT][UMUGU_SAMPLE_RATE];
static umugu_scene umugu__scene;
static umugu_unit umugu__last;
static int32_t umugu__link_count;

static umugu_wave *umugu__process(umugu_unit unit)
{
	umugu_wave *out = NULL;
	switch (umugu__scene.type[unit])
	{
		case UMUGU_NT_OSCILOSCOPE:
		{
			umugu_osciloscope_data *data = (umugu_osciloscope_data*)umugu__scene.data[unit];
			out = umugu__alloc_wave_buffer();
			for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
			{
				out[i].left = umugu__wave_table[data->shape][data->_phase];
				out[i].right = umugu__wave_table[data->shape][data->_phase];
				data->_phase += data->freq;
				if (data->_phase >= UMUGU_SAMPLE_RATE)
				{
					data->_phase -= UMUGU_SAMPLE_RATE;
				}
			}
			break;
		}

		case UMUGU_NT_MIX:
		{
			umugu_unit in[16]; // 16 just to be sure. TODO: efficient custom alloc.
			int count = 0;

			for (int i = 0; umugu__scene.link[i].unit != umugu__last; ++i)
			{
				if (umugu__scene.link[i].unit == unit)
				{
					in[count] = umugu__scene.link[i].in;
					count++;
				}
			}

			out = umugu__scene.out[in[0]];
			for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
			{
				for (int j = 1; j < count; ++j)
				{
					out[i].left += umugu__scene.out[in[j]][i].left;
					out[i].right += umugu__scene.out[in[j]][i].right;
				}
			}

			// NOTE: Check assembly and profile two separate bucles vs one.
			float inv_count = 1.0f / count;
			for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
			{
				out[i].left *= inv_count;
				out[i].right *= inv_count;
			}

			break;
		}

		default:;
	}

	umugu__scene.out[unit] = out;
	return out;
}

UMUGU_DEF umugu_wave *umugu_output()
{
	umugu_wave *out = NULL;
	for (int i = 0; i <= umugu__last; ++i)
	{
		out = umugu__process(umugu__scene.link[i].in);
	}
	umugu__alloc_free();
	return out;
}

UMUGU_DEF umugu_unit umugu_newunit(umugu_type type, void *data, umugu_unit parent)
{
	umugu__scene.type[++umugu__last] = type;
	umugu__scene.data[umugu__last] = data;
	umugu__scene.out[umugu__last] = NULL;
	umugu__scene.link[umugu__link_count].unit = parent;
	umugu__scene.link[umugu__link_count++].in = umugu__last;
	return umugu__last;
}

UMUGU_DEF float *umugu_wave_table(umugu_shape shape)
{
	return umugu__wave_table[shape];
}

UMUGU_DEF int32_t umugu_scene_count()
{
	return umugu__last;
}

UMUGU_DEF const umugu_scene *umugu_scene_data()
{
	return &umugu__scene;
}

UMUGU_DEF void umugu_init()
{
	const float LINEAR_STEP = (1.0f / (UMUGU_SAMPLE_RATE / 4.0f));

	// TRIANGLE WAVE
	float lerp = 0.0f;
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		float cos_value = cosf((i/(float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
		umugu__wave_table[UMUGU_WS_TRIANGLE][i] = lerp;
		lerp += (cos_value > 0.0f ? LINEAR_STEP : -LINEAR_STEP);
	}
	
	// SINE AND SQUARE WAVES
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		float sine_value = sinf((i/(float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
		umugu__wave_table[UMUGU_WS_SINE][i] = sine_value;
		umugu__wave_table[UMUGU_WS_SQUARE][i] = sine_value > 0.0f ? 1.0f : -1.0f;
	}

	// SAW WAVE
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		umugu__wave_table[UMUGU_WS_SAW][i] = (float)i / (UMUGU_SAMPLE_RATE / 2.0f);
		if (i > (UMUGU_SAMPLE_RATE / 2.0f))
		{
			umugu__wave_table[UMUGU_WS_SAW][i] -= 2.0f;
		}
	}

	// WHITE NOISE
	int g_x1 = 0x67452301;
	int g_x2 = 0xefcdab89;
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		g_x1 ^= g_x2;
		umugu__wave_table[UMUGU_WS_WHITE_NOISE][i] = g_x2 * (2.0f / 0xffffffff);
		g_x2 += g_x1;
	}

	umugu__last = -1;
	umugu__alloc_free();
	umugu__init_backend();
}

UMUGU_DEF void umugu_close()
{
	umugu__close_backend();
}

// INPUT / OUTPUT BACKENDS IMPLEMENTATION

#ifdef UMUGU_IO_PORTAUDIO // Portaudio backend.

#include <portaudio.h>
#include <stdio.h>
#include <string.h>

#define umugu__pa_check_err() if (umugu__pa_err != paNoError) umugu__pa_terminate()

static PaStreamParameters umugu__pa_output_params;
static PaStream *umugu__pa_stream;
static PaError umugu__pa_err;

static void umugu__pa_terminate()
{
	Pa_Terminate();
	printf("An error occurred while using the portaudio stream.\n");
	printf("Error number: %d.\n", umugu__pa_err);
	printf("Error message: %s.\n", Pa_GetErrorText(umugu__pa_err));
	exit(umugu__pa_err);
}

static int umugu__pa_callback(const void *in, void *out,
		unsigned long fpb, const PaStreamCallbackTimeInfo *timeinfo,
		PaStreamCallbackFlags flags, void *data)
{
	void *wave = umugu_output();
	memcpy(out, wave, sizeof(umugu_wave) * UMUGU_FRAMES_PER_BUFFER);
	return 0;
}

UMUGU_DEF void umugu_start_stream()
{
	umugu__pa_err = Pa_StartStream(umugu__pa_stream);
	umugu__pa_check_err();
}

UMUGU_DEF void umugu_stop_stream()
{
	umugu__pa_err = Pa_StopStream(umugu__pa_stream);
	umugu__pa_check_err();
}

static void umugu__init_backend()
{
	umugu__pa_err = Pa_Initialize();
	umugu__pa_check_err();

	umugu__pa_output_params.device = Pa_GetDefaultOutputDevice();
	if (umugu__pa_output_params.device == paNoDevice)
	{
		fprintf(stderr, "Error: No default output device.\n");
		umugu__pa_terminate();
	}

	umugu__pa_output_params.channelCount = 2;
	umugu__pa_output_params.sampleFormat = paFloat32;
	umugu__pa_output_params.suggestedLatency =
		Pa_GetDeviceInfo(umugu__pa_output_params.device)->defaultLowOutputLatency;
	umugu__pa_output_params.hostApiSpecificStreamInfo = NULL;

	umugu__pa_err = Pa_OpenStream(&umugu__pa_stream, NULL, &umugu__pa_output_params,
		UMUGU_SAMPLE_RATE, UMUGU_FRAMES_PER_BUFFER, paClipOff, umugu__pa_callback, NULL);

	umugu__pa_check_err();
}

static void umugu__close_backend()
{
	umugu__pa_err = Pa_CloseStream(umugu__pa_stream);
	umugu__pa_check_err();
	Pa_Terminate();
}

#else // Dummy IO backend.

UMUGU_DEF void umugu_start_stream() {}
UMUGU_DEF void umugu_stop_stream() {}
static void umugu__init_backend() {}
static void umugu__close_backend() {}

#endif // IO backends.
#endif // UMUGU_IMPLEMENTATION
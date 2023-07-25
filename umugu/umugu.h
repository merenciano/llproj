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

#ifndef UMUGU_FRAMES_PER_BUFFER
#define UMUGU_FRAMES_PER_BUFFER 60
#endif

#ifndef UMUGU_SAMPLE_RATE
#define UMUGU_SAMPLE_RATE 44100
#endif

typedef void* umugu_unit;

typedef enum {
	UMUGU_NT_OSCILOSCOPE = 0,
	UMUGU_NT_MIX,
	UMUGU_NT_COUNT
} umugu_node_type;

typedef enum {
	UMUGU_WS_SINE = 0,
	UMUGU_WS_SAW,
	UMUGU_WS_SQUARE,
	UMUGU_WS_TRIANGLE,
	UMUGU_WS_WHITE_NOISE,
	UMUGU_WS_COUNT
} umugu_wave_shape;

typedef struct umugu_unit_list {
	umugu_unit unit;
	umugu_unit_list *next;
} umugu_unit_list;

typedef struct {
	float left, right;
} umugu_wave;

typedef struct {
	umugu_wave *out;
	umugu_unit_list *in;
	umugu_node_type type;	
	void *data;
} umugu_node;

typedef struct {
	umugu_wave_shape shape;
	int freq;
	int _phase;
} umugu_osciloscope_data;

UMUGU_DEF void umugu_init();
UMUGU_DEF void umugu_close();
UMUGU_DEF void umugu_start_stream(umugu_unit output_unit);
UMUGU_DEF void umugu_stop_stream();
UMUGU_DEF umugu_unit umugu_newunit(umugu_node_type type, void *data, umugu_unit parent);
UMUGU_DEF void umugu_setparent(umugu_unit unit, umugu_unit parent);
UMUGU_DEF umugu_wave *umugu_getwave(umugu_unit unit);
UMUGU_DEF void umugu_iterate(umugu_unit unit, void(*callback)(umugu_unit));
UMUGU_DEF float *umugu_wave_table(umugu_wave_shape shape);

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

static void umugu__process(umugu_node *node)
{
	for (umugu_unit_list *in = node->in; in; in = in->next)
	{
		umugu__process((umugu_node*)in->unit);
	}

	switch(node->type)
	{
	case UMUGU_NT_OSCILOSCOPE:
	{
		umugu_osciloscope_data *data = (umugu_osciloscope_data*)node->data;
		node->out = umugu__alloc_wave_buffer();
		for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
		{
			node->out[i].left = umugu__wave_table[data->shape][data->_phase];
			node->out[i].right = umugu__wave_table[data->shape][data->_phase];
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
		int count = 0;
		for (umugu_unit_list *in = node->in; in; in = in->next)
		{
			count++;
		}

		node->out = ((umugu_node*)node->in->unit)->out;
		for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
		{
			for (umugu_unit_list *in = node->in->next; in; in = in->next)
			{
				node->out[i].left += ((umugu_node*)in->unit)->out[i].left;
				node->out[i].right += ((umugu_node*)in->unit)->out[i].right;
			}
			node->out[i].left /= count;
			node->out[i].right /= count;
		}
		break;
	}

	default:
	{
		break;
	}
	}
}

UMUGU_DEF umugu_wave *umugu_getwave(umugu_unit unit)
{
	umugu__process((umugu_node*)unit);
	umugu__alloc_free();
	return ((umugu_node*)unit)->out;
}

UMUGU_DEF void umugu_setparent(umugu_unit unit, umugu_unit parent)
{
	umugu_node *p = (umugu_node*)parent;
	umugu_unit_list *node = (umugu_unit_list*)malloc(sizeof(umugu_unit_list));
	node->unit = unit;
	node->next = p->in;
	p->in = node;
}

UMUGU_DEF umugu_unit umugu_newunit(umugu_node_type type, void *data, umugu_unit parent)
{
	umugu_node *unit = (umugu_node*)malloc(sizeof(umugu_node));
	unit->type = type;
	unit->data = data;
	unit->in = NULL;
	if (parent)
	{
		umugu_setparent(unit, parent);
	}
	return (umugu_unit)unit;
}

UMUGU_DEF void umugu_iterate(umugu_unit unit, void(*callback)(umugu_unit))
{
	umugu_node *node = (umugu_node*)unit;
	for (umugu_unit_list *in = node->in; in; in = in->next)
	{
		umugu_iterate((umugu_unit)in->unit, callback);
	}
	callback(unit);
}

UMUGU_DEF float *umugu_wave_table(umugu_wave_shape shape)
{
	return umugu__wave_table[shape];
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
static umugu_unit umugu__pa_output;

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
	void *wave = umugu_getwave(umugu__pa_output);
	memcpy(out, wave, sizeof(umugu_wave) * UMUGU_FRAMES_PER_BUFFER);
	return 0;
}

UMUGU_DEF void umugu_start_stream(umugu_unit output_unit)
{
	umugu__pa_output = output_unit;
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

UMUGU_DEF void umugu_start_stream(umugu_unit output_unit) {}
UMUGU_DEF void umugu_stop_stream() {}
static void umugu__init_backend() {}
static void umugu__close_backend() {}

#endif // IO backends.
#endif // UMUGU_IMPLEMENTATION
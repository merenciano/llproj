/*
	TODO:
	- Data serialization.
*/

#ifndef UMUGU_H
#define UMUGU_H

#ifndef UMUGU_FRAMES_PER_BUFFER
#define UMUGU_FRAMES_PER_BUFFER 60
#endif

#ifndef UMUGU_SAMPLE_RATE
#define UMUGU_SAMPLE_RATE 44100
#endif

//#define UMUGU_NO_OSCILOSCOPE_LUT
//#define UMUGU_NO_STDIO

#ifdef __cplusplus
extern "C" {
#endif

typedef int umugu_unit;

typedef enum
{
	UMUGU_T_OSCILOSCOPE = 0,
	UMUGU_T_MIX,
	UMUGU_T_INSPECTOR,
	UMUGU_T_VOLUME,
	UMUGU_T_CLAMP,
	UMUGU_T_COUNT
} umugu_type;

typedef enum
{
	UMUGU_WS_SINE = 0,
	UMUGU_WS_SAW,
	UMUGU_WS_SQUARE,
	UMUGU_WS_TRIANGLE,
	UMUGU_WS_WHITE_NOISE,
	UMUGU_WS_COUNT
} umugu_shape;

typedef struct
{
	float left;
	float right;
} umugu_wave;

typedef struct
{
	umugu_type *type;
	void **data;
	int *fx_rig;
} umugu_scene;

typedef struct
{
	umugu_wave out[UMUGU_FRAMES_PER_BUFFER];
	int _phase;
	int freq;
	umugu_shape shape;
} umugu_osciloscope_data;

typedef struct
{
	float *values;
	int size;
	int it;
	int stride;
	int offset;
	int pause;
} umugu_inspector_data;

typedef struct
{
	float multiplier;
} umugu_volume_data;

typedef struct
{
	float min;
	float max;
} umugu_clamp_data;

const float *umugu_osciloscope_lut(umugu_shape shape);

void umugu_init(umugu_scene *scene);
void umugu_close();

void umugu_start_stream();
void umugu_stop_stream();

void umugu_serialize_scene(umugu_scene *scene, void *serialized, int count);
umugu_scene *umugu_deserialize_scene(void *serialized_scene, int *count);

void umugu_serialize_scene(umugu_scene *scene, void *serialized, int count)
{
	char *buffer = (char*)serialized;
	*(int*)buffer = count;
	buffer += sizeof(int);
	umugu_type *type = (umugu_type*)buffer;
	buffer += count * sizeof(umugu_type);
	void **data = (void**)buffer;
	buffer += count * sizeof(void**);

	for (int i = 0; i < count; ++i)
	{
		type[i] = scene->type[i];
		data[i] = scene->data[i];
		memcpy(buffer, scene->data[i], sizeof()); // pillar size del data para cada type.
	}

#ifdef __cplusplus
}
#endif
#endif // UMUGU_H

//#define UMUGU_IMPLEMENTATION
#ifdef UMUGU_IMPLEMENTATION

#include <math.h>

#ifdef __cplusplus
	#define UMUGU_RESTRICT __restrict__
#else
	#define UMUGU_RESTRICT restrict
#endif

static void umugu__init_backend(umugu_scene *scene);
static void umugu__close_backend();

static int umugu__fx_it;

#ifndef UMUGU_NO_OSCILOSCOPE_LUT
static float umugu__osciloscope_lut[UMUGU_WS_COUNT][UMUGU_SAMPLE_RATE];

static void umugu__gen_osciloscope_lut()
{
	const float LINEAR_STEP = (1.0f / (UMUGU_SAMPLE_RATE / 4.0f));

	// TRIANGLE WAVE
	float lerp = 0.0f;
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		float cos_value = cosf((i/(float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
		umugu__osciloscope_lut[UMUGU_WS_TRIANGLE][i] = lerp;
		lerp += (cos_value > 0.0f ? LINEAR_STEP : -LINEAR_STEP);
	}
	
	// SINE AND SQUARE WAVES
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		float sine_value = sinf((i/(float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
		umugu__osciloscope_lut[UMUGU_WS_SINE][i] = sine_value;
		umugu__osciloscope_lut[UMUGU_WS_SQUARE][i] = sine_value > 0.0f ? 1.0f : -1.0f;
	}

	// SAW WAVE
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		umugu__osciloscope_lut[UMUGU_WS_SAW][i] = (float)i / (UMUGU_SAMPLE_RATE / 2.0f);
		if (i > (UMUGU_SAMPLE_RATE / 2.0f))
		{
			umugu__osciloscope_lut[UMUGU_WS_SAW][i] -= 2.0f;
		}
	}

	// WHITE NOISE
	int g_x1 = 0x67452301;
	int g_x2 = 0xefcdab89;
	for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i)
	{
		g_x1 ^= g_x2;
		umugu__osciloscope_lut[UMUGU_WS_WHITE_NOISE][i] = g_x2 * (2.0f / 0xffffffff);
		g_x2 += g_x1;
	}
}
#endif

static umugu_wave *umugu__process_unit(const umugu_scene *scene)
{
	umugu_unit unit = scene->fx_rig[umugu__fx_it++];
	umugu_wave *out = NULL;
	switch (scene->type[unit])
	{
		case UMUGU_T_OSCILOSCOPE:
		{
			umugu_osciloscope_data *data = (umugu_osciloscope_data*)scene->data[unit];
			for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
			{
				float sample = umugu__osciloscope_lut[data->shape][data->_phase];
				data->out[i].left = sample;
				data->out[i].right = sample;
				data->_phase += data->freq;
				if (data->_phase >= UMUGU_SAMPLE_RATE)
				{
					data->_phase -= UMUGU_SAMPLE_RATE;
				}
			}
			out = data->out;
			break;
		}

		case UMUGU_T_MIX:
		{
			int count = scene->fx_rig[umugu__fx_it++];
			out = umugu__process_unit(scene);
			for (int i = 1; i < count; ++i)
			{
				umugu_wave * UMUGU_RESTRICT tmp = umugu__process_unit(scene);
				for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; ++i)
				{
					out[i].left += tmp[i].left;
					out[i].right += tmp[i].right;
				}
			}

			float inv_count = 1.0f / count;
			for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
			{
				out[i].left *= inv_count;
				out[i].right *= inv_count;
			}
			break;
		}

		case UMUGU_T_INSPECTOR:
		{
			umugu_inspector_data *data = (umugu_inspector_data*)scene->data[unit];
			out = umugu__process_unit(scene);
			if (data->pause)
			{
				break;
			}
			float *fout = (float*)out;
			for (int i = data->offset; i < UMUGU_FRAMES_PER_BUFFER * 2; i += data->stride)
			{
				data->values[data->it] = fout[i];
				data->values[data->it + data->size] = fout[i];
				data->it++;
				if (data->it >= data->size)
				{
					data->it -= data->size;
				}
			}
			break;
		}

		case UMUGU_T_VOLUME:
		{
			umugu_volume_data *data = (umugu_volume_data*)scene->data[unit];
			out = umugu__process_unit(scene);
			for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
			{
				out[i].left *= data->multiplier;
				out[i].right *= data->multiplier;
			}
			break;
		}

		case UMUGU_T_CLAMP:
		{
			umugu_clamp_data *data = (umugu_clamp_data*)scene->data[unit];
			out = umugu__process_unit(scene);
			for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
			{
				out[i].left = fmin(out[i].left, data->max);
				out[i].left = fmax(out[i].left, data->min);
				out[i].right = fmin(out[i].right, data->max);
				out[i].right = fmax(out[i].right, data->min);
			}
			break;
		}

		default:;
	}
	return out;
}

const float *umugu_osciloscope_lut(umugu_shape shape)
{
#ifndef UMUGU_NO_OSCILOSCOPE_LUT
	return umugu__osciloscope_lut[shape];
#else
	return NULL;
#endif
}


void umugu_init(umugu_scene *scene)
{
#ifndef UMUGU_NO_OSCILOSCOPE_LUT
	umugu__gen_osciloscope_lut();
#endif
	umugu__init_backend(scene);
}

void umugu_close()
{
	umugu__close_backend();
}

// INPUT / OUTPUT BACKENDS IMPLEMENTATION

#ifdef UMUGU_IO_PORTAUDIO // Portaudio backend.
#include <portaudio.h>

#define umugu__pa_check_err() if (umugu__pa_err != paNoError) umugu__pa_terminate()

static PaStreamParameters umugu__pa_output_params;
static PaStream *umugu__pa_stream;
static PaError umugu__pa_err;

#ifdef UMUGU_NO_STDIO
static void umugu__pa_terminate()
{
	Pa_Terminate();
	printf("An error occurred while using the portaudio stream.\n");
	printf("Error number: %d.\n", umugu__pa_err);
	printf("Error message: %s.\n", Pa_GetErrorText(umugu__pa_err));
	exit(umugu__pa_err);
}
#else
#include <stdio.h>
static void umugu__pa_terminate()
{
	Pa_Terminate();
	printf("An error occurred while using the portaudio stream.\n");
	printf("Error number: %d.\n", umugu__pa_err);
	printf("Error message: %s.\n", Pa_GetErrorText(umugu__pa_err));
	exit(umugu__pa_err);
}
#endif

static int umugu__pa_callback(const void *in, void *out,
		unsigned long fpb, const PaStreamCallbackTimeInfo *timeinfo,
		PaStreamCallbackFlags flags, void *data)
{
	umugu__fx_it = 0;
	umugu_wave * UMUGU_RESTRICT w = umugu__process_unit((umugu_scene*)data);
	umugu_wave * UMUGU_RESTRICT o = (umugu_wave*)out;
	unsigned long i = UMUGU_FRAMES_PER_BUFFER;
	while (i--)
	{
		*o++ = *w++;
	}
	return paContinue;
}

void umugu_start_stream()
{
	umugu__pa_err = Pa_StartStream(umugu__pa_stream);
	umugu__pa_check_err();
}

void umugu_stop_stream()
{
	umugu__pa_err = Pa_StopStream(umugu__pa_stream);
	umugu__pa_check_err();
}

static void umugu__init_backend(umugu_scene *scene)
{
	umugu__pa_err = Pa_Initialize();
	umugu__pa_check_err();

	umugu__pa_output_params.device = Pa_GetDefaultOutputDevice();
	if (umugu__pa_output_params.device == paNoDevice)
	{
		umugu__pa_terminate();
	}

	umugu__pa_output_params.channelCount = 2;
	umugu__pa_output_params.sampleFormat = paFloat32;
	umugu__pa_output_params.suggestedLatency =
		Pa_GetDeviceInfo(umugu__pa_output_params.device)->defaultLowOutputLatency;
	umugu__pa_output_params.hostApiSpecificStreamInfo = NULL;

	umugu__pa_err = Pa_OpenStream(&umugu__pa_stream, NULL, &umugu__pa_output_params,
		UMUGU_SAMPLE_RATE, UMUGU_FRAMES_PER_BUFFER, paClipOff, umugu__pa_callback, scene);

	umugu__pa_check_err();
}

static void umugu__close_backend()
{
	umugu__pa_err = Pa_CloseStream(umugu__pa_stream);
	umugu__pa_check_err();
	Pa_Terminate();
}

#else // Dummy IO backend.

void umugu_start_stream() {}
void umugu_stop_stream() {}
static void umugu__init_backend() {}
static void umugu__close_backend() {}

#endif // IO backends.
#endif // UMUGU_IMPLEMENTATION
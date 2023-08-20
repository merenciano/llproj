#ifndef UMUGU_H
#define UMUGU_H

#include <string.h>

#ifndef UMUGU_FRAMES_PER_BUFFER
#define UMUGU_FRAMES_PER_BUFFER 60
#endif

#ifndef UMUGU_SAMPLE_RATE
#define UMUGU_SAMPLE_RATE 48000
#endif

//#define UMUGU_NO_OSCILOSCOPE_LUT

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
	UMUGU_T_FILE,
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
	int *fx_rig;
	umugu_type *type;
	void **data;
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

typedef struct
{
	umugu_wave *out;
	char filename[256];
	void *impl_data;
} umugu_file_data;

const float *umugu_osciloscope_lut(umugu_shape shape);

void umugu_init(umugu_scene *scene, void *alloc_arena);
void umugu_close();

void umugu_start_stream();
void umugu_stop_stream();

size_t umugu_get_serialized_size(umugu_scene *scene);
void umugu_serialize_scene(umugu_scene *in_scene, void *out_serialized);
void umugu_deserialize_scene(umugu_scene *out_scene, void *in_serialized);

#ifdef __cplusplus
}
#endif
#endif // UMUGU_H

//#define UMUGU_IMPLEMENTATION
#ifdef UMUGU_IMPLEMENTATION

#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
	#define UMUGU_RESTRICT __restrict__
#else
	#define UMUGU_RESTRICT restrict
#endif

static void umugu__init_backend(umugu_scene *scene);
static void umugu__close_backend();

static int umugu__fx_it;

static const size_t UMUGU_DATA_SIZE[] = {
	sizeof(umugu_osciloscope_data),
	0,
	sizeof(umugu_inspector_data),
	sizeof(umugu_volume_data),
	sizeof(umugu_clamp_data)
};

size_t umugu_get_serialized_size(umugu_scene *scene)
{
	int count = *scene->fx_rig + 1;
	size_t size = sizeof(int);
	size += count * sizeof(umugu_type);
	size += count * sizeof(void**);
	
	for (int i = 0; i < count; ++i)
	{
		size += UMUGU_DATA_SIZE[scene->type[i]];
	}

	int *pipeline = scene->fx_rig;
	while (*pipeline != 0)
	{
		size += sizeof(int);
		++pipeline;
	}

	return size + sizeof(int);
}

void umugu_serialize_scene(umugu_scene *scene, void *serialized)
{
	int count = *scene->fx_rig + 1;
	char *buffer = (char*)serialized;

	int *pipeline = scene->fx_rig;
	while(*pipeline != 0)
	{
		*(int*)buffer = *pipeline;
		buffer += sizeof(int);
		++pipeline;
	}
	*(int*)buffer = 0;
	buffer += sizeof(int);

	memcpy(buffer, scene->type, count * sizeof(umugu_type));
	buffer += count * sizeof(umugu_type);

	size_t *data_offsets = (size_t*)buffer;
	buffer += count * sizeof(size_t*);

	size_t offset = 0;
	for (int i = 0; i < count; ++i)
	{
		data_offsets[i] = offset;
		memcpy(buffer + offset, scene->data[i], UMUGU_DATA_SIZE[scene->type[i]]);
		offset += UMUGU_DATA_SIZE[scene->type[i]];
	}
}

void umugu_deserialize_scene(umugu_scene *scene, void *serialized)
{
	char *buffer = (char*)serialized;
	int count = *(int*)buffer + 1;	

	scene->fx_rig = (int*)buffer;
	while (*(int*)buffer != 0)
	{
		buffer += sizeof(int);
	}
	buffer += sizeof(int);

	scene->type = (umugu_type*)buffer;
	buffer += count * sizeof(umugu_type);
	scene->data = (void**)buffer;
	buffer += count * sizeof(void**);

	for (int i = 0; i < count; ++i)
	{
        scene->data[i] += (size_t)buffer;
	}
}

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

		case UMUGU_T_FILE:
		{
			static const float INV_SIG_RANGE = 1.0f / 32768.0f;
			umugu_file_data *data = (umugu_file_data*)scene->data[unit];
			fread(data->out, UMUGU_FRAMES_PER_BUFFER * 4, 1, (FILE*)data->impl_data); // size 4 -> 16 bytes stereo

			short *it = (short*)data->out + UMUGU_FRAMES_PER_BUFFER * 2 - 1;
			for (int i = UMUGU_FRAMES_PER_BUFFER - 1; i >= 0; --i)
			{
				data->out[i].right = *it-- * INV_SIG_RANGE;
				data->out[i].left = *it-- * INV_SIG_RANGE;
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

static void umugu__init_units(const umugu_scene *scene, void *alloc_arena)
{
	int count = *scene->fx_rig + 1;
	for (int i = 0; i < count; ++i)
	{
		switch (scene->type[i])
		{
			case UMUGU_T_FILE:
			{
				umugu_file_data *d = scene->data[i];
				d->out = alloc_arena;
				alloc_arena += UMUGU_FRAMES_PER_BUFFER * sizeof(umugu_wave);
				d->impl_data = fopen(d->filename, "rb");
				if (d->impl_data == NULL)
				{
					printf("%s\n", d->filename);
				}
				printf("init file\n");
				break;
			}

			default: break;
		};
	}
}

const float *umugu_osciloscope_lut(umugu_shape shape)
{
#ifndef UMUGU_NO_OSCILOSCOPE_LUT
	return umugu__osciloscope_lut[shape];
#else
	return NULL;
#endif
}


void umugu_init(umugu_scene *scene, void *alloc_arena)
{
#ifndef UMUGU_NO_OSCILOSCOPE_LUT
	umugu__gen_osciloscope_lut();
#endif
	umugu__init_units(scene, alloc_arena);
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

//static PaStreamParameters umugu__pa_input_params;
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
static void umugu__init_backend(umugu_scene *scene) {}
static void umugu__close_backend() {}

#endif // IO backends.
#endif // UMUGU_IMPLEMENTATION

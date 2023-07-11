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
	umugu_wave wave[UMUGU_FRAMES_PER_BUFFER];
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
UMUGU_DEF umugu_unit umugu_newunit(umugu_node_type type, void *data, umugu_unit parent);
UMUGU_DEF void umugu_setparent(umugu_unit unit, umugu_unit parent);
UMUGU_DEF umugu_wave *umugu_getwave(umugu_unit unit);
UMUGU_DEF void umugu_iterate(umugu_unit unit, void(*callback)(umugu_unit));
UMUGU_DEF float *umugu_wave_table(umugu_wave_shape shape);

#endif

#ifdef UMUGU_IMPLEMENTATION

#include <stdlib.h>
#include <math.h>

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
		for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
		{
			node->wave[i].left = umugu__wave_table[data->shape][data->_phase];
			node->wave[i].right = umugu__wave_table[data->shape][data->_phase];
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

		for (int i = 0; i < UMUGU_FRAMES_PER_BUFFER; i++)
		{
			node->wave[i].left = 0.0f;
			node->wave[i].right = 0.0f;
			for (umugu_unit_list *in = node->in; in; in = in->next)
			{
				node->wave[i].left += ((umugu_node*)in->unit)->wave[i].left;
				node->wave[i].right += ((umugu_node*)in->unit)->wave[i].right;
			}
			node->wave[i].left /= count;
			node->wave[i].right /= count;
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
	return ((umugu_node*)unit)->wave;
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
}

#endif // UMUGU_IMPLEMENTATION

#if 0
#include "node.h"
#include "config.h"

#include <math.h>

namespace umugu
{
Node *last;

void *Process(Node *n)
{
	for (auto child : n->in)
	{
		Process(child);
	}
	(*n)();
	return n->wave;
}

float wave_table[WS_COUNT][SAMPLE_RATE];
void GenWaveTable()
{
	const float LINEAR_STEP = (1.0f / (SAMPLE_RATE / 4.0f));

	// TRIANGLE WAVE
	float lerp = 0.0f;
	for (int i = 0; i < SAMPLE_RATE; ++i)
	{
		float cos_value = cosf((i/(float)SAMPLE_RATE) * M_PI * 2.0f);
		wave_table[WS_TRIANGLE][i] = lerp;
		lerp += (cos_value > 0.0f ? LINEAR_STEP : -LINEAR_STEP);
	}
	
	// SINE AND SQUARE WAVES
	for (int i = 0; i < SAMPLE_RATE; ++i)
	{
		float sine_value = sinf((i/(float)SAMPLE_RATE) * M_PI * 2.0f);
		wave_table[WS_SINE][i] = sine_value;
		wave_table[WS_SQUARE][i] = sine_value > 0.0f ? 1.0f : -1.0f;
	}

	// SAW WAVE
	for (int i = 0; i < SAMPLE_RATE; ++i)
	{
		wave_table[WS_SAW][i] = (float)i / (SAMPLE_RATE / 2.0f);
		if (i > (SAMPLE_RATE / 2.0f))
		{
			wave_table[WS_SAW][i] -= 2.0f;
		}
	}

	// WHITE NOISE
	int g_x1 = 0x67452301;
	int g_x2 = 0xefcdab89;
	for (int i = 0; i < SAMPLE_RATE; ++i)
	{
		g_x1 ^= g_x2;
		wave_table[WS_WHITE_NOISE][i] = g_x2 * (2.0f / 0xffffffff);
		g_x2 += g_x1;
	}
}

Node::Node() : in()
{
#ifdef UMUGU_DEBUG
	for (int i = 0; i < FRAMES_PER_BUFFER; ++i)
	{
		wave[i].left = 0.0f;
		wave[i].right = 0.0f;
	}
#endif
}

Osciloscope::Osciloscope() : Node(), shape(WS_SINE), freq(0), phase(0)
{
}

void Osciloscope::operator()()
{
	for (int i = 0; i < FRAMES_PER_BUFFER; ++i)
	{
		wave[i].left = wave_table[shape][phase];			
		wave[i].right = wave_table[shape][phase];			
		phase += freq;
		if (phase >= SAMPLE_RATE)
		{
			phase -= SAMPLE_RATE;
		}
	}
}

Mixer::Mixer() : Node() {}

void Mixer::operator()()
{
	for (int i = 0; i < FRAMES_PER_BUFFER; ++i)
	{
		for (auto *n : in)
		{
			wave[i].left += n->wave[i].left;
			wave[i].right += n->wave[i].right;
		}
		wave[i].left /= in.size();
		wave[i].right /= in.size();
	}
}
}
#endif
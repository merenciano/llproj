#ifndef __UMUGU_CONFIG_H__
#define __UMUGU_CONFIG_H__

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 60
#define MAX_WAVES 32

enum WaveShape : int
{
	WS_SINE,
	WS_SAW,
	WS_SQUARE,
	WS_TRIANGLE,
	WS_WHITE_NOISE,
	WS_COUNT
};

#endif // __UMUGU_CONFIG_H__
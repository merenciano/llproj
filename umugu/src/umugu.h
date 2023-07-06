#include "config.h"

namespace umugu 
{

extern const char *const SHAPE_NAMES[];
extern float *WaveTable(WaveShape ws);

struct AudioCallbackData
{
	int freq[MAX_WAVES];
	int left_phase[MAX_WAVES];
	int right_phase[MAX_WAVES];
	int wave_shape[MAX_WAVES];
	int wave_count;
};

struct Window
{
	void *native_win;
	const char *title;
	int width, height;
	const char *glsl_ver;
	void *gl_context;
};

extern AudioCallbackData data;
extern Window window;
extern float plot_x[SAMPLE_RATE];
void Init();
bool PollEvents();
void Render();
void StartAudioStream();
void Close();
}

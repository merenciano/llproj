
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 60
#define MAX_WAVES 32

namespace umugu 
{
enum WaveShape
{
	WS_SINE,
	WS_SAW,
	WS_SQUARE,
	WS_TRIANGLE,
	WS_WHITE_NOISE,
	WS_COUNT
};

extern const char *const SHAPE_NAMES[];

struct AudioCallbackData
{
	int freq[MAX_WAVES];
	int left_phase[MAX_WAVES];
	int right_phase[MAX_WAVES];
	int wave_shape[MAX_WAVES];
	float wave_table[WS_COUNT][SAMPLE_RATE];
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

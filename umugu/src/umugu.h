
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 60
#define MAX_WAVES 32

namespace umugu 
{
struct AudioCallbackData
{
	int freq[MAX_WAVES];
	int left_phase[MAX_WAVES];
	int right_phase[MAX_WAVES];
	float sine[SAMPLE_RATE];
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
void Init();
bool PollEvents();
void Render();
void StartAudioStream();
void Close();
}
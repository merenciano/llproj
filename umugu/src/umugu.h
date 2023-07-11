#define UMUGU_SAMPLE_RATE 44100
#define UMUGU_FRAMES_PER_BUFFER 60

namespace umugu 
{

extern const char *const SHAPE_NAMES[];

struct Window
{
	void *native_win;
	const char *title;
	int width, height;
	const char *glsl_ver;
	void *gl_context;
};

extern Window window;
extern float plot_x[UMUGU_SAMPLE_RATE];
void Init();
bool PollEvents();
void Render();
void StartAudioStream();
void Close();
}

extern void *graph_fx;

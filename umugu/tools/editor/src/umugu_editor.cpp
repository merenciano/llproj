#include "umugu_editor.h"

#define UMUGU_IO_PORTAUDIO
#define UMUGU_IMPLEMENTATION
#include "umugu.h"

#include "imgui.h"
#include "implot.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#define PLOT_WAVE_SHAPE(WS, R, G, B) \
	ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(R, G, B, 1.0f)); \
	ImPlot::PlotLine(SHAPE_NAMES[WS], plot_x, umugu_wave_table(WS), UMUGU_SAMPLE_RATE); \
	ImPlot::PopStyleColor()

static ImGuiIO *io;
static bool show_demo_window = true;

namespace umugu_editor
{
struct Window
{
	void *native_win;
	const char *title;
	int width, height;
	const char *glsl_ver;
	void *gl_context;
};

umugu_unit graph_fx;
Window window;
float plot_x[UMUGU_SAMPLE_RATE];

const char *const SHAPE_NAMES[] = {
	"SINE",
	"SAW",
	"SQUARE",
	"TRIANGLE",
	"WHITE_NOISE"
};

static void DrawUnitUI(umugu_unit unit)
{
	const umugu_scene *scene = umugu_scene_data();
	switch(scene->type[unit])
	{
		case UMUGU_NT_OSCILOSCOPE:
		{
			umugu_osciloscope_data *d = (umugu_osciloscope_data*)scene->data[unit];
			ImGui::Text("Osciloscope");
			if (ImGui::BeginCombo("Wave Shape", SHAPE_NAMES[d->shape], 0))
			{
				for (int j = 0; j < UMUGU_WS_COUNT; ++j)
				{
					const bool selected = (d->shape == (umugu_shape)j);
					if (ImGui::Selectable(SHAPE_NAMES[j], selected))
					{
						d->shape = (umugu_shape)j;
					}

					if (selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::InputInt("Frequency", &d->freq, 1, 10, 0);
			break;
		}

		case UMUGU_NT_MIX:
		{
			ImGui::Text("Mix");
			break;
		}

		default: break;
	}
}

/*static void IterateGraphAndShowNodes(umugu_unit unit)
{
	ImGui::PushID(unit);
	DrawUnitUI(unit);
	if (ImGui::TreeNode("Unit"))
	{
		umugu_node *n = (umugu_node*)unit;
		for (umugu_unit_list *in = n->in; in; in = in->next)
		{
			IterateGraphAndShowNodes(in->unit);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}*/

static void GraphWindow()
{
	ImGui::Begin("Graph");
	for (int i = 0; i < umugu_scene_count(); ++i)
	{
		ImGui::PushID(i);
		DrawUnitUI(i);
		ImGui::PopID();
	}
	ImGui::End();
}

static void InitUI()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return;
	}

#if defined(IMGUI_IMPL_OPENGL_ES2)
	window.glsl_ver = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	window.glsl_ver = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	window.native_win = SDL_CreateWindow("Venga, a ver si te buscas una musiquilla guapa, no colega?", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
	window.gl_context = SDL_GL_CreateContext((SDL_Window*)window.native_win);
	SDL_GL_MakeCurrent((SDL_Window*)window.native_win, window.gl_context);
	SDL_GL_SetSwapInterval(1);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	io = &ImGui::GetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(window.native_win, window.gl_context);
	ImGui_ImplOpenGL3_Init(window.glsl_ver);
}

bool PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID((SDL_Window*)window.native_win))) {
			return true;
		}
	}
	return false;
}

void Render()
{
	glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	if (show_demo_window) {
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	GraphWindow();

	ImGui::Begin("Hello, world!");
	ImGui::Checkbox("Demo Window", &show_demo_window);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
	if (ImPlot::BeginPlot("Wave form")) {
		PLOT_WAVE_SHAPE(UMUGU_WS_SINE, 1.0f, 0.0f, 0.0f);
		PLOT_WAVE_SHAPE(UMUGU_WS_SAW, 0.0f, 1.0f, 0.0f);
		PLOT_WAVE_SHAPE(UMUGU_WS_SQUARE, 0.0f, 0.0f, 1.0f);
		PLOT_WAVE_SHAPE(UMUGU_WS_TRIANGLE, 1.0f, 0.0f, 1.0f);
		PLOT_WAVE_SHAPE(UMUGU_WS_WHITE_NOISE, 1.0f, 1.0f, 0.0f);
		ImPlot::EndPlot();
	}

	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow((SDL_Window*)window.native_win);
}

void Close()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(window.gl_context);
	SDL_DestroyWindow((SDL_Window*)window.native_win);
	SDL_Quit();

	umugu_close();
}
 
void Init()
{
	float *x = plot_x;
	float value = 0.0f;
	float fsample_rate = (float)UMUGU_SAMPLE_RATE;

	do
	{
		*x = value++;
	}
	while (*x++ < fsample_rate);

	umugu_init();
	static umugu_osciloscope_data osc_data[2];
	osc_data[0]._phase = 0;
	osc_data[0].freq = 220;
	osc_data[0].shape = UMUGU_WS_SINE;

	osc_data[1]._phase = 0;
	osc_data[1].freq = 440;
	osc_data[1].shape = UMUGU_WS_SINE;


	graph_fx = umugu_newunit(UMUGU_NT_MIX, NULL, -1);
	umugu_newunit(UMUGU_NT_OSCILOSCOPE, &osc_data[0], graph_fx);
	umugu_newunit(UMUGU_NT_OSCILOSCOPE, &osc_data[1], graph_fx);

	umugu_start_stream();
	InitUI();
}
} // namespace umugu
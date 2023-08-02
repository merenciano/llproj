#include "umugu_editor.h"
#include <unordered_map>
#include <vector>
#include <list>
#include <mutex>

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
	ImPlot::PlotLine(SHAPE_NAMES[WS], umugu_osciloscope_lut(WS), UMUGU_SAMPLE_RATE); \
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

Window window;

const char *const SHAPE_NAMES[] = {
	"SINE",
	"SAW",
	"SQUARE",
	"TRIANGLE",
	"WHITE_NOISE"
};

static umugu_scene scene;
static umugu_osciloscope_data o1;
static umugu_volume_data vdat;
static umugu_inspector_data inspec_dta;
static umugu_clamp_data clampdat;
static std::vector<umugu_type> types = {UMUGU_T_OSCILOSCOPE, UMUGU_T_VOLUME, UMUGU_T_CLAMP, UMUGU_T_INSPECTOR};
static std::vector<void*> data = {&o1, &vdat, &clampdat, &inspec_dta};
static std::vector<int> fx_rig = {3, 2, 1, 0};
static float inspec_values[4096];

static void InitData()
{
	o1.freq = 140;
	o1.shape = UMUGU_WS_SINE;
	o1._phase = 0;

	vdat.multiplier = 1.0f;
	clampdat.min = -1.0f;
	clampdat.max = 1.0f;

	scene.data = data.data();
	scene.type = types.data();
	scene.fx_rig = fx_rig.data();

	inspec_dta.values = inspec_values;
	inspec_dta.it = 0;
	inspec_dta.offset = 0;
	inspec_dta.stride = 2;
	inspec_dta.size = 2048;
	inspec_dta.pause = false;
}

static void DrawUnitUI(umugu_unit unit)
{
	switch(scene.type[unit])
	{
		case UMUGU_T_OSCILOSCOPE:
		{
			umugu_osciloscope_data *d = (umugu_osciloscope_data*)scene.data[unit];
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

		case UMUGU_T_MIX:
		{
			ImGui::Text("Mix");
			break;
		}

		case UMUGU_T_INSPECTOR:
		{
			umugu_inspector_data *d = (umugu_inspector_data*)scene.data[unit];
			if (ImPlot::BeginPlot("Inspector"))
			{
				ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				ImPlot::PlotLine("Wave", d->values + d->it, d->size);
				ImPlot::PopStyleColor();

				ImPlot::EndPlot();
			}
			if (d->pause)
			{
				ImGui::InputInt("Stride", &d->stride, 0, 1024, 0);
				ImGui::InputInt("Offset", &d->offset, 0, 1024, 0);
			}
			ImGui::Checkbox("Paused", (bool*)&d->pause);
			break;
		}

		case UMUGU_T_VOLUME:
		{
			umugu_volume_data *d = (umugu_volume_data*)scene.data[unit];
			ImGui::Text("Volume");
			ImGui::InputFloat("Multiplier:", &d->multiplier, 0, 100, 0);
			break;
		}

		case UMUGU_T_CLAMP:
		{
			umugu_clamp_data *d = (umugu_clamp_data*)scene.data[unit];
			ImGui::Text("Clamp");
			ImGui::InputFloat("Lower Limit:", &d->min, -2.0f, 2.0f, 0);
			ImGui::InputFloat("Upper Limit:", &d->max, -2.0f, 2.0f, 0);
			break;
		}

		default: break;
	}
}

static void GraphWindow()
{
	ImGui::Begin("Graph");
	for (auto fx : fx_rig)
	{
		ImGui::PushID(fx);
		DrawUnitUI(fx);
		ImGui::PopID();
		ImGui::Separator();
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
	InitData();
	umugu_init(&scene);

	umugu_start_stream();
	InitUI();
}
} // namespace umugu
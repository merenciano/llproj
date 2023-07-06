#include "ui.h"
#include "umugu.h"
#include "imgui.h"
#include "node.h"
#include "implot.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include <cstdio>

#define PLOT_WAVE_SHAPE(WS, R, G, B) ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(R, G, B, 1.0f)); ImPlot::PlotLine(SHAPE_NAMES[WS], plot_x, WaveTable(WS), SAMPLE_RATE); ImPlot::PopStyleColor()

static ImGuiIO *io;
static bool show_demo_window = true;

namespace umugu
{
void InitUI()
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
#elif defined(__APPLE__)
	window.glsl_ver = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
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

bool PollWindowEvents()
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

void DrawUI()
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

	ImGui::Begin("Hello, world!");
	ImGui::Checkbox("Demo Window", &show_demo_window);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
	ImGui::InputInt("Number of Waves", &data.wave_count);
	if (ImPlot::BeginPlot("Wave form")) {
		PLOT_WAVE_SHAPE(WS_SINE, 1.0f, 0.0f, 0.0f);
		PLOT_WAVE_SHAPE(WS_SAW, 0.0f, 1.0f, 0.0f);
		PLOT_WAVE_SHAPE(WS_SQUARE, 0.0f, 0.0f, 1.0f);
		PLOT_WAVE_SHAPE(WS_TRIANGLE, 1.0f, 0.0f, 1.0f);
		PLOT_WAVE_SHAPE(WS_WHITE_NOISE, 1.0f, 1.0f, 0.0f);
		ImPlot::EndPlot();
	}

	last->Show();	

	ImGui::End();

	/*char buffer[128];
	for (int i = 0; i < data.wave_count; ++i)
	{
		memset(buffer, '\0', 128);
		sprintf(buffer, "Wave %d", i + 1);
		ImGui::Begin(buffer);
		ImGui::InputInt("Frequency", &(data.freq[i]), 1, data.freq[i]);
		if (ImGui::BeginCombo("Wave Shape", SHAPE_NAMES[data.wave_shape[i]], 0)) {
			for (int j = 0; j < WS_COUNT; ++j) {
				const bool selected = (data.wave_shape[i] == j);
				if (ImGui::Selectable(SHAPE_NAMES[j], selected)) {
					data.wave_shape[i] = j;
				}

				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::End();
	}
	*/

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow((SDL_Window*)window.native_win);
}

void CloseUI()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(window.gl_context);
	SDL_DestroyWindow((SDL_Window*)window.native_win);
	SDL_Quit();
}

void Osciloscope::Show()
{
	ImGui::Begin("Osciloscope");
	ImGui::InputInt("Frequency", &freq, 1, 10, 0);
	if (ImGui::BeginCombo("Wave Shape", SHAPE_NAMES[shape], 0))
	{
		for (int j = 0; j < WS_COUNT; ++j)
		{
			const bool selected = (shape == (WaveShape)j);
			if (ImGui::Selectable(SHAPE_NAMES[j], selected))
			{
				shape = (WaveShape)j;
			}

			if (selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::End();
}

} // namespace umugu
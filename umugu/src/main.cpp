#include "umugu.h"

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

int main()
{
    umugu::Init();
    umugu::StartAudioStream();

#ifdef __EMSCRIPTEN__
    //io->IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!umugu::PollEvents())
#endif
    {
        umugu::Render();
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    umugu::Close();

    return 0;
}

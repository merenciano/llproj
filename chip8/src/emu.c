#include "ch8cpu.h"
#include "ch8io.h"

int main(int argc, char **argv)
{
    Memory mem;
    Init(&mem);
    InitIO(&(mem.window), argv[1]);
    LoadFile(&mem, argv[1]);

    mem.pc = 512;
    mem.min_instr_duration_ms = 2;
    mem.timers_span = TimeMS();
    mem.instr_span = TimeMS();

    while (1)
    {
        mem.curr_time = TimeMS();
        if ((mem.curr_time - mem.timers_span) > 16)
        {
            mem.timers_span = mem.curr_time;
            UpdateTimers(&mem);
        }
        if (mem.curr_time > (mem.instr_span + mem.min_instr_duration_ms))
        {
            mem.instr_span = mem.curr_time;
            if (!GetInput(mem.pressed_keys))
            {
                break;
            }
            else
            {
                ExecuteInstruction(&mem);
                RenderBitmap(&(mem.window), mem.display);
            }
        }
    }

    CloseIO(&(mem.window));
    return 0;
}


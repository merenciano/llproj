import synth

import dearpygui.dearpygui as dpg
import dearpygui.demo as demo 

def mute(sender, data):
    if data:
        synth.volume = 0.0
    else:
        synth.volume = 1.0

def freq_changed(sender, data):
    synth.set_tone(data)

def bend(sender, data):
    synth.bend(data)

def set_volume(sender, data):
    synth.volume = data

def update():
    dpg.set_value("plot", [synth.plot_x, synth.plot_y])

dpg.create_context()
dpg.create_viewport(title='Sinte', width=1920, height=1080)

with dpg.window() as primary_window:
    dpg.add_checkbox(label="Mute", default_value=False, callback=mute)
    dpg.add_slider_float(label="Volume", default_value=0.5, min_value=0.0 ,max_value=1.0, format="ratio = %.2f", callback=set_volume)
    dpg.add_input_int(label="Freq", default_value=440, min_value=20, max_value=3000, callback=freq_changed)
    dpg.add_slider_int(label="Bending", default_value=0, min_value=-60 ,max_value=60, callback=bend)
    with dpg.plot(label="Wave inspector", height=400, width=-1):
        dpg.add_plot_axis(dpg.mvXAxis, label="samples")
        with dpg.plot_axis(dpg.mvYAxis, label="y"):
            dpg.add_line_series(synth.plot_x, synth.plot_y, tag="plot")

dpg.set_primary_window(primary_window, True)
demo.show_demo()

dpg.setup_dearpygui()
dpg.show_viewport()

while dpg.is_dearpygui_running():
    update()
    dpg.render_dearpygui_frame()

dpg.destroy_context()

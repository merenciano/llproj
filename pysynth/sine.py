import synth

import dearpygui.dearpygui as dpg
import dearpygui.demo as demo 



def config_changed(sender, data):
    print(data)
    if data == True:
        synth.volume = 0.0
    else:
        synth.volume = 1.0

dpg.create_context()
dpg.create_viewport(title='Jejej', width=1920, height=1080)

with dpg.window(tag='Wave nene'):
    dpg.add_text('K pasaaa')
    dpg.add_checkbox(label="Mute", default_value=True, callback=config_changed)

demo.show_demo()

dpg.setup_dearpygui()
dpg.show_viewport()

dpg.set_primary_window("Wave nene", True)
while dpg.is_dearpygui_running():
    dpg.render_dearpygui_frame()

dpg.destroy_context()


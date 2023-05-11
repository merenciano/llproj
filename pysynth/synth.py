import numpy as np
import pyaudio

_wave_inspector_buffers = 200
_buffer_samples = 100

_p = pyaudio.PyAudio()
_fs = 44100
_f = 440
_f_base = _f
_bend = 0
_phase = 0
_current_vol = 1.0
volume = 1.0

plot_x = list(np.linspace(0, _buffer_samples * _wave_inspector_buffers, _buffer_samples * _wave_inspector_buffers))
plot_y = list(np.linspace(0, 0, _buffer_samples * _wave_inspector_buffers))

def _gen_wave_samples(in_data, frame_count, time_info, status):
    global plot_y
    global _phase
    global _current_vol
    wave_data = []
    volume_data = np.linspace(_current_vol, volume, frame_count)

    for i in range(frame_count):
        wave_value = np.sin(_f * 2 * np.pi * (_phase / _fs));
        wave_value *= volume_data[i];
        wave_data.append(wave_value.astype(np.float32))
        _phase += 1
        _phase %= _fs

        plot_y.append(wave_value.astype(np.float32))
        plot_y.pop(0)

    _current_vol = volume
    out = np.asarray(wave_data).tobytes()
    return (out, pyaudio.paContinue)

def _set_f():
    global  _phase
    global  _f
    _phase = int(float(_phase) * (_f / (_f_base + _bend)))
    _f = int(_f_base + _bend)

def set_tone(hz):
    global _f_base
    _f_base = int(hz)
    _set_f()

def bend(hz):
    global _bend
    _bend = int(hz)
    _set_f()

stream = _p.open(format=pyaudio.paFloat32, channels=1, rate=_fs, frames_per_buffer = _buffer_samples, stream_callback = _gen_wave_samples, output=True)
stream.start_stream()

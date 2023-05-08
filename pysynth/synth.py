import numpy as np
import pyaudio

_p = pyaudio.PyAudio()
_fs = 44100
_f = 440.0
_phase = 0
_current_vol = 0.0
volume = 0.0

def _callback(in_data, frame_count, time_info, status):
    global _phase
    global _current_vol
    waveData = []
    #waveData = np.sin(_f * 2 * np.pi * (np.arange(_phase, _phase + frame_count) % _fs) / _fs).astype(np.float32)
    volumeData = np.linspace(_current_vol, volume, frame_count)
    print(volumeData)
    for i in range(frame_count):
        waveData.append(np.sin(_f * 2 * np.pi * (_phase / _fs)).astype(np.float32))
        _phase += 1
        _phase %= _fs

    _current_vol = volume
    out = np.asarray(waveData).tobytes()
    return (out, pyaudio.paContinue)

def set_tone(hz):
    global  _phase
    global  _f
    _phase *= (_f / hz)
    _f = hz 

stream = _p.open(format=pyaudio.paFloat32, channels=1, rate=_fs, frames_per_buffer = 100, stream_callback = _callback, output=True)
stream.start_stream()

"""Inspect the shipped FMOD event and meter its real output without speakers."""
import ctypes as c
import json
import math
import os
from pathlib import Path

root = Path(__file__).resolve().parents[2]
os.add_dll_directory(str(root / 'Bin/Debug'))
core = c.CDLL(str(root / 'Bin/Debug/fmod.dll'))
studio = c.CDLL(str(root / 'Bin/Debug/fmodstudio.dll'))
P = c.c_void_p

def api(dll, name, types):
    fn = getattr(dll, name)
    fn.argtypes = types
    fn.restype = c.c_int
    def call(*args):
        result = fn(*args)
        if result:
            raise RuntimeError(f'{name}: FMOD error {result}')
    return call

class Vec(c.Structure):
    _fields_ = [(k, c.c_float) for k in ('x', 'y', 'z')]

class Attr(c.Structure):
    _fields_ = [(k, Vec) for k in ('position', 'velocity', 'forward', 'up')]

class Meter(c.Structure):
    _fields_ = [('samples', c.c_int), ('peak', c.c_float * 32),
                ('rms', c.c_float * 32), ('channels', c.c_short)]

create = api(studio, 'FMOD_Studio_System_Create', [c.POINTER(P), c.c_uint])
getcore = api(studio, 'FMOD_Studio_System_GetCoreSystem', [P, c.POINTER(P)])
output = api(core, 'FMOD_System_SetOutput', [P, c.c_int])
init = api(studio, 'FMOD_Studio_System_Initialize', [P, c.c_int, c.c_uint, c.c_uint, P])
load = api(studio, 'FMOD_Studio_System_LoadBankFile', [P, c.c_char_p, c.c_uint, c.POINTER(P)])
getevent = api(studio, 'FMOD_Studio_System_GetEvent', [P, c.c_char_p, c.POINTER(P)])
is3d = api(studio, 'FMOD_Studio_EventDescription_Is3D', [P, c.POINTER(c.c_int)])
minmax = api(studio, 'FMOD_Studio_EventDescription_GetMinMaxDistance', [P, c.POINTER(c.c_float), c.POINTER(c.c_float)])
loadsamples = api(studio, 'FMOD_Studio_EventDescription_LoadSampleData', [P])
flushsamples = api(studio, 'FMOD_Studio_System_FlushSampleLoading', [P])
mix = api(core, 'FMOD_System_Update', [P])
instance = api(studio, 'FMOD_Studio_EventDescription_CreateInstance', [P, c.POINTER(P)])
position = api(studio, 'FMOD_Studio_EventInstance_Set3DAttributes', [P, c.POINTER(Attr)])
listener = api(studio, 'FMOD_Studio_System_SetListenerAttributes', [P, c.c_int, c.POINTER(Attr), P])
start = api(studio, 'FMOD_Studio_EventInstance_Start', [P])
update = api(studio, 'FMOD_Studio_System_Update', [P])
flush = api(studio, 'FMOD_Studio_System_FlushCommands', [P])
master = api(core, 'FMOD_System_GetMasterChannelGroup', [P, c.POINTER(P)])
getdsp = api(core, 'FMOD_ChannelGroup_GetDSP', [P, c.c_int, c.POINTER(P)])
meteron = api(core, 'FMOD_DSP_SetMeteringEnabled', [P, c.c_int, c.c_int])
meterget = api(core, 'FMOD_DSP_GetMeteringInfo', [P, P, c.POINTER(Meter)])
release = api(studio, 'FMOD_Studio_System_Release', [P])

def measure(distance):
    system, native = P(), P()
    create(c.byref(system), 0x00020205)
    try:
        getcore(system, c.byref(native))
        output(native, 4)  # FMOD_OUTPUTTYPE_NOSOUND_NRT: offline mixing, no speakers.
        init(system, 128, 4, 0, None)  # synchronous Studio updates
        for filename in ('Master.strings.bank', 'Master.bank'):
            bank = P()
            load(system, os.fsencode(root / 'Bin/Debug/Audio' / filename), 0, c.byref(bank))
        desc = P()
        getevent(system, b'event:/Heavy Roar', c.byref(desc))
        spatial, minimum, maximum = c.c_int(), c.c_float(), c.c_float()
        is3d(desc, c.byref(spatial))
        minmax(desc, c.byref(minimum), c.byref(maximum))
        loadsamples(desc)
        flushsamples(system)
        group, dsp = P(), P()
        master(native, c.byref(group))
        getdsp(group, 0, c.byref(dsp))
        meteron(dsp, 0, 1)
        event = P()
        instance(desc, c.byref(event))
        origin = Attr(Vec(0, 0, 0), Vec(), Vec(0, 0, 1), Vec(0, 1, 0))
        source = Attr(Vec(distance, 0, 0), Vec(), Vec(0, 0, 1), Vec(0, 1, 0))
        listener(system, 0, c.byref(origin), None)
        position(event, c.byref(source))
        start(event)
        flush(system)
        energy, samples = 0.0, 0
        for _ in range(240):
            update(system)
            mix(native)
            info = Meter()
            meterget(dsp, None, c.byref(info))
            energy += sum(info.rms[i] ** 2 for i in range(info.channels)) * info.samples
            samples += info.channels * info.samples
        rms = math.sqrt(energy / max(samples, 1))
        return {'distance_fmod_units': distance, 'is_3d': bool(spatial.value),
                'min_distance': minimum.value, 'max_distance': maximum.value,
                'output_rms': rms}
    finally:
        release(system)

if __name__ == '__main__':
    rows = [measure(d) for d in (0.0, 1.0, 10.0, 1000.0)]
    print(json.dumps(rows, indent=2))
    assert rows[0]['output_rms'] > 0, 'Probe did not capture playback'
    assert rows[-1]['output_rms'] < rows[0]['output_rms'] * 0.25, 'Heavy Roar does not attenuate at far distance'

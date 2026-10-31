# Preset files

`Models/preset.NAM` is the included Neural Amp Modeler model file. It is a model,
not a JUCE parameter-state preset. Load it with the plugin's **load NAM model**
button. Parameter values (GAIN, PEAK REDUCTION, ATTACK, RELEASE, POWER, LIMIT,
and METER) are saved/restored by the host through the plugin state.

Default parameter values:

- GAIN: 50.0
- PEAK REDUCTION: 25.0
- ATTACK: 1.0
- RELEASE: 500.0 ms

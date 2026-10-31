LA-2A Leveling - Light (NAM)

preset.NAM is a compatibility-converted, full-quality standalone WaveNet NAM model.
The source file was a NAM v0.7.0 SlimmableContainer with two embedded WaveNet
submodels (max_value 0.5 and 1.0). This preset selects the max_value=1.0 submodel
and writes its own version/architecture/config/weights/sample_rate as a regular
standalone NAM document. The model weights are preserved; no audio weights were
retrained or synthesized.

Load preset.NAM from the plugin's Load NAM Model button.
Model sample rate: 48000 Hz.
Selected architecture: WaveNet.
Selected submodel: max_value=1.0 (full-size / highest-quality tier).

The project pins NeuralAmpModelerCore v0.6.0 to support this NAM v0.7.0 file format. GitHub Actions runs a load-and-process smoke test on this exact model.

The .gen extension is not a NeuralAmpModelerCore model format by itself. A .gen
file is accepted only if its contents are valid NAM JSON. Proprietary binary .gen
files require their own decoder and cannot be loaded by NAM Core.

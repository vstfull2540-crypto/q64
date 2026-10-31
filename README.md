# LA-2A Arm Nong Muang — GitHub Ready (FIXED54 REALISTIC HARDWARE KNOBS)

โปรเจกต์ JUCE/C++ สำหรับปลั๊กอิน **VST3 + Standalone** บน Windows พร้อม NeuralAmpModelerCore และไฟล์โมเดลที่แนบมา

## สิ่งที่เพิ่มใน FIXED47

- เพิ่ม `Models/preset.NAM` ซึ่งเป็นสำเนาโมเดล `LA-2A Leveling – Light.nam` ที่ผู้ใช้อัปโหลด (NAM JSON v0.7.0 / `SlimmableContainer`).
- เพิ่มเอกสาร `Models/README.txt` และ `Presets/README.md` อธิบายวิธีโหลดโมเดลและความแตกต่างระหว่าง NAM model กับ parameter preset.
- ปรับบัฟเฟอร์รับ/ส่ง NAM ให้ใช้ชนิด `NAM_SAMPLE` โดยตรง แทนการ cast หน่วยความจำจาก `double*` เป็น `NAM_SAMPLE*` ซึ่งอาจทำให้ข้อมูลเสียงเสียหายเมื่อชนิด sample ไม่ตรงกัน.
- คงค่าเริ่มต้น GAIN 50.0 และ **PEAK REDUCTION 25.0**.
- GitHub Actions จะ build VST3/Standalone, ทดสอบโหลดและประมวลผลเสียงด้วย `Models/preset.NAM` จริง แล้วแนบโมเดลไปใน artifact ด้วย หากโมเดลโหลดไม่ผ่าน workflow จะ fail

> หมายเหตุ: ปุ่ม `load NAM model` ใช้เลือก `Models/preset.NAM` ด้วยตนเอง ไม่ได้ auto-load จาก path ของโปรเจกต์ เพราะ path ของไฟล์โมเดลเปลี่ยนได้เมื่อย้าย VST3 ไปเครื่องอื่น

## โครงสร้างโปรเจกต์

```text
.
├── .github/workflows/build-windows.yml
├── Assets/LA2A_Interface.png
├── Models/preset.NAM
├── Presets/README.md
├── Source/PluginEditor.cpp
├── Source/PluginEditor.h
├── Source/PluginProcessor.cpp
├── Source/PluginProcessor.h
├── CMakeLists.txt
└── README.md
```

## วิธี Build ผ่าน GitHub Actions (แนะนำ)

1. แตกไฟล์ ZIP แล้วเปิดโฟลเดอร์โปรเจกต์ที่มี `CMakeLists.txt` อยู่ระดับบนสุด
2. สร้าง repository ใหม่บน GitHub เช่น `LA2A-Arm-Nong-Muang` แล้วกด **Create repository**
3. อัปโหลดไฟล์และโฟลเดอร์ทั้งหมดใน ZIP ไปที่ root ของ repository (อย่าอัปโหลดโฟลเดอร์ชั้นนอกที่ครอบโปรเจกต์อีกชั้น)
4. ตรวจสอบว่าเห็นไฟล์ `.github/workflows/build-windows.yml` ในหน้า repository โดยเปิด/แสดง hidden files ด้วยหากจำเป็น
5. กด **Commit changes** เพื่อส่งไฟล์ขึ้น GitHub
6. เปิดแท็บ **Actions** → เลือก workflow ชื่อ **Build LA-2A VST3 (Windows)**
7. รอ job `build-windows` ทำงานจนจบ หากสำเร็จจะเป็นเครื่องหมายสีเขียว
8. เปิดรายการ workflow run ล่าสุด เลื่อนลงส่วน **Artifacts** แล้วดาวน์โหลด `LA2A-Windows-VST3`
9. แตก artifact จะได้ไฟล์ `.vst3`, Standalone `.exe` (ถ้าถูกสร้าง) และ `Models/preset.NAM`

ทุกครั้งที่ push ไป branch `main` หรือ `master` หรือเปิด Pull Request workflow จะทำงานอัตโนมัติ นอกจากนี้สั่งเองได้จาก **Actions → Build LA-2A VST3 (Windows) → Run workflow**

## วิธี Build บน Windows ด้วยตนเอง

ต้องติดตั้ง Visual Studio 2022 พร้อม workload **Desktop development with C++**, CMake 3.22+, Git และ Windows 10/11 SDK

เปิด **x64 Native Tools Command Prompt for VS 2022** ที่ root โปรเจกต์ แล้วรัน:

```bat
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_QUIET=OFF
cmake --build build --config Release --parallel 2
```

JUCE 8.0.6, NeuralAmpModelerCore v0.6.0 (รองรับ NAM file v0.7.0), Eigen 5.0.1 และ AudioDSPTools จะถูกดึงมาโดย CMake ระหว่าง configure ครั้งแรก จึงต้องเชื่อมต่ออินเทอร์เน็ต

## วิธีเปิดใช้งานใน Nuendo 15

1. ดาวน์โหลด artifact จาก GitHub Actions และแตกไฟล์
2. คัดลอกโฟลเดอร์ `LA-2A Arm Nong Muang.vst3` ไปยังโฟลเดอร์ VST3 ของผู้ใช้ เช่น `%LOCALAPPDATA%\\Programs\\Common\\VST3` (หรือโฟลเดอร์ VST3 ที่ Nuendo สแกนอยู่)
3. เปิด Nuendo 15 แล้วสั่ง rescan ปลั๊กอิน VST3 หากยังไม่ปรากฏ
4. แทรก **LA-2A Arm Nong Muang** ลงบน audio track
5. กดปุ่ม **load NAM model** แล้วเลือก `Models/preset.NAM` ที่อยู่ในโฟลเดอร์ artifact
6. ตรวจข้อความสถานะบนหน้าปลั๊กอิน หากโหลดไม่สำเร็จ ข้อความจะแสดงสาเหตุที่ตรวจพบ

## ค่าตั้งต้น

| Parameter | Default |
|---|---:|
| GAIN | 50.0 |
| PEAK REDUCTION | 25.0 |
| ATTACK | 1.0 |
| RELEASE | 500.0 ms |

## ข้อจำกัดและการตรวจสอบ

- `.NAM` เป็นโมเดล Neural Amp Modeler ไม่ใช่ไฟล์ preset ของปุ่ม/พารามิเตอร์ JUCE โดยตรง
- ตัวโหลดนี้ส่ง JSON ให้ NeuralAmpModelerCore และรองรับโมเดลที่ Core โหลดได้และมี I/O แบบ mono-in / mono-out; ปลั๊กอินสร้าง DSP แยกสำหรับ L/R
- `.gen` ที่เป็น binary proprietary ไม่สามารถแปลงให้เป็น NAM ด้วยการเปลี่ยนนามสกุลได้; ต้องมี decoder/SDK ของรูปแบบนั้น
- ไฟล์ workflow มีขั้นตอน build และรวบรวม artifact แต่การสร้าง ZIP นี้ไม่ได้รัน MSVC build หรือทดสอบเสียงจริงใน Nuendo 15 ดังนั้นต้องรอผล GitHub Actions และทดสอบโหลดโมเดลบนเครื่องจริงก่อนถือว่าผ่าน

## NAM model compatibility note

`Models/preset.NAM` is the full-size (`max_value = 1.0`) WaveNet submodel extracted
from the supplied v0.7.0 `SlimmableContainer`. This converts the container wrapper
to a conventional standalone WaveNet NAM document while preserving the selected
submodel's config and weights. The plugin loads `.nam` files through NAM Core's
canonical file-path loader so Core performs its own schema/version validation.

To use it, open the plugin and choose `Models/preset.NAM` with **Load NAM Model**.
The preset is not a parameter-state preset; it is the neural model itself.


## NAM v0.7.0 compatibility fix (FIXED50)

The project is restored from FIXED48 (the earlier UI/parameter baseline; it does not include the later FIXED49 program-dependent release experiment). The bundled `Models/preset.NAM` is the exact full-size `max_value=1.0` WaveNet child from the uploaded `SlimmableContainer`; its architecture, config, weights, sample rate, and metadata are preserved.

The previous build pinned NeuralAmpModelerCore v0.5.4. This project now pins v0.6.0, whose documented support includes NAM file version 0.7.0. GitHub Actions builds `nam_model_smoke_test` and runs it against `Models/preset.NAM`: the test loads the file through `nam::get_dsp`, checks mono I/O, resets at 48 kHz, processes a 512-sample test signal, and rejects non-finite or all-zero output. This is an automated model-load/process check, not a substitute for listening tests inside Nuendo.

### NAM Core parser registration note

The NAM Core sources are compiled as a CMake OBJECT library. This is intentional:
NAM architecture/config parsers are registered by static initialization in
translation units that may not otherwise be directly referenced. Building NAM
Core as a STATIC archive can allow the linker to omit those object files, which
causes runtime errors such as `No config parser registered for architecture:
WaveNet`. The Windows workflow runs a real load-and-process smoke test against
`Models/preset.NAM` before publishing artifacts.


## Realistic hardware rotary controls (FIXED54)

The GAIN and PEAK REDUCTION controls are real JUCE rotary sliders connected to APVTS parameters. FIXED54 upgrades the procedural hardware rendering with a deeper 12-lobed grip, layered black/chrome bezels, curved specular reflection arcs, multi-pass cast shadows, a brushed aluminum cap with fine radial tool marks and concentric lathe grooves, and a raised beveled silver pointer. The artwork is rendered in C++ and scales with the editor; the faceplate PNG remains the background. Mouse drag and wheel changes update the JUCE slider/APVTS parameter and host automation. Double-click now resets GAIN to 50.0 and PEAK REDUCTION to 25.0 (the earlier version opened a numeric dialog). Numeric readouts remain editable by clicking the displayed value.


## FIXED54 verification status

- The supplied FIXED53 archive was checked for ZIP integrity before editing.
- FIXED54 changes the rotary-control drawing and double-click reset behavior while retaining the existing faceplate, NAM model, processor, CMake configuration, and GitHub Actions workflow.
- This delivery has not been compiled with MSVC or run inside Nuendo 15 in this environment. The GitHub Actions workflow is the authoritative Windows build and NAM smoke-test check; confirm its run is green before installing the artifact.


## FIXED55 — Photorealistic PNG hardware knobs

The GAIN and PEAK REDUCTION rotary controls now use the embedded
`Assets/LA2A_Hardware_Knob.png` 512x512 transparent PNG instead of the
procedural vector knob renderer. The image is compiled into the plugin with
`juce_add_binary_data`, so the VST3 does not depend on an external PNG file
at runtime. JUCE rotates the embedded image from the live slider position;
the existing APVTS attachments, host automation, mouse drag, wheel handling,
and double-click defaults remain in place.

The supplied faceplate remains `Assets/LA2A_Interface.png`.


## FIXED56 — High-clarity, smooth and aligned hardware knob update

- `Assets/LA2A_Hardware_Knob.png` is a 2048 x 2048 RGBA master, enlarged with Lanczos and mild detail sharpening for smoother rotation/scaling.
- The rotary drawing compensates for the source PNG's transparent alpha margins so the visible knob centre is aligned to the faceplate centre.
- Gain and Peak Reduction control centres are set to the measured faceplate coordinates (290, 330) and (1208, 330) in the 1498 x 775 design space.
- GitHub Actions validates the knob PNG resolution and alpha-capable pixel format before compiling.
- Windows/MSVC, NAM smoke-test, and Nuendo host validation still need to be confirmed by running the workflow and testing the resulting plugin.


## FIXED57 — Larger, sharper, smoother, PNG-aligned hardware knobs

- The original 2048x2048 RGBA hardware-knob PNG remains embedded with `juce_add_binary_data`; no external PNG file is required at plugin runtime.
- GAIN and PEAK REDUCTION slider drawing areas are enlarged from 190px to 205px at the 1498x775 faceplate coordinate scale (about 8% larger). The visible alpha silhouette remains within the printed tick scale and above the numeric readout.
- The visible PNG center is mathematically aligned to the faceplate pivots at (290,330) and (1208,330), using the measured non-transparent pixel bounds of the actual 2048px asset.
- JUCE `highResamplingQuality` is used during fractional-angle rotation and editor scaling. The entire PNG assembly—including its raised beveled silver pointer—rotates from the live rotary slider position. The sliders remain attached to APVTS `gain` and `peak`, so UI gestures and host automation share the same parameter values.
- Mouse drag, mouse wheel, editable numeric values and double-click reset behavior are retained.

FIXED57 has been packaged and ZIP integrity checked. A Windows/MSVC build, NAM runtime smoke test on Windows, and Nuendo 15 listening/visual test still require a GitHub Actions run and the user's host environment; they are not claimed as passed here.


## FIXED58 — Fixed outer bezel / inner-only rotary

- The knob is rendered from two embedded transparent PNG layers.
- `LA2A_Knob_Outer_Static.png` is drawn without rotation.
- `LA2A_Knob_Inner_Rotor.png` contains the scalloped center and raised silver pointer and is the only rotating layer.
- The rotary sweep is explicitly 205° to 475°; the source pointer orientation is calibrated to 205°, so the pointer follows the normalized parameter value (including host automation and preset changes).
- Both layers are embedded with `juce_add_binary_data`; no external asset path is needed at runtime.

- The rotor pivot is the exact source-image centre (1024, 1024); no off-centre alpha-bounds compensation is applied, so the silver cap stays on the faceplate dial centre.


## FIXED61 — Piecewise pointer calibration (125° / 260° / 425°)

This release is based on FIXED59. The dial pointer is calibrated from the
normalized APVTS slider value using a two-segment mapping:
- 0% -> 125 degrees (lower-left zero tick)
- 50% -> 260 degrees (upper/midpoint tick)
- 100% -> 425 degrees (lower-right 100 tick)

The midpoint is intentionally piecewise rather than a single linear mapping,
because 260 degrees is not the arithmetic midpoint of 125 and 425 degrees.
The source pointer orientation in the inner rotor PNG is treated as 205 degrees.
The outer bezel remains static and only the inner rotor layer is rotated.


## FIXED62 — Updated pointer calibration

The project is based on FIXED59 and updates only the rotary pointer angle calibration:
- Parameter 0%: 128° (lower-left zero tick)
- Parameter 50%: 260° (top midpoint)
- Parameter 100%: 418° (lower-right full-scale tick)

The pointer angle is piecewise interpolated on either side of 50%, so the requested
midpoint is exact. The outer bezel remains static and only the inner rotor image
rotates. The source pointer artwork reference remains 205°.

Validation: ZIP structure and source calibration strings were checked. A Windows
MSVC build, NAM audio smoke test, and Nuendo 15 visual/automation test still need
to be run in GitHub Actions and the DAW.


## FIXED63 — 128° / 263° / 418° calibration and smooth centered bezel

- Based on the supplied FIXED62 project.
- Pointer mapping: parameter 0 -> 128°, parameter 50 -> 263°, parameter 100 -> 418°.
- The mapping is piecewise so the midpoint is exactly 263°.
- The outer static bezel remains non-rotating.
- Applied a subtle 1.25 px Gaussian smoothing pass to the outer PNG alpha silhouette only; RGB metal texture and image center are preserved.
- The inner rotor continues to rotate about the existing centered JUCE transform pivot; no intentional X/Y skew or translation was added.
- Windows/MSVC, NAM audio processing, and Nuendo 15 validation must still be run in CI/DAW.


## FIXED65 — Use Original Outer Ring PNG Directly

- The outer ring is rendered directly from `Assets/LA2A_Knob_Outer_Static.png` via JUCE `ImageCache` and `Graphics::drawImage`.
- The outer ring image is static and is not rotated, recolored, blurred, or procedurally redrawn by the plugin.
- The inner moving hardware remains sourced from `Assets/LA2A_Knob_Inner_Rotor.png`.
- The original FIXED63 outer PNG artwork is retained unchanged in this version.
- Validation performed: source-code path check, PNG dimensions/mode check, required-file check, and ZIP integrity check.
- Windows/MSVC build, GitHub Actions execution, NAM audio smoke test, and Nuendo 15 validation have not been run in this environment.


## FIXED66 — Correct PNG Layer Separation

- `Assets/LA2A_Knob_Outer_Static.png` is now a static annulus-only PNG layer with the inner rotor region transparent.
- `Assets/LA2A_Knob_Inner_Rotor.png` now contains the central rotor/cap and the original pointer only; the duplicate outer ring is transparent.
- Both assets retain 2048x2048 RGBA dimensions and are rendered as separate PNG layers by the existing JUCE `ImageCache` / `Graphics::drawImage` path.
- The pointer/inner rotor can rotate without carrying a duplicate outer bezel; the outer ring stays fixed.
- A composite preview was generated to visually inspect the two separated layers.
- Validation performed: PNG dimensions/mode, layer alpha separation, source references, required project files, and ZIP integrity.
- Windows/MSVC build, GitHub Actions execution, NAM audio smoke test, and Nuendo 15 host validation have not been run in this environment.


## FIXED67 — Grooveless Outer Ring PNG

- Removed concentric groove/ridge shading from the outer static ring image itself.
- Replaced the annulus surface with a continuous matte-black radial gradient; no narrow concentric highlights or groove lines are drawn.
- Preserved the original PNG alpha mask, transparent center, outer silhouette, 2048x2048 RGBA dimensions, and PNG-based rendering path.
- Inner rotor remains a separate transparent-background PNG layer.
- Validation performed: PNG dimensions/mode, alpha-mask preservation, groove-free generated annulus pixels, project files, and ZIP integrity.
- Windows/MSVC build, GitHub Actions execution, NAM audio smoke test, and Nuendo 15 validation have not been run in this environment.


## FIXED51 v3 — Reduced outer bezel

The static outer ring is rendered at 90% of the rotor drawing size and remains centered and stationary. The inner PNG rotor remains full size and continues to rotate from the APVTS slider value.


## FIXED51 v6 — Left-shifted controls

- Based on `FIXED51_ROTOR_ROTATING_v5`.
- Moved both complete rotary controls 20 faceplate pixels to the left.
- Moved the corresponding numeric readout labels 20 faceplate pixels left.
- Preserved the 50% fixed outer ring, inner rotating rotor, original PNG assets,
  NAM model, JUCE sources, tests, and GitHub Actions workflow.

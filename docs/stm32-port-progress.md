# STM32 Port Implementation Plan and Progress

## Scope
Branch: `feature/stm32-serial-comm-and-displays`  
Target: STM32 BlackPill F411CE controller for Gaggiuino Lego V3, using serial comms to the ESP32 display.

## Current Status
Working now:
- `controller-stm32-gaggiuino-lego-v3` builds successfully.
- `display-sunton43` builds successfully.
- `gaggiuino-display-sunton5` and `gaggiuino-display-sunton43` now share the same LVGL draw-buffer strategy for Sunton panels (single draw buffer, internal RAM, ~20-line chunk), which resolved recent display sync instability observed during small frequent UI updates.
- Sunton-target LVGL heap allocation is now routed to internal RAM (`SUNTON5_DISPLAY` / `SUNTON43_DISPLAY`) instead of PSRAM in `lv_conf.h`, reducing allocator-related jitter during active redraw.
- STM32 entrypoint, board config, serial transport, heater path, valve path, alt relay path, button path, and MAX6675 thermocouple path are wired.
- Serial display handshake was hardened in commit `096301b6 fix: harden serial display handshake`.
- `STM32DimmedPump` now follows the original ESP32 `PSM` model again: zero-cross driven burst-fire / cycle-skipping across full AC cycles, rather than delayed phase-angle firing inside each half-cycle.
- Serial payload handling now rejects malformed sensor, autotune, output-control, PID, pump-model, autotune-start, and LED-control payloads before applying them.
- Serial parsing now also validates numeric and boolean field contents before calling `toInt()` / `toFloat()`, so malformed payloads can no longer silently turn into zero-valued commands or button states.
- Serial PID, autotune-result, and pump-model messages now require exact supported field counts instead of accepting trailing or partially-shaped CSV payloads.
- Digital input polling now initializes from the live pin state and applies a simple debounce window at a 20 ms poll interval, reducing the chance of stray brew/steam button transitions.
- STM32 startup now skips addon I2C probing when the selected board config does not define addon bus pins, avoiding blind LED/ToF probing on unsupported wiring.
- STM32 addon I2C setup now applies the configured SDA/SCL pins before `Wire.begin()`, so future addon-capable STM32 board configs no longer depend on whatever the core happens to pick as default I2C pins.
- The STM32 controller build now enables a bench-only USB CDC monitor behind `STM32_USB_BENCH_MONITOR`. Over `SerialUSB`, it can override raw pump output with simple commands like `pump 25`, `status`, and `off`, with an automatic 5 second timeout back to safe outputs.
- STM32 USB bench-monitor startup now waits non-busily for CDC enumeration, blinks the built-in LED while waiting, and avoids float-formatted `SerialUSB.printf()` in the command response path after runtime corruption was observed when issuing `pump <value>` over USB CDC.
- A second STM32 PlatformIO environment, `controller-stm32-gaggiuino-lego-v3-dfu`, now exists for USB DFU flashing. The original ST-Link upload target remains unchanged.
- Experimental STM32 PlatformIO environments pinned to `ststm32@19.5.0` now exist as `controller-stm32-gaggiuino-lego-v3-pio19` and `controller-stm32-gaggiuino-lego-v3-pio19-dfu`. The first compile exposed stricter `[[noreturn]]` prototype matching in the newer GCC/STM32 stack, which was fixed by updating the corresponding task-function declarations in the thermocouple and pressure-sensor headers. Both `pio19` environments now build, and the `pio19-dfu` firmware has been flashed successfully for USB CDC runtime testing.
- STM32 logging no longer writes to generic `Serial` by default. On the BlackPill core, generic `Serial` maps to `Serial1`, which is also the controller/display UART, so leaving logs enabled there would corrupt the serial protocol.
- Display-controller transport is now moving to build-time configuration instead of NVS. The new `gaggiuino-display-sunton5` and `gaggiuino-display-sunton43` environments default to serial controller comms on `GPIO44/GPIO43 @ 115200`, while the generic display environments remain BLE-oriented. This avoids a serial-only display silently falling back to BLE after NVS loss.
- The display settings API no longer treats controller communication mode or UART pins as mutable user settings.
- Display builds now compile only the selected controller-link client path where possible: the Gaggiuino display environments omit BLE transport client code, while the generic BLE-oriented display environments omit the serial transport client code.
- Display-side BLE assumptions were reduced for serial mode: OTA setup now checks for a real BLE controller before dereferencing it, generic tare/LED commands go through the transport-agnostic comm client, and BLE-only UI elements no longer assume BLE exists when the display is using UART.
- The OTA page now marks controller OTA as unavailable in serial mode and the backend refuses controller-update requests unless the display is actually connected over BLE. This avoids offering a controller DFU flow that only works with the BLE bootloader path.
- Controller-side ping timeout handling now latches once per timeout event instead of re-running the same heater/pump shutdown path every control loop after comms are already lost.
- STM32 startup no longer inherits the full 5 second boot delay used on the ESP32 path; it now uses a short pause instead so serial comms and peripheral setup come up promptly on the BlackPill controller.
- STM32 phase-angle timing now derives its maximum firing delay from the configured half-cycle length instead of a fixed 50 Hz-style ceiling, which keeps the timer math internally consistent for both 50 Hz and 60 Hz mains configurations.
- STM32 controller main-loop cadence is now tighter than the legacy ESP32 path, reducing serial command latency and sensor update spacing from 250 ms to 50 ms on the UART controller build.
- STM32 Lego V3 pressure support is now enabled in board config using the BlackPill default I2C pins (`PB7` SDA, `PB6` SCL), so the controller can instantiate the pressure sensor path instead of treating pressure as unsupported hardware.
- Pressure capability is now only advertised after the ADS1115 pressure sensor initializes successfully at boot. If pressure-sensor bring-up fails, the controller logs the fault and drops pressure support for that boot instead of reporting a misleading half-working capability set.
- Display-side volumetric availability no longer depends on `NIGHTLY_BUILD` for the serial controller path. When BLE scale data is unavailable, the display now allows controller-side flow estimation whenever the connected controller reports both `dimming` and `pressure` capabilities.
- Advanced pressure/flow output-control messages now fail safe on the controller if pressure capability is unavailable, rather than reusing the dimmed-pump path under a mismatched capability set.

Known incomplete or risky:
- `STM32DimmedPump` now matches the original burst-fire control model more closely, but it still needs real-hardware validation to confirm the zero-cross detector cadence and cycle-skipping behavior are clean on the STM32 wiring.
- STM32 pressure support is enabled in software, but the real sensor wiring, calibration, and pressure-to-volumetric behavior still need hardware validation on an actual Lego V3 machine.
- `detectAddon()` is still a TODO.
- STM32 I2C addon path uses generic `Wire.begin()` only; pin mapping and electrical behavior are not verified.
- Sunton 4.3 timing still uses `SUNTON43_RGB_TIMING_FREQ_HZ = 16MHz`; if residual whole-frame horizontal phase nudge is still visible on some panels, timing A/B (for example 14MHz vs 16MHz plus porch/polarity profile checks) remains open.

## Priority Plan
### 1. Validate pump dimming on real hardware
- Confirm the STM32 zero-cross input produces the expected once-per-half-cycle interrupt cadence on the actual hardware.
- Confirm the burst-fire output holds the SSR in the expected on/off state across whole AC cycles and follows the same skip pattern as the original ESP32 behavior.
- Verify behavior at low, medium, and high pump power, especially during startup and rapid setpoint changes.
- Use the STM32 USB bench monitor for fixed-output tests instead of normal brew/steam UI flows, since the monitor sends direct raw pump percentages without involving profile or advanced flow-control logic.

### 2. Stabilize serial transport
- Keep the non-blocking handshake introduced in `096301b6`.
- Add malformed-frame / bad-payload counters and logs.
- Add stricter field-count validation per message type.
- Consider explicit disconnect/reconnect plugin events if UI state needs to react cleanly.

### 3. Validate pressure and volumetric behavior on Lego V3
- Confirm the pressure sensor is detected reliably on the BlackPill default I2C pins and produces sane live readings.
- Verify controller-side pressure reporting, pump control, and flow-estimation volumetrics behave correctly with the serial display path.
- Check calibration assumptions before treating volumetric dosing as production-ready.

### 4. Verify peripherals on real hardware
- Confirm brew/steam switch polarity and debounce behavior.
- Confirm MAX6675 fault handling with disconnected probe and overtemp.
- Confirm I2C addon behavior for PCA9634 / TOF on STM32, or disable until proven.

### 5. Tighten display-side comm UX
- Consider pre-filling board-specific serial defaults for the intended STM32 + display pair.
- Consider gating BLE-only UI affordances when serial mode is selected.
- If hot-switching comm mode is needed later, add explicit transport reinitialization instead of relying on restart.

## Real Hardware Test Order
1. Scope zero-cross input and SSR control output at several power levels.
2. Verify UART disconnect fails safe: heater off, pump off, valve closed, alt off.
3. Verify display boot ordering: display first, controller first, reconnect after unplug.
4. Verify thermocouple fault handling and startup behavior.
5. Run a supervised brew/steam cycle and compare commanded vs actual pump behavior.

## Suggested Next Session Start
1. Re-open `lib/GaggiMateController/src/peripherals/STM32DimmedPump.cpp`.
2. Prepare a scope-based validation checklist for zero-cross cadence and burst-fire output state.
3. Flash `controller-stm32-gaggiuino-lego-v3` and capture traces at several pump levels.
4. If timing is stable, move to pressure-sensor and volumetric validation on real hardware.

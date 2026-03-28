# STM32 Port Implementation Plan and Progress

## Scope
Branch: `feature/stm32-serial-comm-and-displays`  
Target: STM32 BlackPill F411CE controller for Gaggiuino Lego V3, using serial comms to the ESP32 display.

## Current Status
Working now:
- `controller-stm32-gaggiuino-lego-v3` builds successfully.
- `display-sunton43` builds successfully.
- STM32 entrypoint, board config, serial transport, heater path, valve path, alt relay path, button path, and MAX6675 thermocouple path are wired.
- Serial display handshake was hardened in commit `096301b6 fix: harden serial display handshake`.
- `STM32DimmedPump` now uses an STM32 `HardwareTimer` (`TIM2`) to schedule triac firing from zero-cross instead of relying on a 30 ms polling loop. This is a meaningful safety improvement, but it still needs oscilloscope validation on real hardware.
- The triac gate pulse is now timer-driven in two phases (`wait-to-fire` then `pulse-active`) rather than using a blocking `delayMicroseconds()` inside the timer callback.
- Serial payload handling now rejects malformed sensor, autotune, output-control, PID, pump-model, autotune-start, and LED-control payloads before applying them.
- Digital input polling now initializes from the live pin state and applies a simple debounce window at a 20 ms poll interval, reducing the chance of stray brew/steam button transitions.
- STM32 startup now skips addon I2C probing when the selected board config does not define addon bus pins, avoiding blind LED/ToF probing on unsupported wiring.

Known incomplete or risky:
- `STM32DimmedPump` is still prototype-grade. `setupTimer()` is empty and triac firing is checked from a 30 ms task loop, which is not acceptable for precise 50/60 Hz phase-angle control.
- STM32 config currently sets `pressure = false`, so pressure sensing, pressure-target control, and controller-side volumetric reporting are not functionally ported.
- `detectAddon()` is still a TODO.
- Display-side serial/comm-mode settings exist in preferences, but there is no normal UI or API flow to enable/configure them.
- STM32 I2C addon path uses generic `Wire.begin()` only; pin mapping and electrical behavior are not verified.

## Priority Plan
### 1. Make pump dimming hardware-timer based
- Replace the `micros()` polling scheme in `STM32DimmedPump` with a one-shot `HardwareTimer`.
- On zero-cross ISR: arm timer for `_firingDelayMicros`.
- On timer ISR: pulse the SSR/TRIAC gate for `TRIAC_PULSE_WIDTH_US`, then stop or disarm the timer.
- Keep zero-cross ISR minimal and avoid blocking calls there.
- Validate with an oscilloscope before trusting brew behavior.

### 2. Stabilize serial transport
- Keep the non-blocking handshake introduced in `096301b6`.
- Add malformed-frame / bad-payload counters and logs.
- Add stricter field-count validation per message type.
- Consider explicit disconnect/reconnect plugin events if UI state needs to react cleanly.

### 3. Decide feature envelope for Lego V3
- If this board has no pressure sensor: document pressure features as unsupported and gate UI/settings accordingly.
- If pressure support is intended: add the real sensor path and calibration flow before testing volumetrics.

### 4. Verify peripherals on real hardware
- Confirm brew/steam switch polarity and debounce behavior.
- Confirm MAX6675 fault handling with disconnected probe and overtemp.
- Confirm I2C addon behavior for PCA9634 / TOF on STM32, or disable until proven.

### 5. Expose serial mode cleanly on the display
- Add a supported config path for `comm_mode`, `serial_rx`, `serial_tx`, and `serial_baud`.
- Alternatively, set fixed defaults for the intended STM32 + display wiring and document them.

## Real Hardware Test Order
1. Scope zero-cross input and triac gate output at several power levels.
2. Verify UART disconnect fails safe: heater off, pump off, valve closed, alt off.
3. Verify display boot ordering: display first, controller first, reconnect after unplug.
4. Verify thermocouple fault handling and startup behavior.
5. Run a supervised brew/steam cycle and compare commanded vs actual pump behavior.

## Suggested Next Session Start
1. Re-open `lib/GaggiMateController/src/peripherals/STM32DimmedPump.cpp`.
2. Implement `HardwareTimer`-based one-shot firing.
3. Rebuild `controller-stm32-gaggiuino-lego-v3`.
4. Prepare a scope-based validation checklist before flashing hardware.

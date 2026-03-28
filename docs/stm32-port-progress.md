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
- Serial parsing now also validates numeric and boolean field contents before calling `toInt()` / `toFloat()`, so malformed payloads can no longer silently turn into zero-valued commands or button states.
- Digital input polling now initializes from the live pin state and applies a simple debounce window at a 20 ms poll interval, reducing the chance of stray brew/steam button transitions.
- STM32 startup now skips addon I2C probing when the selected board config does not define addon bus pins, avoiding blind LED/ToF probing on unsupported wiring.
- STM32 logging no longer writes to generic `Serial` by default. On the BlackPill core, generic `Serial` maps to `Serial1`, which is also the controller/display UART, so leaving logs enabled there would corrupt the serial protocol.
- Display-side settings now expose controller communication mode, serial RX/TX pins, and baud rate through `/api/settings` and the WebUI settings page. This makes the STM32 serial path configurable without manual NVS edits, but it still requires a display restart to take effect.
- The WebUI now warns when controller communication settings have changed but the display has not yet been restarted, reducing the chance of saving UART changes and assuming they are already active.
- The display settings API now bounds-checks serial pin and baud-rate inputs before saving them, so obviously invalid UART values from the WebUI will be ignored instead of becoming persisted startup config.
- Display-side BLE assumptions were reduced for serial mode: OTA setup now checks for a real BLE controller before dereferencing it, generic tare/LED commands go through the transport-agnostic comm client, and BLE-only UI elements no longer assume BLE exists when the display is using UART.
- The OTA page now marks controller OTA as unavailable in serial mode and the backend refuses controller-update requests unless the display is actually connected over BLE. This avoids offering a controller DFU flow that only works with the BLE bootloader path.

Known incomplete or risky:
- `STM32DimmedPump` is now software-structured correctly around `HardwareTimer`, but it remains unproven on real mains hardware until zero-cross and gate timing are checked on a scope.
- STM32 config currently sets `pressure = false`, so pressure sensing, pressure-target control, and controller-side volumetric reporting are not functionally ported.
- `detectAddon()` is still a TODO.
- STM32 I2C addon path uses generic `Wire.begin()` only; pin mapping and electrical behavior are not verified.

## Priority Plan
### 1. Validate pump dimming on real hardware
- Confirm the new `HardwareTimer` path produces stable fire delays at low, medium, and high power.
- Confirm the gate pulse width is sufficient for reliable latching and does not repeat within the same half-cycle.
- Verify behavior at both startup and mode transitions where pump power changes quickly.

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

### 5. Tighten display-side comm UX
- Consider pre-filling board-specific serial defaults for the intended STM32 + display pair.
- Consider gating BLE-only UI affordances when serial mode is selected.
- If hot-switching comm mode is needed later, add explicit transport reinitialization instead of relying on restart.

## Real Hardware Test Order
1. Scope zero-cross input and triac gate output at several power levels.
2. Verify UART disconnect fails safe: heater off, pump off, valve closed, alt off.
3. Verify display boot ordering: display first, controller first, reconnect after unplug.
4. Verify thermocouple fault handling and startup behavior.
5. Run a supervised brew/steam cycle and compare commanded vs actual pump behavior.

## Suggested Next Session Start
1. Re-open `lib/GaggiMateController/src/peripherals/STM32DimmedPump.cpp`.
2. Prepare a scope-based validation checklist for zero-cross and gate timing.
3. Flash `controller-stm32-gaggiuino-lego-v3` and capture traces at several pump levels.
4. If timing is stable, move to the next missing capability: pressure/volumetric scope or display comm UX polish.

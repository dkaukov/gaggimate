#include "BLECommClient.h"

#if defined(ESP32) && !defined(DISPLAY_CONTROLLER_SERIAL_ONLY)

BLECommClient::BLECommClient() {}

void BLECommClient::init() { _ble.initClient(); }

void BLECommClient::loop() {
    // BLE uses callbacks, no polling needed
}

bool BLECommClient::connect() { return _ble.connectToServer(); }

bool BLECommClient::isReadyForConnection() { return _ble.isReadyForConnection(); }

bool BLECommClient::isConnected() { return _ble.isConnected(); }

String BLECommClient::readInfo() { return String(_ble.readInfo().c_str()); }

void BLECommClient::sendOutputControl(bool valve, float pumpSetpoint, float boilerSetpoint) {
    _ble.sendOutputControl(valve, pumpSetpoint, boilerSetpoint);
}

void BLECommClient::sendAdvancedOutputControl(bool valve, float boilerSetpoint, bool pressureTarget, float pressure,
                                              float flow) {
    _ble.sendAdvancedOutputControl(valve, boilerSetpoint, pressureTarget, pressure, flow);
}

void BLECommClient::sendAltControl(bool pinState) { _ble.sendAltControl(pinState); }

void BLECommClient::sendPing() { _ble.sendPing(); }

void BLECommClient::sendAutotune(int testTime, int samples) { _ble.sendAutotune(testTime, samples); }

void BLECommClient::sendPidSettings(const String &pid) { _ble.sendPidSettings(pid); }

void BLECommClient::sendPumpModelCoeffs(const String &pumpModelCoeffs) { _ble.sendPumpModelCoeffs(pumpModelCoeffs); }

void BLECommClient::setPressureScale(float scale) { _ble.setPressureScale(scale); }

void BLECommClient::sendLedControl(uint8_t channel, uint8_t brightness) { _ble.sendLedControl(channel, brightness); }

void BLECommClient::tare() { _ble.tare(); }

void BLECommClient::registerRemoteErrorCallback(const comm_remote_err_callback_t &callback) {
    _ble.registerRemoteErrorCallback(callback);
}

void BLECommClient::registerBrewBtnCallback(const comm_brew_callback_t &callback) {
    _ble.registerBrewBtnCallback(callback);
}

void BLECommClient::registerSteamBtnCallback(const comm_steam_callback_t &callback) {
    _ble.registerSteamBtnCallback(callback);
}

void BLECommClient::registerSensorCallback(const comm_sensor_read_callback_t &callback) {
    _ble.registerSensorCallback(callback);
}

void BLECommClient::registerAutotuneResultCallback(const comm_pid_control_callback_t &callback) {
    _ble.registerAutotuneResultCallback(callback);
}

void BLECommClient::registerVolumetricMeasurementCallback(const comm_float_callback_t &callback) {
    _ble.registerVolumetricMeasurementCallback(callback);
}

void BLECommClient::registerTofMeasurementCallback(const comm_int_callback_t &callback) {
    _ble.registerTofMeasurementCallback(callback);
}

#endif // ESP32 && !DISPLAY_CONTROLLER_SERIAL_ONLY

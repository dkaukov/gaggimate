#include "BLECommServer.h"

#ifdef ESP32

BLECommServer::BLECommServer() {}

void BLECommServer::init(const String &infoString) { _ble.initServer(infoString); }

void BLECommServer::setInfo(const String &infoString) { _ble.setInfo(infoString); }

void BLECommServer::loop() {
    // BLE uses callbacks, no polling needed
}

bool BLECommServer::isConnected() {
    // NimBLEServerController doesn't expose connection state directly
    // but we can infer it from the callback behavior
    return _connected;
}

void BLECommServer::sendSensorData(float temperature, float pressure, float puckFlow, float pumpFlow,
                                   float puckResistance) {
    _ble.sendSensorData(temperature, pressure, puckFlow, pumpFlow, puckResistance);
}

void BLECommServer::sendError(int errorCode) { _ble.sendError(errorCode); }

void BLECommServer::sendBrewBtnState(bool brewButtonStatus) { _ble.sendBrewBtnState(brewButtonStatus); }

void BLECommServer::sendSteamBtnState(bool steamButtonStatus) { _ble.sendSteamBtnState(steamButtonStatus); }

void BLECommServer::sendAutotuneResult(float Kp, float Ki, float Kd) { _ble.sendAutotuneResult(Kp, Ki, Kd); }

void BLECommServer::sendVolumetricMeasurement(float value) { _ble.sendVolumetricMeasurement(value); }

void BLECommServer::sendTofMeasurement(int value) { _ble.sendTofMeasurement(value); }

void BLECommServer::registerOutputControlCallback(const comm_simple_output_callback_t &callback) {
    _ble.registerOutputControlCallback(callback);
}

void BLECommServer::registerAdvancedOutputControlCallback(const comm_advanced_output_callback_t &callback) {
    _ble.registerAdvancedOutputControlCallback(callback);
}

void BLECommServer::registerAltControlCallback(const comm_pin_control_callback_t &callback) {
    _ble.registerAltControlCallback(callback);
}

void BLECommServer::registerPidControlCallback(const comm_pid_control_callback_t &callback) {
    _ble.registerPidControlCallback(callback);
}

void BLECommServer::registerPumpModelCoeffsCallback(const comm_pump_model_coeffs_callback_t &callback) {
    _ble.registerPumpModelCoeffsCallback(callback);
}

void BLECommServer::registerPingCallback(const comm_ping_callback_t &callback) { _ble.registerPingCallback(callback); }

void BLECommServer::registerAutotuneCallback(const comm_autotune_callback_t &callback) {
    _ble.registerAutotuneCallback(callback);
}

void BLECommServer::registerPressureScaleCallback(const comm_float_callback_t &callback) {
    _ble.registerPressureScaleCallback(callback);
}

void BLECommServer::registerTareCallback(const comm_void_callback_t &callback) { _ble.registerTareCallback(callback); }

void BLECommServer::registerLedControlCallback(const comm_led_control_callback_t &callback) {
    _ble.registerLedControlCallback(callback);
}

#endif // ESP32

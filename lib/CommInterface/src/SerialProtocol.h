#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include <Arduino.h>

/**
 * @brief Serial Protocol for GaggiMate Controller Communication
 *
 * Frame format: <STX><TYPE>:<PAYLOAD><CHECKSUM><ETX>
 * - STX (0x02): Start of frame
 * - TYPE: Single character message type
 * - PAYLOAD: CSV-formatted data (same format as BLE messages)
 * - CHECKSUM: 2-char hex XOR of TYPE and PAYLOAD bytes
 * - ETX (0x03): End of frame
 *
 * Example: <STX>S:92.5,9.2,0.8,1.2,2.1<CHECKSUM><ETX>
 */

namespace SerialProtocol {

// Frame delimiters
constexpr char STX = 0x02; // Start of text
constexpr char ETX = 0x03; // End of text
constexpr char TYPE_DELIMITER = ':';

// Message types - Controller to Display (Server -> Client)
constexpr char MSG_SENSOR_DATA = 'S';      // Sensor data (temp,pressure,flows)
constexpr char MSG_ERROR = 'E';            // Error code
constexpr char MSG_BREW_BTN = 'B';         // Brew button state
constexpr char MSG_STEAM_BTN = 'T';        // Steam button state (T for toggle)
constexpr char MSG_INFO = 'I';             // System info JSON
constexpr char MSG_AUTOTUNE_RESULT = 'R';  // Autotune result (PID values)
constexpr char MSG_VOLUMETRIC = 'V';       // Volumetric measurement
constexpr char MSG_TOF = 'D';              // Distance (ToF) measurement

// Message types - Display to Controller (Client -> Server)
constexpr char MSG_OUTPUT_CONTROL = 'O';       // Output control (valve, pump, heater)
constexpr char MSG_ALT_CONTROL = 'A';          // Alt relay control
constexpr char MSG_PING = 'P';                 // Ping keepalive
constexpr char MSG_PID_SETTINGS = 'K';         // PID settings (Kp, Ki, Kd, Kf)
constexpr char MSG_PUMP_MODEL = 'M';           // Pump model coefficients
constexpr char MSG_AUTOTUNE_START = 'U';       // Start autotune
constexpr char MSG_PRESSURE_SCALE = 'C';       // Pressure scale
constexpr char MSG_TARE = 'W';                 // Tare volumetric
constexpr char MSG_LED_CONTROL = 'L';          // LED control
constexpr char MSG_REQUEST_INFO = 'Q';         // Request system info

// Protocol constants
constexpr size_t MAX_FRAME_SIZE = 256;
constexpr size_t CHECKSUM_SIZE = 2;
constexpr uint32_t DEFAULT_BAUD_RATE = 115200;
constexpr uint32_t FRAME_TIMEOUT_MS = 100;

/**
 * @brief Calculate XOR checksum of data
 * @param data Pointer to data buffer
 * @param length Length of data
 * @return XOR checksum byte
 */
inline uint8_t calculateChecksum(const char *data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= static_cast<uint8_t>(data[i]);
    }
    return checksum;
}

/**
 * @brief Convert checksum byte to 2-char hex string
 * @param checksum Checksum byte
 * @param output Output buffer (must be at least 3 bytes)
 */
inline void checksumToHex(uint8_t checksum, char *output) {
    const char hexChars[] = "0123456789ABCDEF";
    output[0] = hexChars[(checksum >> 4) & 0x0F];
    output[1] = hexChars[checksum & 0x0F];
    output[2] = '\0';
}

/**
 * @brief Parse 2-char hex string to checksum byte
 * @param hex Hex string (2 characters)
 * @return Checksum byte
 */
inline uint8_t hexToChecksum(const char *hex) {
    auto hexCharToInt = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        return 0;
    };
    return (hexCharToInt(hex[0]) << 4) | hexCharToInt(hex[1]);
}

/**
 * @brief Build a complete frame from type and payload
 * @param type Message type character
 * @param payload Payload string
 * @param output Output buffer
 * @param maxSize Maximum output buffer size
 * @return Length of frame, or 0 on error
 */
inline size_t buildFrame(char type, const String &payload, char *output, size_t maxSize) {
    // Calculate required size: STX + type + ':' + payload + checksum(2) + ETX
    size_t requiredSize = 1 + 1 + 1 + payload.length() + CHECKSUM_SIZE + 1;
    if (requiredSize > maxSize) {
        return 0;
    }

    // Build the content for checksum (type + ':' + payload)
    String content = String(type) + TYPE_DELIMITER + payload;

    // Calculate checksum
    uint8_t checksum = calculateChecksum(content.c_str(), content.length());
    char checksumHex[3];
    checksumToHex(checksum, checksumHex);

    // Build frame
    size_t pos = 0;
    output[pos++] = STX;
    for (size_t i = 0; i < content.length(); i++) {
        output[pos++] = content[i];
    }
    output[pos++] = checksumHex[0];
    output[pos++] = checksumHex[1];
    output[pos++] = ETX;

    return pos;
}

/**
 * @brief Frame parser state machine
 */
enum class ParseState { WAIT_STX, READ_TYPE, READ_PAYLOAD, READ_CHECKSUM, COMPLETE };

/**
 * @brief Result of frame parsing
 */
struct ParseResult {
    bool valid;
    char type;
    String payload;

    ParseResult() : valid(false), type(0), payload("") {}
    ParseResult(bool v, char t, const String &p) : valid(v), type(t), payload(p) {}
};

/**
 * @brief Frame parser class for receiving serial data
 */
class FrameParser {
  public:
    FrameParser() : state(ParseState::WAIT_STX), checksumIndex(0), frameStartTime(0) {}

    /**
     * @brief Reset parser to initial state
     */
    void reset() {
        state = ParseState::WAIT_STX;
        type = 0;
        payload = "";
        checksumStr[0] = 0;
        checksumStr[1] = 0;
        checksumIndex = 0;
        frameStartTime = 0;
    }

    /**
     * @brief Process a single byte
     * @param byte Input byte
     * @return ParseResult with valid=true when a complete frame is received
     */
    ParseResult processByte(uint8_t byte) {
        // Check for timeout
        if (state != ParseState::WAIT_STX && millis() - frameStartTime > FRAME_TIMEOUT_MS) {
            reset();
        }

        switch (state) {
        case ParseState::WAIT_STX:
            if (byte == STX) {
                reset();
                state = ParseState::READ_TYPE;
                frameStartTime = millis();
            }
            break;

        case ParseState::READ_TYPE:
            type = static_cast<char>(byte);
            state = ParseState::READ_PAYLOAD;
            // Skip the ':' delimiter - will be read as first char of payload
            break;

        case ParseState::READ_PAYLOAD:
            if (byte == ETX) {
                // End of frame - last 2 chars are checksum
                if (payload.length() >= CHECKSUM_SIZE + 1) { // +1 for ':'
                    checksumStr[0] = payload[payload.length() - 2];
                    checksumStr[1] = payload[payload.length() - 1];
                    // Remove checksum and leading ':' from payload
                    payload = payload.substring(1, payload.length() - CHECKSUM_SIZE);

                    // Verify checksum
                    String content = String(type) + TYPE_DELIMITER + payload;
                    uint8_t expectedChecksum = calculateChecksum(content.c_str(), content.length());
                    uint8_t receivedChecksum = hexToChecksum(checksumStr);

                    if (expectedChecksum == receivedChecksum) {
                        ParseResult result(true, type, payload);
                        reset();
                        return result;
                    }
                }
                reset();
            } else if (payload.length() < MAX_FRAME_SIZE - 10) {
                payload += static_cast<char>(byte);
            } else {
                // Payload too long, reset
                reset();
            }
            break;

        default:
            reset();
            break;
        }

        return ParseResult();
    }

  private:
    ParseState state;
    char type;
    String payload;
    char checksumStr[3];
    size_t checksumIndex;
    unsigned long frameStartTime;
};

/**
 * @brief Helper to get a token from CSV string
 * @param from Source string
 * @param index Token index (0-based)
 * @param separator Separator character
 * @param defaultValue Default value if token not found
 * @return Token string
 */
inline String getToken(const String &from, uint8_t index, char separator, const String &defaultValue = "") {
    int startIndex = 0;
    int endIndex = 0;
    uint8_t currentIndex = 0;

    while (currentIndex <= index) {
        endIndex = from.indexOf(separator, startIndex);
        if (endIndex == -1) {
            if (currentIndex == index) {
                return from.substring(startIndex);
            }
            return defaultValue;
        }
        if (currentIndex == index) {
            return from.substring(startIndex, endIndex);
        }
        startIndex = endIndex + 1;
        currentIndex++;
    }
    return defaultValue;
}

/**
 * @brief Count separator-delimited tokens in a string
 * @param from Source string
 * @param separator Separator character
 * @return Number of tokens
 */
inline size_t countTokens(const String &from, char separator) {
    if (from.length() == 0) {
        return 0;
    }

    size_t count = 1;
    for (size_t i = 0; i < from.length(); i++) {
        if (from[i] == separator) {
            count++;
        }
    }
    return count;
}

} // namespace SerialProtocol

#endif // SERIALPROTOCOL_H

#pragma once

#include <span>

#include <asio/serial_port.hpp>

namespace telnet {
class Telnet;
}

namespace uart {
enum class ESP32BootMode {
    Reset,
    Bootloader,
    Normal,
};
class UART {
public:
    UART(asio::io_context &io_context, const char *device);
    void Open();
    void SetReceiveCallback(telnet::Telnet *telnet);
    void Send(std::span<const uint8_t> bytes);
    void SetBaud(uint32_t baud);
    void ESP32EnterBootMode(ESP32BootMode mode)
    {
        /*
        DTR RTS EN BOOT
        1   1   1  1     Normal
        0   0   1  1     Normal (*)
        1   0   0  1     Reset
        0   1   1  0     Bootloader

        (*) Mode due to Espressif DevKit board autodownload circuit to avoid reset
        when plugging USB in some OS
        */
        SetDTR(mode == ESP32BootMode::Bootloader);
        SetRTS(mode == ESP32BootMode::Reset);
    }

private:
    void SetDTR(bool);
    void SetRTS(bool);
    void ReceiveLoop();

    asio::serial_port m_serial_port;
    const char *const m_device;
    uint8_t m_buffer[128];
    uint32_t m_baud;
    telnet::Telnet *m_callback;
};
}  // namespace uart

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
        The following sequences work both with:
        - Native RTS/DTR connection to EN/BOOT (or bridge chip)
        - USB-Serial/JTAG port.
        */
        if (mode == ESP32BootMode::Normal) {
            SetRTS(true);
            SetDTR(true);
        } else if (mode == ESP32BootMode::Bootloader) {
            SetRTS(true);
            SetDTR(false);
            SetRTS(false);
            SetDTR(true);  // USB-Serial/JTAG in download mode, Native disabled

            SetDTR(false);
            SetRTS(true);  // USB-Serial/JTAG in download mode, Native in download mode
        } else if (mode == ESP32BootMode::Reset) {
            SetDTR(true);
            SetRTS(true);
            SetRTS(false);  // USB-Serial/JTAG port reset, Native disabled
        }
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

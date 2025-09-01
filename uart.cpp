#include "uart.hpp"

#include <iostream>

#include <asio/ts/buffer.hpp>

#include "telnet.hpp"

namespace uart {

UART::UART(asio::io_context &io_context, const char *device)
    : m_serial_port{io_context}, m_device{device}
{
}
void UART::Open()
{
    // open on first telnet connection only
    if (m_serial_port.is_open()) return;
    m_serial_port.open(m_device);
    ESP32EnterBootMode(ESP32BootMode::Normal);
    SetBaud(115200);
    m_serial_port.set_option(asio::serial_port::character_size(8));
    m_serial_port.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
    m_serial_port.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
    m_serial_port.set_option(
        asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    ReceiveLoop();
}
void UART::Send(std::span<const uint8_t> bytes)
{
    std::error_code ec;
    asio::write(m_serial_port, asio::buffer(bytes), ec);
}
void UART::SetBaud(uint32_t baud)
{
    if (baud == m_baud) return;
    m_baud = baud;
    std::cout << "Set baud to " << baud << '\n';
    m_serial_port.set_option(asio::serial_port::baud_rate(baud));
}
void UART::SetRTS(bool enabled)
{
    int data = TIOCM_RTS;
    if (ioctl(m_serial_port.native_handle(), enabled ? TIOCMBIS : TIOCMBIC, &data)) throw;
}
void UART::SetDTR(bool enabled)
{
    int data = TIOCM_DTR;
    if (ioctl(m_serial_port.native_handle(), enabled ? TIOCMBIS : TIOCMBIC, &data)) throw;
}
void UART::SetReceiveCallback(telnet::Telnet *telnet) { m_callback = telnet; }
void UART::ReceiveLoop()
{
    asio::async_read(m_serial_port, asio::buffer(m_buffer), asio::transfer_at_least(1),
                     [this](std::error_code ec, const size_t n) {
                         if (ec == asio::error::operation_aborted) return;
                         if (ec == asio::error::eof) {
                             std::cerr << "UART disconnected!\n";
                             std::exit(1);
                         }
                         if (ec) throw asio::system_error(ec);
                         m_callback->Send({m_buffer, n});
                         ReceiveLoop();
                     });
}
}  // namespace uart

#include "telnet.hpp"

#include <iostream>

#include <asio/ts/buffer.hpp>

#include "uart.hpp"

namespace {
enum class TelnetCmd : uint8_t {
    NotSet = 0,
    Escaped = 1,
    SE = 240,
    NOP = 241,
    DataMark = 242,
    Break = 243,
    InterruptProcess = 244,
    Abortoutput = 245,
    Areyouthere = 246,
    Erasecharacter = 247,
    EraseLine = 248,
    Goahead = 249,
    SB = 250,
    WILL = 251,
    WONT = 252,
    DO = 253,
    DONT = 254,
    IAC = 255,
};
}
namespace telnet {
Telnet::Telnet(asio::io_context &io_context, uint16_t port)
    : m_io_context{io_context},
      m_acceptor{io_context, asio::ip::tcp::endpoint(asio::ip::address_v6::any(), port)}
{
    AcceptLoop();
}
void Telnet::SetUART(uart::UART *uart) { m_uart = uart; }
void Telnet::Send(const std::span<const uint8_t> bytes)
{
    for (const auto b : bytes) {
        if (b == std::to_underlying(TelnetCmd::IAC)) {
            // slow-path, need to escape
            std::vector<uint8_t> escaped;
            escaped.reserve(bytes.size() + 1);  // at least size + 1
            for (const auto b : bytes) {
                if (b == std::to_underlying(TelnetCmd::IAC)) escaped.push_back(b);
                escaped.push_back(b);
            }
            std::error_code ec;
            asio::write(m_socket, asio::buffer(escaped), ec);
            return;
        }
    }
    // fast-path, no need to copy and escape
    std::error_code ec;
    asio::write(m_socket, asio::buffer(bytes), ec);
}
void Telnet::AcceptLoop()
{
    m_cmd.clear();
    m_escape = false;
    m_socket = asio::ip::tcp::socket{m_io_context};
    m_acceptor.async_accept(m_socket, [this](std::error_code ec) {
        if (ec == asio::error::operation_aborted) return;
        if (ec) throw asio::system_error(ec);
        std::cout << "Telnet connection from " << m_socket.remote_endpoint() << '\n';
        // open on first telnet connection
        m_uart->Open();
        ReceiveLoop();
    });
}
void Telnet::ReceiveLoop()
{
    m_socket.async_read_some(asio::buffer(m_buffer), [this](std::error_code ec, std::size_t n) {
        if (ec == asio::error::operation_aborted) return;
        if (ec == asio::error::eof || ec == asio::error::connection_reset) {
            // expected errors on disconnect
            // After a flash this will power again the device and be ready for a
            // new connection
            std::cout << "Finished telnet connection\n";
            m_uart->SetBaud(115200);
            m_uart->ESP32EnterBootMode(uart::ESP32BootMode::Normal);
            AcceptLoop();
            return;
        }
        if (ec) throw asio::system_error(ec);
        Parse({m_buffer, n});
        ReceiveLoop();
    });
}
void Telnet::ParseCmd()
{
    auto rfc2217 = [this](const std::span<const uint8_t> cmd) {
        if (cmd[0] == 1 && cmd.size() == 5) {  // SET-BAUD
            const uint32_t baud = (cmd[1] << 24) | (cmd[2] << 16) | (cmd[3] << 8) | (cmd[4]);
            m_uart->SetBaud(baud);
        } else if (cmd[0] == 2 && cmd.size() == 2) {  // SET-DATASIZE
        } else if (cmd[0] == 3 && cmd.size() == 2) {  // SET-PARITY
        } else if (cmd[0] == 4 && cmd.size() == 2) {  // SET-STOPSIZE
        } else if (cmd[0] == 5 && cmd.size() == 2) {
            auto c = cmd[1];
            if (c == 1) {
            } else if (c == 8) {
                m_uart->ESP32EnterBootMode(uart::ESP32BootMode::Bootloader);
            } else if (c == 9) {
            } else if (c == 11) {
                m_uart->ESP32EnterBootMode(uart::ESP32BootMode::Reset);
            } else if (c == 12) {
            } else {
            }
        } else if (cmd[0] == 12 && cmd.size() == 2) {  // PURGE-DATA
        } else {
            return false;
        }
        return true;
    };
    const auto size = m_cmd.size();
    assert(size >= 1);
    assert(m_cmd[0] == std::to_underlying(TelnetCmd::IAC));
    if (size < 3) return;
    auto CMD = m_cmd[1];
    auto OPT = m_cmd[2];
    if (CMD == std::to_underlying(TelnetCmd::DO)) {
        if (OPT == 1) {
            // this is the first message received by idf.py, and we need to send
            // also a single message, so do it here. in case of a raw tcp connection
            // we don't send this message to avoid some trash on scre
            std::error_code ec;
            uint8_t answer[] = {std::to_underlying(TelnetCmd::IAC),
                                std::to_underlying(TelnetCmd::DO), 44};
            asio::write(m_socket, asio::buffer(answer), ec);
        }
    } else if (CMD == std::to_underlying(TelnetCmd::WILL)) {
    } else if (size >= 5 && CMD == std::to_underlying(TelnetCmd::SB) && OPT == 44 &&
               m_cmd[size - 2] == std::to_underlying(TelnetCmd::IAC) &&
               m_cmd[size - 1] == std::to_underlying(TelnetCmd::SE)) {
        if (rfc2217({m_cmd.data() + 3, size - 5})) {
            // ACK
            m_cmd[3] += 100;
            std::error_code ec;
            asio::write(m_socket, asio::buffer(m_cmd), ec);
        }
    } else {
        return;
    }
    // command complete
    m_cmd.clear();
}
void Telnet::Parse(const std::span<const uint8_t> bytes)
{
    auto tx_payload = [this](const std::span<const uint8_t> payload) {
        uint16_t size = 1;
        while (size < payload.size() && payload[size] != std::to_underlying(TelnetCmd::IAC)) ++size;
        m_uart->Send(payload.subspan(0, size));
        return size;
    };
    for (unsigned i = 0; i < bytes.size(); ++i) {
        const auto b = bytes[i];
        if (m_escape) {
            m_escape = false;
            if (b != std::to_underlying(TelnetCmd::IAC))  // escaped command sequence
            {
                m_cmd.push_back(std::to_underlying(TelnetCmd::IAC));
            }
        } else if (b == std::to_underlying(TelnetCmd::IAC)) {
            m_escape = true;
            continue;
        }
        if (!m_cmd.empty()) {
            m_cmd.push_back(b);
            ParseCmd();
            continue;
        }
        const auto consumed = tx_payload({bytes.subspan(i)});
        i += (consumed - 1);
    }
}
}  // namespace telnet

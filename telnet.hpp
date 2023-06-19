#pragma once

#include <cstdint>
#include <span>

#include <asio/ts/internet.hpp>

namespace uart {
class UART;
}

namespace telnet {

class Telnet {
public:
  Telnet(asio::io_context &io_context, uint16_t port);
  void SetUART(uart::UART *uart);
  void Send(std::span<const uint8_t> bytes);

private:
  void AcceptLoop();
  void ReceiveLoop();
  void Parse(std::span<const uint8_t> cmd);
  void ParseCmd();

  asio::io_context &m_io_context;
  asio::ip::tcp::acceptor m_acceptor;
  asio::ip::tcp::socket m_socket{m_io_context};
  uint8_t m_buffer[128];
  bool m_escape;
  std::vector<uint8_t> m_cmd;
  uart::UART *m_uart;
};

} // namespace telnet

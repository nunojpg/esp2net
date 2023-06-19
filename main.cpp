#include <iostream>

#include "telnet.hpp"
#include "uart.hpp"

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <serial port> <tcp port>\n";
    return 1;
  }

  asio::io_context io_context;
  telnet::Telnet telnet(io_context, std::atoi(argv[2]));
  uart::UART uart(io_context, argv[1]);
  telnet.SetUART(&uart);
  uart.SetReceiveCallback(&telnet);
  io_context.run();
  return 0;
}

# natpp

This library lets you create [port mappings](https://en.wikipedia.org/wiki/Port_forwarding) using
the well established Asio C++ networking library.

## Asio

This library integrates deeply with the Asio framework by providing an IO service backend and corresponding IO
object classes for each protocol type.

## Work-in-progress note

Since this library is work-in-progress, currently it only supports the [NATPMP protocol](https://en.wikipedia.org/wiki/NAT_Port_Mapping_Protocol).
However, support for [UPnP port forwarding](https://en.wikipedia.org/wiki/Universal_Plug_and_Play#NAT_traversal) is in the works and should land soon.

## Usage

```c++
#include <asio.hpp>
#include <natpp/natpmp.hpp>

int main()
{
  asio::io_context io;
  nat::natpmp natpmp(io);
  
  natpmp.async_public_address([](nat::error_code error, asio::ip::address addr) {
        if(!error) {
          // TODO print addr
        }
      });
      
  nat::port_mapping mapping;
  mapping.private_port = 55555;
  mapping.public_port = 55555;
  mapping.duration = std::chrono::hours(2);
  natpmp.async_request_mapping(mapping,
      [](nat::error_code error, nat::port_mapping mapping) {
        if(!error) {
          // TODO print mapping
        }
      });
      
  io.run();
}
```

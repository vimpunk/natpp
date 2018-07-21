#ifndef NATPP_GATEWAY_HEADER
#define NATPP_GATEWAY_HEADER

#include "port_mapping.hpp"
#include "error.hpp"

#include <asio/ip/address.hpp>

namespace nat {

asio::ip::address default_gateway_address(error_code& error);

} // nat

#include "impl/gateway.ipp"

#endif // NATPP_GATEWAY_HEADER


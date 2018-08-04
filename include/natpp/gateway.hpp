#ifndef NATPP_GATEWAY_HEADER
#define NATPP_GATEWAY_HEADER

#include "port_mapping.hpp"
#include "error.hpp"

#include <asio/ip/address.hpp>

namespace nat {

/**
 * @brief Returns the IP address of the default gateway that is configured for
 * this host.
 *
 * The implementation does not make any network requests. It instead parses
 * OS dependent config files.
 *
 * @param error The variable through which errors are reported.
 *
 * @return The default gateway address if no error occurred. Otherwise the
 * return value is a default constructed `asio::ip::address` object.
 */
asio::ip::address default_gateway_address(error_code& error);

} // nat

#include "impl/gateway.ipp"

#endif // NATPP_GATEWAY_HEADER


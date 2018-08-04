#ifndef NATPP_PORT_MAPPING_HEADER
#define NATPP_PORT_MAPPING_HEADER

#include <chrono>

namespace nat {

/**
 * This object represents a mapping between host's port and the requested port
 * on the router's WAN facing side.
 */
struct port_mapping
{
    enum { udp, tcp } type;
    // The port on which this host will be listening for connections.
    uint16_t private_port = 0;
    // The port on which the router's WAN facing side will be listening for
    // connections.
    //
    // @note This is usually a suggestion and NAT boxes are free to ignore it
    // and map `private_port` to something else.
    uint16_t public_port = 0;
    // The total lifetime of the mapping. It is advised, however, that the
    // mapping be renewed at an interval half of this value. If it's 0, the NAT
    // box will choose a suitable value.
    std::chrono::seconds duration{0};
};

/** Determines if @p request is a request to remove a mapping. */
inline bool is_remove_mapping_request(const port_mapping& request)
{
    return (request.public_port == 0) && (request.duration == std::chrono::seconds(0));
}

} // nat

#endif // NATPP_PORT_MAPPING_HEADER

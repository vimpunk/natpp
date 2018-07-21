#ifndef NATPP_PORT_MAPPING_HEADER
#define NATPP_PORT_MAPPING_HEADER

#include <chrono>

namespace nat {

/**
 * This object represents a mapping between host's port and the requested port on the
 * router's WAN facing side.
 */
struct port_mapping
{
    enum { udp, tcp } type;
    uint16_t private_port = 0;
    uint16_t public_port = 0;
    std::chrono::seconds duration{};
};

inline bool is_remove_mapping_request(const port_mapping& mapping)
{
    return (mapping.public_port == 0) && (mapping.duration == std::chrono::seconds(0));
}

} // nat

#endif // NATPP_PORT_MAPPING_HEADER

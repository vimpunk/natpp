#ifndef NATPP_NATPMP_IMPL
#define NATPP_NATPMP_IMPL

#include "../natpmp.hpp"

namespace nat {

natpmp::natpmp(asio::io_context& ios) : socket_(ios) {}

} // nat

#endif // NATPP_NATPMP_IMPL

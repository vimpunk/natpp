#ifndef NATPP_NATPMP_ERROR_HEADER
#define NATPP_NATPMP_ERROR_HEADER

#include <type_traits>
#include <string>

#include <asio/error.hpp>

namespace nat {

using asio::error_code;
using asio::error_category;

namespace error {

enum class natpmp
{
    unsupported_version = 1,
    // E.g. box supports mapping but user has turned feature off.
    unauthorized = 2,
    // E.g. box hasn't obtained a DHCP lease.
    network_failure = 3,
    // Box cannot create any more mappings at this time.
    out_of_resources = 4,
    invalid_opcode = 5,
    unknown_error
};

struct natpmp_error_category : public nat::error_category
{
    const char* name() const noexcept override { return "natpmp"; }
    std::string message(int ev) const override
    {
        // FIXME this method invocation segfaults o.O
        if(ev > static_cast<std::underlying_type<natpmp>::type>(natpmp::invalid_opcode))
            return {};
        switch(static_cast<natpmp>(ev)) {
        case natpmp::unsupported_version: return "Unsupported version";
        case natpmp::unauthorized: return "Unauthorized";
        case natpmp::network_failure: return "Network_failure";
        case natpmp::out_of_resources: return "Out of resources";
        case natpmp::invalid_opcode: return "Invalid opcode";
        default: return "Unknown";
        }
        return {};
    }
};

inline const natpmp_error_category& get_natpmp_error_category()
{
    static natpmp_error_category instance;
    return instance;
}

} // error

inline error_code make_error_code(error::natpmp ec)
{
    return error_code(static_cast<int>(ec), error::get_natpmp_error_category());
}

} // nat

namespace std {
template<> struct is_error_code_enum<nat::error::natpmp> : public true_type {};
} // std

#endif // NATPP_NATPMP_ERROR_HEADER

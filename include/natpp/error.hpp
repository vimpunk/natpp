#ifndef NATPP_NATPMP_ERROR_HEADER
#define NATPP_NATPMP_ERROR_HEADER

#include <type_traits>
// TODO generalize
#include <system_error>

#include <asio/error.hpp>

namespace nat {

using asio::error_code;
using asio::error_category;

namespace error {

enum class natpmp
{
    unsupported_version = 1,
    unauthorized,
    network_failure,
    out_of_resources,
    unsupported_opcode,
    invalid_opcode,
};

struct natpmp_error_category : public error_category
{
    const char* name() const noexcept override { return "natpmp"; }
    std::string message(int ev) const override
    {
        switch(static_cast<natpmp>(ev))
        {
        case natpmp::unsupported_version: return "Unsupported version";
        case natpmp::unauthorized: return "Unauthorized";
        case natpmp::network_failure: return "Network_failure";
        case natpmp::out_of_resources: return "Out of resources";
        case natpmp::unsupported_opcode: return "Unsupported version";
        case natpmp::invalid_opcode: return "Invalid opcode";
        default: return "Unknown";
        }
    }
};

inline const natpmp_error_category& natpmp_category()
{
    static natpmp_error_category instance;
    return instance;
}

} // error

inline error_code make_error_code(error::natpmp ec)
{
    return error_code(static_cast<int>(ec), error::natpmp_error_category());
}

} // nat

namespace std {
template<> struct is_error_code_enum<nat::error::natpmp> : public true_type {};
} // std

#endif // NATPP_NATPMP_ERROR_HEADER

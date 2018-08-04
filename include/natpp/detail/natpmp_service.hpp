#ifndef NATPP_NATPMP_SERVICE_HEADER
#define NATPP_NATPMP_SERVICE_HEADER

#include "../port_mapping.hpp"
#include "../error.hpp"

#include <functional>
#include <vector>
#include <deque>
#include <array>
#include <variant>

#include <asio/async_result.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/udp.hpp>
#include <asio/buffer.hpp>

namespace asio { class io_context; } // asio

namespace nat {
namespace detail {

/**
 * @brief A service that implements the NAT-PMP communication protocol for all
 * @ref nat::natpmp objects.
 *
 * Since there is only a single default gateway for a host it makes sense to
 * only have one actual entity communicating with it, regardless of how many
 * times it is used in an application. Thus, @ref nat::natpmp instances act as
 * a frontend for this service backend.
 */
struct natpmp_service : public asio::io_context::service
{
    /**
     * @brief Data for each @ref nat::natpmp instance.
     *
     * Holds all currently active mappings of the client.
     */
    struct natpmp_client
    {
        std::vector<port_mapping> mappings;
    };

    using implementation_type = natpmp_client;

    static asio::execution_context::id id;

private:
    asio::ip::udp::socket socket_;

    // After the first invocation of `public_address` or `async_public_address`,
    // the result is cached here, since it's not expected to change.
    // TODO is this the correct thing to do?
    asio::ip::address public_address_;
    asio::ip::udp::endpoint gateway_endpoint_;

    // References to all the established mappings (which are stored in
    // `implementation_type::mappings`).
    std::vector<std::reference_wrapper<port_mapping>> mappings_;

    struct address_request
    {
        std::function<void(error_code, asio::ip::address)> handler;
    };

    struct mapping_request
    {
        port_mapping mapping;
        std::function<void(error_code, port_mapping)> handler;
    };

    struct pending_request
    {
        std::variant<address_request, mapping_request> data;
        implementation_type& impl;
    };

    // This client is in effect synchronous in that only a single outstanding
    // request to gateway may exist at any given time, and the rest are queued
    // up.
    //
    // The currently executed request is the first item of the queue and is only
    // removed once it's been served.
    std::deque<pending_request> pending_requests_;

    std::array<char, 12> send_buffer_;
    std::array<char, 16> receive_buffer_;

public:
    explicit natpmp_service(asio::io_context& ios);

    void construct(implementation_type& impl)
    {
        impl = implementation_type();
    }

    void move_construct(implementation_type& impl, implementation_type& other_impl)
    {
        move_assign(impl, other_impl);
    }

    void move_assign(implementation_type& impl, implementation_type& other_impl)
    {
        impl = std::move(other_impl);
    }

    void destroy(implementation_type& impl)
    {
        // TODO maybe delete mappings too?
        impl = implementation_type();
    }

    void shutdown();

    asio::ip::address gateway_address(implementation_type& impl)
    {
        return gateway_endpoint_.address();
    }

    asio::ip::address public_address(implementation_type& impl, error_code& error);

    template<typename Handler>
    void async_public_address(implementation_type& impl, Handler handler);

    const std::vector<port_mapping>&
    port_mappings(const implementation_type& impl) const noexcept
    {
        return impl.mappings;
    }

    port_mapping request_mapping(implementation_type& impl,
            const port_mapping& mapping, error_code& error);

    template<typename Handler>
    void async_request_mapping(implementation_type& impl,
            const port_mapping& mapping, Handler handler);

    void remove_mapping(implementation_type& impl,
            const port_mapping& mapping, error_code& error);

    template<typename Handler>
    void async_remove_mapping(implementation_type& impl,
            const port_mapping& mapping, Handler handler);

private:
    void execute_request();

    void async_public_address_impl();
    void async_request_mapping_impl();
    void async_remove_mapping_impl();

    template<typename Request>
    Request pop_current_request();
};

inline asio::execution_context::id natpmp_service::id;

} // detail
} // nat

#include "impl/natpmp_service.ipp"

#endif // NATPP_NATPMP_SERVICE_HEADER

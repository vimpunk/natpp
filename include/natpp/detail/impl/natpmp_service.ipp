#ifndef NATPP_NATPMP_SERVICE_IMPL
#define NATPP_NATPMP_SERVICE_IMPL

#include "../natpmp_service.hpp"
#include "../../gateway.hpp"

#include <cassert>

#include <endian/endian.hpp>
#include <asio/socket_base.hpp>
#include <asio/buffer.hpp>

namespace nat {
namespace detail {

enum opcode
{
    public_address = 0,
    udp_mapping = 1,
    tcp_mapping = 2,
};

natpmp_service::natpmp_service(asio::io_context& ios)
    : asio::io_context::service(ios)
    , socket_(ios)
{
    error_code ec;
    auto gateway_address = default_gateway_address(ec);
    gateway_endpoint_ = asio::ip::udp::endpoint(gateway_address, 5351);

    socket_.connect(gateway_endpoint_, ec);
    if(ec) {
        socket_.close(ec);
        return;
    }

    // Set this option to allow other processes (since there may be more than
    // a single NATPMP client running) to listen on this port as well.
    // TODO verify that this is the equivalent of SO_REUSEPORT
    socket_.set_option(asio::ip::udp::socket::reuse_address(true), ec);
    if(ec) {
        socket_.close(ec);
        return;
    }
}

void natpmp_service::shutdown()
{
    // TODO
}

inline asio::const_buffer prep_public_address_request_message(char* buffer)
{
    buffer[0] = 0;
    buffer[1] = 0;
    return asio::buffer(buffer, 2);
}

inline void verify_response_header(const char* buffer, int opcode, error_code& error)
{
    const auto errc = endian::read<endian::order::network, uint16_t>(&buffer[2]);
    if(errc > 5) {
        error = make_error_code(error::natpmp::unknown_error);
    } else {
        error = make_error_code(static_cast<error::natpmp>(errc));
    }
    if(error) {
        return;
    }

    // Version code must be zero.
    if(buffer[0] != 0) {
        error = make_error_code(error::natpmp::unsupported_version);
        return;
    }
    // The protocol number must be 128 + opcode.
    // NOTE: need to cast to unsigned since char can has a max value of 127.
    if(static_cast<uint8_t>(buffer[1]) != 128 + opcode) {
        error = make_error_code(error::natpmp::invalid_opcode);
        return;
    }
}

inline asio::ip::address parse_public_address_response(
        const char* buffer, error_code& error)
{
    verify_response_header(buffer, opcode::public_address, error);
    if(error) {
        return {};
    }
    return asio::ip::address_v4(endian::read<endian::order::network,
            uint32_t>(&buffer[8]));
}

asio::ip::address natpmp_service::public_address(
        implementation_type& impl, error_code& error)
{
    error = error_code();
    if(public_address_ != asio::ip::address()) {
        // We've already requested this once, and since it's not expected to
        // change, return a cached value.
        return public_address_;
    }

    if(pending_requests_.empty()) {
        // No pending requests, we're free to request away.
        const auto num_sent = socket_.send(
                prep_public_address_request_message(send_buffer_.data()),
                0, error);
        if(error) {
            return {};
        }
        if(num_sent < 2) {
            error = std::make_error_code(std::errc::bad_message);
            return {};
        }

        const auto num_received = socket_.receive(asio::buffer(receive_buffer_),
                0, error);
        if(error) {
            return {};
        }
        if(num_received < 12) {
            error = std::make_error_code(std::errc::bad_message);
            return {};
        }
        error = error_code();
        return parse_public_address_response(receive_buffer_.data(), error);
    } else {
        // Cannot have concurrent requests.
        error = make_error_code(asio::error::try_again);
        return {};
    }
}

void natpmp_service::async_public_address_impl()
{
    // Send the request...
    socket_.async_send(prep_public_address_request_message(send_buffer_.data()),
            [this](const auto& error, const auto num_sent) {
                // Don't do anything if there wasn't an error.
                if(!error && num_sent == 2) { return; }

                auto request = pop_current_request<address_request>();
                if(error) {
                    request.handler(error, {});
                } else if(num_sent != 2) {
                    request.handler(std::make_error_code(std::errc::bad_message), {});
                }

                // Even though this one was a failure, try to execute the next request.
                if(!pending_requests_.empty()) { execute_request(); }
            });

    // ...and simultaneously initiate a receive operation.
    socket_.async_receive(asio::buffer(receive_buffer_),
            [this](auto error, const auto num_received) {
                auto request = pop_current_request<address_request>();
                if(error) {
                    request.handler(error, {});
                    return;
                } else if(num_received < 12) {
                    request.handler(std::make_error_code(std::errc::bad_message), {});
                    return;
                }

                request.handler(error, parse_public_address_response(
                                receive_buffer_.data(), error));

                // If there are any more pending requests, execute them.
                if(!pending_requests_.empty()) { execute_request(); }
            });
}

template<typename Handler>
void natpmp_service::async_public_address(implementation_type& impl, Handler handler)
{
    if(public_address_ != asio::ip::address()) {
        // We already have requested and cached gateway's public address
        // previously, so post the cached value via user's executor.
        asio::post(socket_.get_executor(),
                [handler = std::move(handler), addr = public_address_]
                { handler({}, addr); });
    } else {
        pending_requests_.emplace_back(pending_request{
                address_request{std::move(handler)}, impl});
        if(pending_requests_.size() == 1) {
            // No pending requests (other than the one just added), we're free to
            // request away.
            async_public_address_impl();
        }
    }
}

inline asio::const_buffer prep_mapping_request_message(
        char* buffer, const port_mapping& mapping)
{
    buffer[0] = 0;
    if(mapping.type == port_mapping::udp) {
        buffer[1] = opcode::udp_mapping;
    } else {
        buffer[1] = opcode::tcp_mapping;
    }
    buffer[2] = 0;
    buffer[3] = 0;
    endian::write<endian::order::network, uint16_t>(mapping.private_port, &buffer[4]);
    endian::write<endian::order::network, uint16_t>(mapping.public_port, &buffer[6]);
    endian::write<endian::order::network, uint32_t>(mapping.duration.count(), &buffer[8]);
    return asio::buffer(buffer, 12);
}


inline port_mapping parse_mapping_response(const char* buffer, int opcode,
        error_code& error)
{
    verify_response_header(buffer, opcode, error);
    if(error) {
        return {};
    }

    port_mapping mapping;
    mapping.private_port = endian::read<endian::order::network, uint16_t>(&buffer[8]);
    mapping.public_port = endian::read<endian::order::network, uint16_t>(&buffer[10]);
    mapping.duration = std::chrono::seconds(
            endian::read<endian::order::network, uint32_t>(&buffer[12]));
    return mapping;
}

port_mapping natpmp_service::request_mapping(implementation_type& impl,
        const port_mapping& request, error_code& error)
{
    if(pending_requests_.empty()) {
        error = error_code();
        // No pending requests, we're free to request away.
        const auto num_sent = socket_.send(prep_mapping_request_message(
                        send_buffer_.data(), request),
                0, error);
        if(error) {
            return {};
        }
        if(num_sent < 12) {
            error = std::make_error_code(std::errc::bad_message);
            return {};
        }

        const auto num_received = socket_.receive(asio::buffer(receive_buffer_),
                0, error);
        if(error) {
            return {};
        }
        if(num_received < 12) {
            error = std::make_error_code(std::errc::bad_message);
            return {};
        }

        auto mapping = parse_mapping_response(receive_buffer_.data(),
                request.type == port_mapping::udp
                    ? opcode::udp_mapping : opcode::tcp_mapping,
                error);
        if(!error) {
            impl.mappings.push_back(mapping);
        }
        return mapping;
    } else {
        // Cannot have concurrent requests.
        error = make_error_code(asio::error::try_again);
        return {};
    }
}

void natpmp_service::async_request_mapping_impl()
{
    assert(!pending_requests_.empty());
    assert(std::holds_alternative<mapping_request>(pending_requests_.front().data));
    const auto& request = std::get<mapping_request>(pending_requests_.front().data);
    // Send the request...
    socket_.async_send(prep_mapping_request_message(
                send_buffer_.data(), request.mapping),
            [this](const auto& error, const auto num_sent) {
                // Don't do anything if there wasn't an error.
                if(!error && num_sent == 12) { return; }

                auto request = pop_current_request<mapping_request>();
                if(error) {
                    request.handler(error, {});
                } else if(num_sent != 12) {
                    request.handler(std::make_error_code(std::errc::bad_message), {});
                }

                // Even though this one was a failure, try to execute the next request.
                if(!pending_requests_.empty()) { execute_request(); }
            });

    // ...and simultaneously initiate a receive operation.
    socket_.async_receive(asio::buffer(receive_buffer_),
            [this](auto error, const auto num_received)
            {
                assert(!pending_requests_.empty());
                auto& impl = pending_requests_.front().impl;
                auto request = pop_current_request<mapping_request>();
                if(error) {
                    request.handler(error, {});
                    return;
                } else if(num_received < 12) {
                    request.handler(std::make_error_code(std::errc::bad_message), {});
                    return;
                }

                auto mapping = parse_mapping_response(receive_buffer_.data(),
                        request.mapping.type == port_mapping::udp
                            ? opcode::udp_mapping : opcode::tcp_mapping,
                        error);
                if(!error) {
                    impl.mappings.push_back(mapping);
                }
                request.handler(error, mapping);

                // If there are any more pending requests, execute them.
                if(!pending_requests_.empty()) { execute_request(); }
            });
}

template<typename Handler>
void natpmp_service::async_request_mapping(implementation_type& impl,
        const port_mapping& mapping, Handler handler)
{
    pending_requests_.emplace_back(pending_request{mapping_request{mapping,
            std::move(handler)}, impl});
    if(pending_requests_.size() == 1) {
        // No pending requests (other than the one just added), we're free to
        // request away.
        async_public_address_impl();
    }
}

// Removing a mapping involves the exact same request and response message
// formats, so don't duplicate effort.

void natpmp_service::async_remove_mapping_impl()
{
    async_request_mapping_impl();
}

void natpmp_service::remove_mapping(implementation_type& impl,
        const port_mapping& mapping, error_code& error)
{
    auto request = mapping;
    request.duration = std::chrono::seconds(0);
    request.public_port = 0;
    request_mapping(impl, request, error);
}

template<typename Handler>
void natpmp_service::async_remove_mapping(implementation_type& impl,
        const port_mapping& mapping, Handler handler)
{
    auto request = mapping;
    request.duration = std::chrono::seconds(0);
    request.public_port = 0;
    async_request_mapping(impl, request, std::move(handler));
}

void natpmp_service::execute_request()
{
    assert(!pending_requests_.empty());
    const auto& curr_req = pending_requests_.front();
    if(std::holds_alternative<address_request>(curr_req.data)) {
        async_public_address_impl();
    } else {
        const auto& request = std::get<mapping_request>(curr_req.data);
        if(is_remove_mapping_request(request.mapping)) {
            async_remove_mapping_impl();
        } else {
            async_request_mapping_impl();
        }
    }
}

template<typename Request>
Request natpmp_service::pop_current_request()
{
    assert(!pending_requests_.empty());
    auto curr_req = std::move(pending_requests_.front());
    pending_requests_.pop_front();
    assert(std::holds_alternative<Request>(curr_req.data));
    return std::move(std::get<Request>(curr_req.data));
}

} // detail
} // nat

#endif // NATPP_NATPMP_SERVICE_IMPL

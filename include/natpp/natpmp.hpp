#ifndef NATPP_NATPMP_HEADER
#define NATPP_NATPMP_HEADER

#include "port_mapping.hpp"
#include "error.hpp"
#include "detail/natpmp_service.hpp"

#include <asio/async_result.hpp>

namespace asio { class io_context; }

namespace nat {

struct natpmp : public asio::basic_io_object<detail::natpmp_service>
{
    explicit natpmp(asio::io_context& ios)
        : asio::basic_io_object<detail::natpmp_service>(ios)
    {}

    asio::ip::address gateway_address(error_code& error)
    {
        return this->get_service().gateway_address(this->get_implementation()/*, error*/);
    }

    asio::ip::address public_address(error_code& error)
    {
        return this->get_service().public_address(this->get_implementation(), error);
    }

    template<typename Handler>
    ASIO_INITFN_RESULT_TYPE(Handler, void(error_code, asio::ip::address))
    async_public_address(Handler handler)
    {
        asio::async_completion<Handler,
                void(error_code, asio::ip::address)> init(handler);
        this->get_service().async_public_address(
                this->get_implementation(), std::move(init.completion_handler));
        return init.result.get();
    }

    const std::vector<port_mapping>& port_mappings() const noexcept
    {
        return this->get_service().port_mappings(this->get_implementation());
    }

    port_mapping request_mapping(const port_mapping& mapping, error_code& error)
    {
        return this->get_service().request_mapping(
                this->get_implementation(), mapping, error);
    }

    template<typename Handler>
    ASIO_INITFN_RESULT_TYPE(Handler, void(error_code, port_mapping))
    async_request_mapping(const port_mapping& mapping, Handler handler)
    {
        asio::async_completion<Handler,
                void(error_code, port_mapping)> init(handler);
        this->get_service().async_request_mapping(
                this->get_implementation(), mapping,
                std::move(init.completion_handler));
        return init.result.get();
    }

    void remove_mapping(const port_mapping& mapping, error_code& error)
    {
        return this->get_service().remove_mapping(
                this->get_implementation(), mapping, error);
    }

    template<typename Handler>
    ASIO_INITFN_RESULT_TYPE(Handler, void(error_code))
    async_remove_mapping(const port_mapping& mapping, Handler handler)
    {
        asio::async_completion<Handler, void(error_code)> init(handler);
        this->get_service().async_remove_mapping(
                this->get_implementation(), mapping,
                std::move(init.completion_handler));
        return init.result.get();
    }
};

} // nat

#endif // NATPP_NATPMP_HEADER

#ifndef NATPP_NATPMP_HEADER
#define NATPP_NATPMP_HEADER

#include "port_mapping.hpp"
#include "error.hpp"
#include "detail/natpmp_service.hpp"

#include <asio/async_result.hpp>

namespace asio { class io_context; }

namespace nat {

/**
 * Provides a facility to create port mappings using the NAT-PMP protocol.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
struct natpmp : public asio::basic_io_object<detail::natpmp_service>
{
    /**
     * Constructs a natpmp object.
     *
     * @param io_context The io_context object that the datagram socket will use
     * to dispatch handlers for any asynchronous operations performed on this
     * object.
     */
    explicit natpmp(asio::io_context& io_context)
        : asio::basic_io_object<detail::natpmp_service>(io_context)
    {}

    /** Returns the default gateway address of this host. */
    asio::ip::address gateway_address()
    {
        return this->get_service().gateway_address(this->get_implementation());
    }

    /**
     * @brief Requests the address of the WAN facing side of this host's default
     * gateway.
     *
     * @param error Set to indicate what error occurered, if any.
     *
     * @return The public gateway address, or undefined if @p error is set.
     *
     * @note If there are any pending asynchronous requests enqueued in the
     * underlying @ref natpmp_service, @p error will be set to
     * `asio::error::try_again`.
     */
    asio::ip::address public_address(error_code& error)
    {
        return this->get_service().public_address(this->get_implementation(), error);
    }

    /**
     * @brief Requests the address of the WAN facing side of this host's default
     * gateway.
     *
     * @param handler The handler to be called when the public address request
     * operation completes.
     * The function signature of the handler must be:
     * @code void handler(natpp::error_code, asio::ip::address); @endcode
     * Regardless of whether the asynchronous operation completes immediately or
     * not, the handler will not be invoked from within this function. Invocation
     * of the handler will be performed in a manner equivalent to using
     * asio::io_context::post.
     */
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

    /**
     * Returns all active mappings associated with this instance.
     */
    const std::vector<port_mapping>& port_mappings() const noexcept
    {
        return this->get_service().port_mappings(this->get_implementation());
    }

    /**
     * @brief Requests a port mapping to be made between this host and the
     * default gateway.
     *
     * @param mapping Holds the details of the mapping request.
     *
     * @param error Set to indicate what error occurered, if any.
     *
     * @return The port mapping that the NAT box created, which may differ from
     * the one that was requested.
     *
     * @note If there are any pending asynchronous requests enqueued in the
     * underlying @ref natpmp_service, @p error will be set to
     * `asio::error::try_again`.
     */
    port_mapping request_mapping(const port_mapping& mapping, error_code& error)
    {
        return this->get_service().request_mapping(
                this->get_implementation(), mapping, error);
    }

    /**
     * @brief Requests a port mapping to be made between this host and the
     * default gateway in an asynchronous fashion.
     *
     * @param mapping Holds the details of the mapping request.
     *
     * @param handler The handler to be called when the public address request
     * operation completes.
     * The function signature of the handler must be:
     * @code void handler(
     *   natpp::error_code, // The result of the operation.
     *   natpp::post_mapping // The port mapping that the NAT box created.
     * ); @endcode
     * Regardless of whether the asynchronous operation completes immediately or
     * not, the handler will not be invoked from within this function. Invocation
     * of the handler will be performed in a manner equivalent to using
     * asio::io_context::post.
     */
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

    /**
     * @brief Requests a port mapping to be removed between this host and the
     * default gateway.
     *
     * @param mapping Holds the details of the mapping request. The
     * `port_mapping::duration` and `port_mapping::public_address` fields will
     * both be set to zero, so the caller code need not do this.
     *
     * @param error Set to indicate what error occurered, if any.
     *
     * @note If there are any pending asynchronous requests enqueued in the
     * underlying @ref natpmp_service, @p error will be set to
     * `asio::error::try_again`.
     */
    void remove_mapping(const port_mapping& mapping, error_code& error)
    {
        return this->get_service().remove_mapping(
                this->get_implementation(), mapping, error);
    }

    /**
     * @brief Requests a port mapping to be removed between this host and the
     * default gateway in an asynchronous fashion.
     *
     * @param mapping Holds the details of the mapping request. The
     * `port_mapping::duration` and `port_mapping::public_address` fields will
     * both be set to zero, so the caller code need not do this.
     *
     * @param handler The handler to be called when the public address request
     * operation completes.
     * The function signature of the handler must be:
     * @code void handler(
     *   natpp::error_code // The result of the operation.
     * ); @endcode
     * Regardless of whether the asynchronous operation completes immediately or
     * not, the handler will not be invoked from within this function. Invocation
     * of the handler will be performed in a manner equivalent to using
     * asio::io_context::post.
     */
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

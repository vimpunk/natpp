#ifndef NATPP_NATPMP_REQUEST_PUBLIC_ADDRESS_OP_HEADER
#define NATPP_NATPMP_REQUEST_PUBLIC_ADDRESS_OP_HEADER

#include <asio/detail/handler_alloc_helpers.hpp>
//#include <asio/detail/handler_invoke_helpers.hpp>
#include <asio/detail/operation.hpp>
#include <asio/detail/addressof.hpp>

namespace nat {
namespace detail {

template<typename Handler>
class natpmp_request_public_address_op : public asio::detail::operation
{
    asio::detail::io_context_impl& ios_impl_;
    Handler handler_;

public:
    ASIO_DEFINE_HANDLER_PTR(natpmp_request_public_address_op);

    natpmp_request_public_address_op(asio::detail::io_context_impl& ios, Handler& handler)
        : operation(&natpmp_request_public_address_op::do_complete)
        , ios_impl_(ios)
        , handler_(std::move(handler)
    {}

    ~natpmp_request_public_address_op();

    static void do_complete(asio::detail::io_context_impl* owner,
        asio::detail::operation* base, error_code& error,
        const size_t num_transferred)
    {
        // Take ownership of the operation object.
        resolve_op* o(static_cast<natpmp_request_public_address_op*>(base));
        ptr p = { asio::detail::addressof(o->handler_), o, o };

        // TODO


        if(owner)
        {
            // fenced block?
            ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, "..."));
            asio::detail::asio_handler_invoke_helpers::invoke(handler, handler.handler_);
            ASIO_HANDLER_INVOCATION_END;
        }
    }
};

} // detail
} // nat

#endif // NATPP_NATPMP_REQUEST_PUBLIC_ADDRESS_OP_HEADER

#include <iostream>

#include "../include/natpp/natpmp.hpp"

#include <asio.hpp>

//namespace nat {
//namespace error {
//const natpmp_error_category& natpmp_category() {
    //static natpmp_error_category instance;
    //return instance;
//}
//}
//}

std::ostream& operator<<(std::ostream& out, const nat::error_code& error)
{
    const std::string msg = error.message();
    out << error.category().name()
        << ": " << msg
        << " (" << error.value() << ")";
    return out;
}

int main()
{
    asio::io_context io;
    nat::natpmp natpmp(io);
    nat::error_code error;

    auto gw_addr = natpmp.gateway_address(error);
    if(error)
        std::cout << "gateway_address error: " << error << '\n';
    else
        std::cout << gw_addr << '\n';

    auto pub_addr = natpmp.public_address(error);
    if(error)
        std::cout << "public_address error: " << error << '\n';
    else
        std::cout << pub_addr << '\n';


    natpmp.async_public_address(
            [](nat::error_code error, asio::ip::address addr)
            {
                std::cout << "async_public_address handler";
                if(error)
                    std::cout << ": " << error << '\n';
                else
                    std::cout << addr << '\n';
            });

    nat::port_mapping mapping;
    mapping.private_port = 55'555;
    mapping.public_port = 55'555;
    mapping.duration = std::chrono::hours(2);
    natpmp.async_request_mapping(mapping,
            [](nat::error_code error, nat::port_mapping mapping)
            {
                std::cout << "async_request_mapping handler";
                if(error)
                    std::cout << ": " << error << '\n';
                else
                    std::cout << '\n';
            });
    natpmp.async_remove_mapping(mapping,
            [](nat::error_code error, nat::port_mapping mapping)
            {
                std::cout << "async_remove_mapping handler";
                if(error)
                    std::cout << ": " << error << '\n';
                else
                    std::cout << '\n';
            });

    io.run();
}

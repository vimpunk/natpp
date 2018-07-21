#include "../include/natpp/natpmp.hpp"

#include <asio.hpp>

int main()
{
    asio::io_context io;
    nat::natpmp natpmp(io);
}

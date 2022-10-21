#include <iostream>
#include <asio.hpp>

#define SERVER_PORT 8080

using std::cout;
using std::cerr;
using std::endl;

using asio::ip::tcp;

class Client
{
public:
    Client(asio::io_context ioc, tcp::socket sock):
        sock_(std::move(sock))
    {}

private:
    tcp::socket sock_;
};


class Server
{
public:
    Server(asio::io_context ioc, short port):
        acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
    {}

    asio::awaitable<void> co_start()
    {
        auto ioc = co_await asio::this_coro::executor;

        while (true)
        {
            auto client_sock = co_await acceptor_.async_accept(asio::use_awaitable);

            auto fd = client_sock.release();
        }
    }

private:
    tcp::acceptor acceptor_;
};

asio::awaitable<void> co_main()
{
    auto ioc = co_await asio::this_coro::executor;

    tcp::acceptor server(ioc, tcp::endpoint(tcp::v4(), SERVER_PORT));

    while (true)
    {
        auto client_sock = co_await server.async_accept(asio::use_awaitable);
    
        asio::co_spawn(ioc,
                        [client_sock]()
                        {
                            co_await handle_client(std::move(client_sock));
                        },
                        asio::detached);
    }
    auto a = server.async_accept(asio::use_awaitable);

    asio::steady_timer timer(ioc);

    while (true)
    {
        timer.expires_from_now(asio::chrono::seconds(1));
        co_await timer.async_wait(asio::use_awaitable);

        cout << "Timer fired" << endl;
    }

    co_return;
}

int main(int argc, char* argv[])
{
    asio::io_context ioc;

    asio::co_spawn(ioc, co_main, asio::detached);

    ioc.run();

    return 0;
}
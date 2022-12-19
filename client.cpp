#include <iostream>
#include <string>

#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/buffer.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>

using std::cout;
using std::cerr;
using std::endl;

using asio::ip::tcp;
using asio::ip::address_v4;
using asio::awaitable;
using asio::use_awaitable;
using asio::co_spawn;
using asio::detached;


#define BUFFER_SIZE 128*1024*1024

unsigned long long bytes_sent =0;
unsigned long long bytes_received =0;

class Client
{
public:
    Client(): _sock(_ioc) {}

    Client(Client&) = delete;
    Client(Client&&) = delete;

    ~Client() {}

    void start(std::string host, short port)
    {
        _sock.connect(tcp::endpoint(address_v4::from_string(host), port));

        co_spawn(_ioc, _sender_loop(), detached);
        co_spawn(_ioc, _receiver_loop(), detached);
        co_spawn(_ioc, _stats(), detached);

        _ioc.run();
    }

private:
    awaitable<void> _sender_loop()
    {
        char buf[BUFFER_SIZE];

        for (;;)
            bytes_sent += co_await _sock.async_write_some(asio::buffer(buf, BUFFER_SIZE), use_awaitable);
    }

    awaitable<void> _receiver_loop()
    {
        char buf[BUFFER_SIZE];

        for (;;)
            bytes_received += co_await _sock.async_read_some(asio::buffer(buf, BUFFER_SIZE), use_awaitable);
    }

    awaitable<void> _stats()
    {
        asio::steady_timer timer(co_await asio::this_coro::executor);

        for (;;)
        {
            timer.expires_from_now(asio::chrono::seconds(1));
            co_await timer.async_wait(use_awaitable);

            cout << "Up: ";

            if (bytes_sent * 8 > 1024*1024*1024)
                cout << ((double)bytes_sent*8)/(1024*1024*1024) << " Gb/s";

            else if (bytes_sent * 8 > 1024*1024)
                cout << ((double)bytes_sent*8)/(1024*1024) << " Mb/s";

            else if (bytes_sent * 8 > 1024)
                cout << ((double)bytes_sent*8)/(1024) << " Kb/s";

            else
                cout << ((double)bytes_sent*8) << " b/s";

            cout << " | ";
            cout << "Down: ";

            if (bytes_received * 8 > 1024*1024*1024)
                cout << ((double)bytes_received*8)/(1024*1024*1024) << " Gb/s";

            else if (bytes_received * 8 > 1024*1024)
                cout << ((double)bytes_received*8)/(1024*1024) << " Mb/s";

            else if (bytes_received * 8 > 1024)
                cout << ((double)bytes_received*8)/(1024) << " Kb/s";

            else
                cout << ((double)bytes_received*8) << " b/s";

            cout << endl;

            bytes_sent =0;
            bytes_received =0; 
        }
    }

private:
    asio::io_context _ioc;
    tcp::socket _sock;
};


int main(int argc, char* argv[])
{
    Client cli;

    cli.start("127.0.0.1", 1337);

    return 0;
}
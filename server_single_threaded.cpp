#include <iostream>

#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/buffer.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>

#define BUFFER_SIZE 1024*1024

using std::cout;
using std::cerr;
using std::endl;

using asio::ip::tcp;
using asio::buffer;
using asio::awaitable;
using asio::use_awaitable;
using asio::co_spawn;
using asio::detached;

unsigned long long bytes_transfered =0;

class Client
{
public:
    Client(asio::io_context& ioc, tcp::socket sock):
        _ioc(ioc),
        _sock(std::move(sock))
    {}

    Client(Client&) = delete;
    Client(Client&&) = delete; 

    ~Client() {}

    void start()
    {
        co_spawn(_ioc, _echo_loop(), detached);
    }

private:
    awaitable<void> _echo_loop()
    {
        char buf[BUFFER_SIZE];

        for (;;)
        {
            auto bytes_received = co_await _sock.async_read_some(buffer(buf, BUFFER_SIZE), use_awaitable);
            co_await _write_all((void*)buf, bytes_received);
            bytes_transfered += bytes_received;
        }
    }

    awaitable<void> _write_all(void* buf, unsigned long length)
    {
        auto bytes_sent = co_await _sock.async_write_some(buffer(buf, length), use_awaitable);

        if (bytes_sent < length)
            co_await _write_all((uint8_t*)buf + bytes_sent, length - bytes_sent);
    }

private:
    asio::io_context& _ioc;
    tcp::socket _sock;
};


class Server
{
public:
    Server(short port):
        _acceptor(_ioc, tcp::endpoint(tcp::v4(), port))
    {}

    Server(Server&) = delete;
    Server(Server&&) = delete;

    ~Server() {}

    void start()
    {
        co_spawn(_ioc, _listen(), detached);
        co_spawn(_ioc, _stats(), detached);

        _ioc.run();
    }

    void stop() {}

public:
    awaitable<void> _listen()
    {
        for (;;)
        {
            auto client_sock = co_await _acceptor.async_accept(use_awaitable);
        
            cout << "New connection from " << client_sock.remote_endpoint() << endl;

            auto client = new Client(_ioc, std::move(client_sock));
            client->start();
        }
    }

    awaitable<void> _stats()
    {
        asio::steady_timer timer(co_await asio::this_coro::executor);

        for (;;)
        {
            timer.expires_from_now(asio::chrono::seconds(1));
            co_await timer.async_wait(use_awaitable);
        
            cout << "Bytes_transferred: " << bytes_transfered << endl;
            bytes_transfered =0;
        }
    }

private:
    asio::io_context _ioc;
    tcp::acceptor _acceptor;
};


int main(int argc, char* argv[])
{
    Server server(1337);

    server.start();

    return 0;
}
#pragma once

#include <iostream>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/co_spawn.hpp>

using std::cout;
using std::cerr;
using std::endl;

using asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;
using asio::co_spawn;

class Server
{
public:
    Server(short port, std::shared_ptr<asio::io_context> ioc):
        _ioc(ioc),
        _acceptor(_ioc->get_executor(), tcp::endpoint(tcp::v4(), port))
    {
        cout << "[SERVER] Created." << endl;
    }

    Server(short port): Server(port, std::make_shared<asio::io_context>()) {}

    Server(Server&) = delete;
    Server(Server&&) = delete;

    ~Server()
    {
        cout << "[SERVER] Destroyed." << endl;
    }

    void start()
    {
        cout << "[SERVER] Start listening on " << _acceptor.local_endpoint() << endl;

        co_spawn(_ioc->get_executor(),
                 _listen(),
                 [this](std::exception_ptr exp)
                 {
                    try
                    {
                        if (exp)
                            std::rethrow_exception(exp);

                    }catch (const std::exception& e)
                    {
                        cerr << "[SERVER] Exception occured while listening: " << e.what() << endl;
                    }

                    this->stop();
                 });

        _ioc->run();
    }

    void stop()
    {
        cout << "[SERVER] Stopping event loop" << endl;
        _ioc->stop();
    }

    virtual void on_new_connection(tcp::socket socket);

    auto get_io_context() { return _ioc; }

public:
    awaitable<void> _listen()
    {
        for (;;)
        {
            auto accepted_sock = co_await _acceptor.async_accept(use_awaitable);

            on_new_connection(std::move(accepted_sock));
        }
    }

private:
    std::shared_ptr<asio::io_context> _ioc;
    tcp::acceptor _acceptor;
};
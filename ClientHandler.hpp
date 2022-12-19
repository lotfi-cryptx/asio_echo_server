#pragma once

#include <iostream>
#include <memory>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/buffer.hpp>
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

#define BUFFER_SIZE 1024


class ClientHandler: public std::enable_shared_from_this<ClientHandler>
{
public:

    auto get_shared_ptr() { return shared_from_this(); }

    static auto create(std::shared_ptr<asio::io_context> ioc, tcp::socket sock)
    {
        return std::shared_ptr<ClientHandler>(new ClientHandler(ioc, std::move(sock)));
    }

    ~ClientHandler()
    {
        _sock.close();
        cout << "[" << _ep << "] Connection closed." << endl;
    }

    void start() { _spawn_loop(); }


private:
    ClientHandler(std::shared_ptr<asio::io_context> ioc, tcp::socket sock):
        _ioc(ioc),
        _sock(_ioc->get_executor(), tcp::v4(), sock.release()),
        _ep(_sock.remote_endpoint())
    {
        cout << "[" << _ep << "] Created connection" << endl;
    }

    ClientHandler(ClientHandler&) = delete;
    ClientHandler(ClientHandler&&) = delete;


private:
    void _spawn_loop()
    {
        auto self = get_shared_ptr();
 
        co_spawn(_ioc->get_executor(),
                 _echo_loop(),
                 [self, this] (std::exception_ptr exp)
                 {
                    try
                    {
                        if (exp)
                            std::rethrow_exception(exp);

                    }
                    catch (const asio::system_error& e)
                    {
                        if (e.code() == asio::error::eof)
                            return;
                        
                        if (e.code() == asio::error::connection_reset)
                            return;

                        cerr << "[" << this->_ep << "]";
                        cerr << " Connection error: ";
                        cerr << e.what() << endl;

                    }
                    catch(const std::exception& e)
                    {
                        cerr << "[" << this->_ep << "]";
                        cerr << " Caught exception: ";
                        cerr << e.what() << endl;
                    }
                 });
    }

    awaitable<void> _echo_loop()
    {
        char buf[BUFFER_SIZE];

        for (;;)
        {
            auto rx_buf = asio::buffer(buf, BUFFER_SIZE);
            auto bytes_received = co_await _sock.async_read_some(rx_buf, use_awaitable);

            co_await _write_all((void*)buf, bytes_received);
            
            _total_bytes_transfered += bytes_received;
        }
    }

    awaitable<void> _write_all(void* buf, unsigned long length)
    {
        auto tx_buf = asio::buffer(buf, length);
        auto bytes_sent = co_await _sock.async_write_some(tx_buf, use_awaitable);

        if (bytes_sent < length)
            co_await _write_all((uint8_t*)buf + bytes_sent, length - bytes_sent);
    }


private:
    std::shared_ptr<asio::io_context> _ioc;
    tcp::socket _sock;
    tcp::endpoint _ep;
    unsigned long long _total_bytes_transfered =0;
};

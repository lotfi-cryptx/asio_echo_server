#include <iostream>

#include <asio/io_context.hpp>
#include <asio.hpp>
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

class Client: public std::enable_shared_from_this<Client>
{
public:

    static std::shared_ptr<Client> create(asio::io_context& ioc, tcp::socket sock)
    {
        return std::shared_ptr<Client>(new Client(ioc, std::move(sock)));
    }

    ~Client()
    {
        cout << "[" << _ep << "]";
        cout << " Connection closed" << endl;
    }

    void start()
    {
        auto self(shared_from_this());

        co_spawn(_ioc,
                 _echo_loop(),
                 [self, this] (std::exception_ptr exp)
                 {
                    this->_sock.close();
                    
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


private:
    Client(asio::io_context& ioc, tcp::socket sock):
        _ioc(ioc),
        _sock(std::move(sock)),
        _ep(_sock.remote_endpoint())
    {
        cout << "[" << _ep << "]";
        cout << " Connection created" << endl;
    }

    Client(Client&) = delete;
    Client(Client&&) = delete;


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
    tcp::endpoint _ep;
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
        co_spawn(_ioc,
                 _listen(),
                 [this](std::exception_ptr exp)
                 {
                    try
                    {
                        if (exp)
                            std::rethrow_exception(exp);

                    }catch (const std::exception& e)
                    {
                        cerr << "Caught exception on _listen: " << e.what() << endl;
                    }

                    this->stop();
                 });

        co_spawn(_ioc,
                 _stats(),
                 [this](std::exception_ptr exp)
                 {
                    try
                    {
                        if (exp)
                            std::rethrow_exception(exp);

                    }catch (const std::exception& e)
                    {
                        cerr << "Caught exception on _stats: " << e.what() << endl;
                    }

                    this->stop();
                 });

        _ioc.run();
    }

    void stop()
    {
        cout << "Stopping server's event loop" << endl;
        _ioc.stop();
    }


public:
    awaitable<void> _listen()
    {
        cout << "Started listening on " << _acceptor.local_endpoint() << endl;

        for (;;)
        {
            auto client_sock = co_await _acceptor.async_accept(use_awaitable);

            Client::create(_ioc, std::move(client_sock))->start();
        }
    }

    awaitable<void> _stats()
    {
        asio::steady_timer timer(co_await asio::this_coro::executor);

        for (;;)
        {
            timer.expires_from_now(asio::chrono::seconds(1));
            co_await timer.async_wait(use_awaitable);

            cout << "Throughput: ";

            if (bytes_transfered * 8 > 1024*1024*1024)
                cout << ((double)bytes_transfered*8)/(1024*1024*1024) << " Gb/s";

            else if (bytes_transfered * 8 > 1024*1024)
                cout << ((double)bytes_transfered*8)/(1024*1024) << " Mb/s";

            else if (bytes_transfered * 8 > 1024)
                cout << ((double)bytes_transfered*8)/(1024) << " Kb/s";

            else
                cout << ((double)bytes_transfered*8) << " b/s";

            cout << endl;

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
#include <iostream>
#include <thread>

#include <asio/ip/tcp.hpp>

#include "Server.hpp"
#include "WorkerThread.hpp"
#include "ClientHandler.hpp"

using asio::ip::tcp;

using std::cout;
using std::cerr;
using std::endl;

int attached_cpu =0;

void Server::on_new_connection(tcp::socket sock)
{
    attached_cpu = attached_cpu % std::thread::hardware_concurrency();
    WorkerThread::create(attached_cpu)->assign_connection(std::move(sock));
    attached_cpu++;
}


int main(int argc, char* argv[])
{
    Server server(1337);

    server.start();

    return 0;
}
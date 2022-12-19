#include <iostream>

#include "Server.hpp"
#include "WorkerThread.hpp"

using std::cout;
using std::cerr;
using std::endl;

auto thread = WorkerThread::create();

void Server::on_new_connection(tcp::socket sock)
{
    thread->assign_connection(std::move(sock));
}



int main(int argc, char* argv[])
{
    Server server(1337);

    server.start();

    return 0;
}
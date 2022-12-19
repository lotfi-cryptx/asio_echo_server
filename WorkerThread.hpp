#pragma once

#include <iostream>
#include <memory>
#include <thread>
#include <pthread.h>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "ClientHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;

using asio::ip::tcp;


class WorkerThread: public std::enable_shared_from_this<WorkerThread>
{
    static int last_id;
public:
    auto get_shared_ptr() { return shared_from_this(); }

    static auto create(int attached_cpu =-1)
    {
        return std::shared_ptr<WorkerThread>(new WorkerThread(attached_cpu));
    }

    ~WorkerThread()
    {
        cout << "[Thread " << _id << "] Destroyed" << endl;
    }

    void assign_connection(tcp::socket sock)
    {
        ClientHandler::create(_ioc, std::move(sock))->start();
        _run();
    }

private:
    WorkerThread(int attached_cpu):
        _id(WorkerThread::last_id++),
        _running(false),
        _attached_cpu(attached_cpu),
        _ioc(std::make_shared<asio::io_context>())
    {
        cout << "[Thread " << _id << "] Created" << endl;
    }

    WorkerThread(WorkerThread&) = delete;
    WorkerThread(WorkerThread&&) = delete;


    void _run()
    {
        auto self = get_shared_ptr();

        if (!_running)
        {
            std::thread([self]()
                        {
                            if (self->_attached_cpu >= 0)
                            {
                                cpu_set_t cpu_set;
                                CPU_ZERO(&cpu_set);
                                CPU_SET(self->_attached_cpu, &cpu_set);

                                pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
                            }

                            cout << "[Thread " << self->_id << "] Started" << endl;

                            self->_running = true;

                            if (self->_ioc->stopped())
                                self->_ioc->restart();

                            self->_ioc->run();

                            self->_running = false;
                            
                            cout << "[Thread " << self->_id << "] Stopped" << endl;
                        }).detach();
        }

        
    }    

private:
    int _id;
    bool _running;
    int _attached_cpu;
    std::shared_ptr<asio::io_context> _ioc;
};

int WorkerThread::last_id = 1;
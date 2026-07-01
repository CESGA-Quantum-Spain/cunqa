#include <atomic>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <boost/asio.hpp>

#include "comm/server.hpp"
#include "utils/helpers/net_functions.hpp"

#include "logger.hpp"

namespace as = boost::asio;
using namespace std::string_literals;
using as::ip::tcp;

namespace cunqa {
namespace comm {

struct Server::Impl {
    as::io_context io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;

    // Self-pipe used to interrupt a thread blocked in wait_for_request(), the
    // POSIX analogue of the ZMQ inproc signal socket. wake_pipe_[0] is the read
    // end (polled by the I/O thread), wake_pipe_[1] the write end.
    int wake_pipe_[2] = {-1, -1};
    std::mutex signal_mutex_;
    std::atomic<bool> running_{true};

    std::string asio_endpoint;

    Impl(const std::string& ip) :
        acceptor_{io_context_, tcp::endpoint{as::ip::address::from_string(ip), 0}},
        socket_{acceptor_.get_executor()}
    {
        auto ep = acceptor_.local_endpoint();
        asio_endpoint = ip + ":" + std::to_string(ep.port());

        if (::pipe(wake_pipe_) != 0)
            throw std::runtime_error("Unable to create the interrupt pipe: "s + std::strerror(errno));
        set_nonblocking(wake_pipe_[0]);
        set_nonblocking(wake_pipe_[1]);

        accept();
    }

    ~Impl()
    {
        close();
        if (wake_pipe_[0] != -1) ::close(wake_pipe_[0]);
        if (wake_pipe_[1] != -1) ::close(wake_pipe_[1]);
    }

    void accept()
    {
        socket_ = tcp::socket(acceptor_.get_executor());
        acceptor_.accept(socket_);
    }

    // Block until the socket has incoming data (returns true) or interrupt() is
    // called (returns false). Waits on both the client socket and the wake pipe
    // so the owning thread can leave to send pending results.
    bool wait_for_request()
    {
        while (running_) {
            struct pollfd fds[2];
            fds[0].fd = socket_.native_handle();
            fds[0].events = POLLIN;
            fds[0].revents = 0;
            fds[1].fd = wake_pipe_[0];
            fds[1].events = POLLIN;
            fds[1].revents = 0;

            int rc = ::poll(fds, 2, -1);
            if (rc < 0) {
                if (errno == EINTR) continue;
                LOGGER_ERROR("Error polling sockets: {}", std::strerror(errno));
                continue;
            }

            if (fds[1].revents & POLLIN) {
                drain_pipe();
                return false;
            }
            // POLLHUP/POLLERR too: recv() will observe the disconnect and re-accept.
            if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
                return true;
            }
        }
        return false;
    }

    // Wake a thread blocked in wait_for_request(). Thread-safe: the write end is
    // only ever touched here, under signal_mutex_.
    void interrupt()
    {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        char byte = 1;
        ssize_t n = ::write(wake_pipe_[1], &byte, 1);
        (void)n;   // EAGAIN just means a wake is already pending; that's enough.
    }

    // Receive one request. Call only after wait_for_request() returned true.
    std::string recv()
    {
        try {
            uint32_t data_length_network;
            as::read(socket_, as::buffer(&data_length_network, sizeof(data_length_network)));

            uint32_t data_length = ntohl(data_length_network);
            std::string data(data_length, '\0');
            as::read(socket_, as::buffer(&data[0], data_length));
            return data;
        } catch (const boost::system::system_error& e) {
            if (e.code() == as::error::eof) {
                LOGGER_DEBUG("Client disconnected gracefully.");
                socket_.close();
                accept();
            } else if (e.code() == as::error::connection_reset) {
                LOGGER_ERROR("Client connection reset (forcible close).");
                socket_.close();
                accept();
            } else {
                LOGGER_ERROR("Error receiving the circuit.");
                throw;
            }
        }

        return std::string();
    }

    void send(const std::string& result)
    {
        try {
            auto data_length = legacy_size_cast<uint32_t, std::size_t>(result.size());
            auto data_length_network = htonl(data_length);

            as::write(socket_, as::buffer(&data_length_network, sizeof(data_length_network)));
            as::write(socket_, as::buffer(result));
        } catch (const boost::system::system_error& e) {
            LOGGER_ERROR("Error sending the result.");
            throw;
        }
    }

    void close()
    {
        // Stop wait_for_request() looping and wake it so the owner can leave.
        if (running_.exchange(false)) {
            interrupt();
            boost::system::error_code ec;
            socket_.close(ec);
        }
    }

private:
    static void set_nonblocking(int fd)
    {
        int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags != -1)
            ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    void drain_pipe()
    {
        char buf[256];
        while (::read(wake_pipe_[0], buf, sizeof(buf)) > 0) { }
    }
};

Server::Server(const std::string& mode) :
    mode{mode},
    nodename{get_nodename()},
    device(get_device()),
    pimpl_{std::make_unique<Impl>(mode == "hpc" ? "127.0.0.1" : get_IP_address())}
{
    endpoint = pimpl_->asio_endpoint;
}

Server::~Server() = default;

bool Server::wait_for_request()
{
    return pimpl_->wait_for_request();
}

void Server::interrupt()
{
    pimpl_->interrupt();
}

std::string Server::recv_data()
{
    return pimpl_->recv();
}

void Server::send_result(const std::string& result)
{
    try {
        pimpl_->send(result);
    } catch (const std::exception& e) {
        throw ServerException(e.what());
    }
}

void Server::close()
{
    pimpl_->close();
}

} // End of comm namespace
} // End of cunqa namespace

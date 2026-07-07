#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>

#include "zmq.hpp"

#include "comm/server.hpp"
#include "logger.hpp"
#include "utils/helpers/net_functions.hpp"

namespace cunqa {
namespace comm {

// Generic inproc endpoint used to interrupt a thread blocked in
// wait_for_request(). It is scoped to this Server's own ZMQ context, so the
// fixed name never collides across Server instances.
static constexpr const char* SIGNAL_ENDPOINT = "inproc://internal-signal";

struct Server::Impl {
    zmq::context_t context_;
    zmq::socket_t socket_;          // ROUTER: owned by the caller's I/O thread
    zmq::socket_t signal_recv_;     // PAIR:   read only by the polling thread
    zmq::socket_t signal_send_;     // PAIR:   written under signal_mutex_

    std::queue<std::string> rid_queue_;   // routing ids captured on recv

    std::mutex signal_mutex_;
    std::atomic<bool> running_{true};

    std::string zmq_endpoint;

    Impl(const std::string& mode) :
        socket_{context_, zmq::socket_type::router},
        signal_recv_{context_, zmq::socket_type::pair},
        signal_send_{context_, zmq::socket_type::pair}
    {
        try {
            std::string ip = (mode == "hpc" ? "127.0.0.1"s : get_IP_address());
            socket_.bind("tcp://" + ip + ":*");

            char endpoint[256];
            size_t sz = sizeof(endpoint);
            zmq_getsockopt(socket_, ZMQ_LAST_ENDPOINT, endpoint, &sz);
            zmq_endpoint = std::string(endpoint);
            LOGGER_DEBUG("Server bound to {}", endpoint);

            // inproc signalling pair: the bind side must exist before connect.
            signal_recv_.bind(SIGNAL_ENDPOINT);
            signal_send_.connect(SIGNAL_ENDPOINT);
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error binding to endpoint: {}", e.what());
            throw;
        }
    }

    ~Impl()
    {
        close();
    }

    // Block until the socket has an incoming request (returns true) or
    // interrupt() is called (returns false). Waits on both the socket and the
    // inproc signal so the owning thread can leave to send pending results.
    bool wait_for_request()
    {
        zmq::pollitem_t items[] = {
            {socket_.handle(),      0, ZMQ_POLLIN, 0},
            {signal_recv_.handle(), 0, ZMQ_POLLIN, 0},
        };

        while (running_) {
            try {
                zmq::poll(&items[0], 2, std::chrono::milliseconds{-1});
            } catch (const zmq::error_t& e) {
                if (!running_) break;
                LOGGER_ERROR("Error polling sockets: {}", e.what());
                continue;
            }

            if (items[1].revents & ZMQ_POLLIN) {
                drain_signals();
                return false;
            }
            if (items[0].revents & ZMQ_POLLIN) {
                return true;
            }
        }
        return false;
    }

    // Wake a thread blocked in wait_for_request(). Thread-safe: signal_send_ is
    // only ever touched here, under signal_mutex_.
    void interrupt()
    {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        try {
            zmq::message_t byte(std::size_t{1});
            signal_send_.send(byte, zmq::send_flags::dontwait);
        } catch (const zmq::error_t&) {
            // EAGAIN just means a wake is already pending; that's enough.
        }
    }

    // Receive one request. Call only after wait_for_request() returned true.
    std::string recv()
    {
        try {
            zmq::message_t identity;
            auto id_size = socket_.recv(identity, zmq::recv_flags::none);
            std::string id_data(static_cast<char*>(identity.data()), id_size.value());
            rid_queue_.push(id_data);

            zmq::message_t message;
            auto size = socket_.recv(message, zmq::recv_flags::none);
            std::string data(static_cast<char*>(message.data()), size.value());
            return data;
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error receiving data: {}", e.what());
            return std::string("CLOSE");
        }
    }

    // Send one result back to the client that issued the matching request.
    void send(const std::string& result)
    {
        try {
            std::string recvr_id = rid_queue_.front();
            rid_queue_.pop();
            zmq::message_t identity_frame(recvr_id.begin(), recvr_id.end());
            zmq::message_t message(result.begin(), result.end());

            socket_.send(identity_frame, zmq::send_flags::sndmore);
            socket_.send(message, zmq::send_flags::none);
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error sending result: {}", e.what());
            throw;
        }
    }

    void close()
    {
        // Stop wait_for_request() looping and wake it so the owner can leave.
        if (running_.exchange(false))
            interrupt();
    }

private:
    void drain_signals()
    {
        zmq::message_t tmp;
        while (signal_recv_.recv(tmp, zmq::recv_flags::dontwait)) { }
    }
};

Server::Server(const std::string& mode) :
    mode{mode},
    nodename{get_nodename()},
    device(get_device()),
    pimpl_{std::make_unique<Impl>(mode)}
{
    endpoint = pimpl_->zmq_endpoint;
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

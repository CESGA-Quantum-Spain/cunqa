
#include <string>
#include <queue>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include "zmq.hpp"

#include "classical_channel/classical_channel.hpp"
#include "utils/helpers/net_functions.hpp"

#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace comm {

struct ClassicalChannel::Impl
{
    std::string zmq_endpoint;
    std::string zmq_id;

    zmq::context_t zmq_context;
    std::unordered_map<std::string, zmq::socket_t> zmq_sockets;
    zmq::socket_t zmq_comm_server;
    std::unordered_map<std::string, std::queue<std::string>> message_queue;

    Impl(const std::string& id)
    {
        //Endpoint part
        auto IP = get_IP_address();
        zmq_endpoint = "tcp://" + IP + ":*";
        zmq_id = id == "" ? zmq_endpoint : id;

        //Server part
        zmq::socket_t qpu_server_socket_(zmq_context, zmq::socket_type::router);
        qpu_server_socket_.bind(zmq_endpoint);
        
        char endpoint[256];
        size_t sz = sizeof(endpoint);
        zmq_getsockopt(qpu_server_socket_, ZMQ_LAST_ENDPOINT, endpoint, &sz);
        zmq_endpoint = std::string(endpoint);

        zmq_comm_server = std::move(qpu_server_socket_);
    }

    ~Impl() = default;


    void connect(const std::string& endpoint, const std::string& id = "")
    {   
        auto client_id = id == "" ? endpoint : id; 
        if (zmq_sockets.find(client_id) == zmq_sockets.end()) {
            zmq::socket_t tmp_client_socket(zmq_context, zmq::socket_type::dealer);
            tmp_client_socket.setsockopt(ZMQ_IDENTITY, zmq_id.c_str(), zmq_id.size());
            zmq_sockets[client_id] = std::move(tmp_client_socket);
            zmq_sockets[client_id].connect(endpoint);
        }
    }

    void connect(const std::string& endpoint, const bool force_endpoint)
    {   
        if (zmq_sockets.find(endpoint) == zmq_sockets.end()) {
            zmq::socket_t tmp_client_socket(zmq_context, zmq::socket_type::dealer);
            std::string connexion_id = force_endpoint ? zmq_endpoint : zmq_id;
            tmp_client_socket.setsockopt(ZMQ_IDENTITY, connexion_id.c_str(), connexion_id.size());
            zmq_sockets[endpoint] = std::move(tmp_client_socket);
            zmq_sockets[endpoint].connect(endpoint);
        }
    }

    void send(const std::string& data, const std::string& target) 
    {
        if (zmq_sockets.find(target) == zmq_sockets.end()) {
            LOGGER_ERROR("No connections were established with endpoint {} trying to send. {}", target, data);
            throw std::runtime_error("Error with endpoint connection.");
        }
        zmq::message_t message(data.begin(), data.end());
        zmq_sockets[target].send(message, zmq::send_flags::none);
        
    }
    
    std::string recv(const std::string& origin)
    {
        if (!message_queue[origin].empty()) {
            std::string stored_data = message_queue[origin].front();
            message_queue[origin].pop();
            return stored_data;
        } else {
            while (true) {
                zmq::message_t id;
                zmq::message_t message;
                
                [[maybe_unused]] auto ret1 = zmq_comm_server.recv(id, zmq::recv_flags::none);
                [[maybe_unused]] auto ret2 = zmq_comm_server.recv(message, zmq::recv_flags::none);
                std::string id_str(static_cast<char*>(id.data()), id.size());
                std::string data(static_cast<char*>(message.data()), message.size());

                if (id_str == origin) {
                    return data;
                } else {
                    message_queue[id_str].push(data);
                }
            }
        }
    }
};


ClassicalChannel::ClassicalChannel() : pimpl_{std::make_unique<Impl>("")} 
{ 
    endpoint = pimpl_->zmq_endpoint;
}

ClassicalChannel::ClassicalChannel(const std::string& id) : pimpl_{std::make_unique<Impl>(id)} 
{ 
    endpoint = pimpl_->zmq_endpoint;
}

ClassicalChannel::~ClassicalChannel() = default;

//-------------------------------------------------
// Publish the endpoint for other processes to read
//-------------------------------------------------
void ClassicalChannel::publish(const std::string& suffix) 
{
    const std::string store = getenv("STORE");
    const std::string filepath = store + "/.cunqa/communications.json"s;
    JSON communications_endpoint = 
    {
        {"communications_endpoint", endpoint}
    };
    write_on_file(communications_endpoint, filepath, suffix);
}


//--------------------------------------------------
// Functions to stablish the other devices connected
//--------------------------------------------------
void ClassicalChannel::connect(const std::string& endpoint, const std::string& id) 
{
    pimpl_->connect(endpoint, id);
}

void ClassicalChannel::connect(const std::string& endpoint, const bool force_endpoint)
{
    pimpl_->connect(endpoint, force_endpoint);
}

// No id in this overload because is thought for classical communications,
// which do not care for ids, its ok for them to use only the endpoints
void ClassicalChannel::connect(const std::vector<std::string>& endpoints, const bool force_endpoint) 
{
    for (const auto& endpoint : endpoints) {
        pimpl_->connect(endpoint, force_endpoint);
    }
}

//------------------------------------------------------------------------------------
// Send and recv functions for arbitrary info (such as a whole circuit or an endpoint)
//------------------------------------------------------------------------------------
void ClassicalChannel::send_info(const std::string& data, const std::string& target) { pimpl_->send(data, target); }
std::string ClassicalChannel::recv_info(const std::string& origin) { return pimpl_->recv(origin); }

//-----------------------------------------
// Send and recv functions for measurements
//-----------------------------------------
void ClassicalChannel::send_measure(const int& measurement, const std::string& target) { pimpl_->send(std::to_string(measurement), target); }
int ClassicalChannel::recv_measure(const std::string& origin) { return std::stoi(pimpl_->recv(origin)); }


} // End of comm namespace
} // End of cunqa namespace

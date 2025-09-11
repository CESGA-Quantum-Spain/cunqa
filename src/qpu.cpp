#include <string>
#include <iostream>

#include "qpu.hpp"
#include "logger.hpp"

using namespace std::string_literals;

namespace {
    // TODO: not having this hardcoded here is an improvement for other supercomputing centers
    const auto store = getenv("STORE");
    const std::string filepath = store + "/.cunqa/qpus.json"s;
}

namespace cunqa {

QPU::QPU(std::unique_ptr<sim::Backend> backend, const std::string& mode, const std::string& family) :
    backend{std::move(backend)},
    server{std::make_unique<comm::Server>(mode)},
    family_{family}
{ }

void QPU::turn_ON() 
{
    std::thread listen([this](){this->recv_data_();});
    std::thread compute([this](){this->compute_result_();});

    JSON qpu_config = *this;
    write_on_file(qpu_config, filepath);

    listen.join();
    compute.join();
}

void QPU::compute_result_()
{    
    QuantumTask quantum_task_; 
    while (true) 
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_condition_.wait(lock, [this] { return !message_queue_.empty(); });

        while (!message_queue_.empty()) 
        {
            try {
                std::string message = message_queue_.front();
                message_queue_.pop();
                lock.unlock();
                
                quantum_task_.update_circuit(message);
                auto result = backend->execute(quantum_task_);
                server->send_result(result.dump());

            } catch(const comm::ServerException& e) {
                LOGGER_ERROR("There has happened an error sending the result, probably the client has had an error.");
                LOGGER_ERROR("Message of the error: {}", e.what());
            } catch(const std::exception& e) {
                LOGGER_ERROR("There has happened an error sending the result, the server keeps on iterating.");
                LOGGER_ERROR("Message of the error: {}", e.what());
                server->send_result("{\"ERROR\":\""s + std::string(e.what()) + "\"}"s);
            }
            lock.lock();
        }
    }
}

void QPU::recv_data_() 
{   
    while (true) {
        try {
            auto message = server->recv_data();
                {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (message.compare("CLOSE"s) == 0) {
                    server->accept();
                    continue;
                }
                else
                    message_queue_.push(message);
            }
            queue_condition_.notify_one();
        } catch (const std::exception& e) {
            LOGGER_INFO("There has happened an error receiving the circuit, the server keeps on iterating.");
            LOGGER_ERROR("Official message of the error: {}", e.what());
        }
    }
}


} // End of cunqa namespace


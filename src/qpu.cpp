#include <string>
#include <iostream>

#include "qpu.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

using namespace std::string_literals;

namespace cunqa {

QPU::QPU(std::unique_ptr<sim::Backend> backend, const std::string& mode, 
         const std::string& name, const std::string& family) :
    backend{std::move(backend)},
    server{std::make_unique<comm::Server>(mode)},
    name_{name},
    family_{family}
{ }

void QPU::turn_ON()
{
    std::thread io([this](){this->recv_data_();});
    std::thread compute([this](){this->compute_result_();});

    JSON qpu_config = *this;
    write_on_file(qpu_config, QPUS_FILEPATH, name_);

    io.join();
    compute.join();
}

void QPU::compute_result_()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_condition_.wait(lock, [this] { return !message_queue_.empty(); });

        while (!message_queue_.empty())
        {
            auto quantum_task_str = message_queue_.front();
            message_queue_.pop();
            lock.unlock();

            std::string result;
            try {
                result = backend->execute(quantum_task_str).dump();
            } catch(const std::exception& e) {
                LOGGER_ERROR("There has happened an error executing the circuit, the server keeps on iterating.");
                LOGGER_ERROR("Message of the error: {}", e.what());

                JSON error_result = {{"ERROR", std::string(e.what())}};
                try {
                    const auto quantum_task = JSON::parse(quantum_task_str);
                    if (quantum_task.contains("id"))
                        error_result["id"] = quantum_task.at("id");
                } catch (const std::exception& parsing_error) {
                    LOGGER_ERROR("The id of the failed circuit could not be read: {}", parsing_error.what());
                }

                result = error_result.dump();
            }

            {
                std::lock_guard<std::mutex> result_lock(result_mutex_);
                result_queue_.push(std::move(result));
            }
            // Hand the result to the I/O thread, which owns the socket, and
            // wake it so it sends without waiting for the next request.
            server->interrupt();

            lock.lock();
        }
    }
}

void QPU::recv_data_()
{
    while (true) {
        try {
            if (server->wait_for_request()) {
                auto message = server->recv_data();
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    message_queue_.push(message);
                }
                queue_condition_.notify_one();
            }
            send_ready_results_();
        } catch (const std::exception& e) {
            LOGGER_INFO("There has happened an error in the I/O loop, the server keeps on iterating.");
            LOGGER_ERROR("Official message of the error: {}", e.what());
            throw;
        }
    }
}

void QPU::send_ready_results_()
{
    std::queue<std::string> ready;
    {
        std::lock_guard<std::mutex> lock(result_mutex_);
        std::swap(ready, result_queue_);
    }

    while (!ready.empty()) {
        try {
            server->send_result(ready.front());
        } catch (const comm::ServerException& e) {
            LOGGER_ERROR("There has happened an error sending the result, probably the client has had an error.");
            LOGGER_ERROR("Message of the error: {}", e.what());
        }
        ready.pop();
    }
}


} // End of cunqa namespace


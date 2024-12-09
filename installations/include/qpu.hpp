#include <thread>
#include <queue>
#include <atomic>
#include "comm/server.hpp"
#include "backend.hpp"
#include "simulators/simulator.hpp"
#include "config/qpu_config.hpp"
#include "utils/custom_json.hpp"

using json = nlohmann::json;
using namespace std::string_literals;
using namespace config::net;

template <SimType sim_type = SimType::Aer>
class QPU {
public:
    config::QPUConfig<sim_type> qpu_config;
    Backend<sim_type> backend;

    QPU(config::QPUConfig<sim_type> qpu_config);

    void turn_ON();
    inline void turn_OFF();

private:
    std::unique_ptr<Server> server;
    std::queue<std::string> _message_queue;
    std::condition_variable _queue_condition;
    std::mutex _queue_mutex;
    std::atomic<bool> _running;

    void _compute_result();
    void _recv_data();
};

template <SimType sim_type>
QPU<sim_type>::QPU(config::QPUConfig<sim_type> qpu_config) : 
    qpu_config{qpu_config}
{
    CustomJson c_json{};
    json config_json(qpu_config.net_config);
    c_json.write(config_json, qpu_config.filepath);
}


template <SimType sim_type>
void QPU<sim_type>::turn_ON() {
    server = std::make_unique<Server>(qpu_config.net_config);
    _running = true;

    std::thread listen([this](){this->_recv_data();});
    std::thread compute([this](){this->_compute_result();});
    
    listen.join();
    compute.join();

    std::cout << "Main: Program terminated.\n";
}

template <SimType sim_type>
void QPU<sim_type>::turn_OFF(){
    std::string response{"Cerrando conexión..."};
    server->send_result(response);
    _running = false;
    _queue_condition.notify_all();
}

template <SimType sim_type>
void QPU<sim_type>::_compute_result()
{
    while (_running) 
    {
        std::unique_lock<std::mutex> lock(_queue_mutex);

        _queue_condition.wait(lock, [this] { return !_message_queue.empty() || !_running; });

        while (!_message_queue.empty() && _running) 
        {
            std::string message = _message_queue.front();
            _message_queue.pop();
            lock.unlock();

            //Computacion
            json kernel = json::parse(message);
            json response = backend.run(kernel, config::run::RunConfig(kernel.at("config")));
            server->send_result(to_string(response));

            lock.lock();
        }
    }
}

template <SimType sim_type>
void QPU<sim_type>::_recv_data() 
{
    while (_running) 
    {
        auto message = server->recv_data();
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            if (message.compare("CLOSE"s) == 0) {
                turn_OFF();
            }
            else
                _message_queue.push(message);
        }
        _queue_condition.notify_one(); // Notificar al otro hilo
    }
}
#include <string>
#include <mpi.h>

#include "utils/helpers/net_functions.hpp"
#include "classical_channel.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {
namespace comm {

struct ClassicalChannel::Impl
{
    int mpi_size;
    int mpi_rank;

    Impl()
    {
        MPI_Init(NULL, NULL);
        MPI_Comm_size(MPI_COMM_WORLD, &(mpi_size));
        MPI_Comm_rank(MPI_COMM_WORLD, &(mpi_rank));
    
        LOGGER_DEBUG("Communication channel with MPI configured.");
    }

    ~Impl() = default;

    void send(const int& measurement, const std::string& target)
    {
        int target_int = std::atoi(target.c_str());
        MPI_Send(&measurement, 1, MPI_INT, target_int, 1, MPI_COMM_WORLD);
        
    }

    int recv(const std::string& origin)
    {
        int measurement;
        int origin_int = std::atoi(origin.c_str());
        MPI_Recv(&measurement, 1, MPI_INT, origin_int, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        return measurement;
    }

    void send_str(const std::string& data, const std::string& target)
    {
        int target_int = std::atoi(target.c_str());
        int size = data.size();
        MPI_Send(&size, 1, MPI_INT, target_int, 1, MPI_COMM_WORLD);
        MPI_Send(&data, size, MPI_CHAR, target_int, 1, MPI_COMM_WORLD);
    }

    std::string recv_str(const std::string& origin)
    {
        int datasize;
        std::string data;
        int origin_int = (origin == "executor") ? (mpi_size - 1) : std::atoi(origin.c_str());
        LOGGER_DEBUG("origin_int: {}", std::to_string(origin_int));
        MPI_Recv(&datasize, 1, MPI_INT, origin_int, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&data, datasize, MPI_CHAR, origin_int, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        return data;
    }
};

ClassicalChannel::ClassicalChannel(const std::string& qpu_id) : 
    qpu_id{qpu_id},
    pimpl_{std::make_unique<Impl>(qpu_id)} 
{ 
    endpoint = std::to_string(pimpl_->mpi_rank);
}

ClassicalChannel::~ClassicalChannel() = default;


void ClassicalChannel::publish()
{
    JSON endpoint = 
    {
        {"endpoint", endpoint}
    };
    write_on_file(endpoint, constants::COMM_FILEPATH, qpu_id);
}

void connect(const std::string& qpu_id)
{
    LOGGER_DEBUG("connect(const std::string& endpoint, const std::string& id) not implemented for MPI");
}

void ClassicalChannel::send_info(const std::string& data, const std::string& target)
{
    pimpl_->send_str(data, target);
}

std::string ClassicalChannel::recv_info(const std::string& origin)
{
    return pimpl_->recv_str(origin);
}

void ClassicalChannel::send_measure(const int& measurement, const std::string& target)
{
    pimpl_->send(measurement, target);
}

int ClassicalChannel::recv_measure(const std::string& origin)
{
    return pimpl_->recv(origin);
}

} // End of comm namespace
} // End of cunqa namespace

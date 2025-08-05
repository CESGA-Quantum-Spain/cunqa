
#include <string>
#include <mpi.h>

#include "classical_channel.hpp"
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

    void send(int& measurement, std::string& target)
    {
        int target_int = std::atoi(target.c_str());
        MPI_Send(&measurement, 1, MPI_INT, target_int, 1, MPI_COMM_WORLD);
        
    }

    int recv(std::string& origin)
    {
        int measurement;
        int origin_int = std::atoi(origin.c_str());
        MPI_Recv(&measurement, 1, MPI_INT, origin_int, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        return measurement;
    }
};

ClassicalChannel::ClassicalChannel() : pimpl_{std::make_unique<Impl>()}, endpoint{std::to_string(pimpl_->mpi_rank)}
{}

ClassicalChannel::~ClassicalChannel() = default;

void ClassicalChannel::send_measure(int& measurement, std::string& target)
{
    pimpl_->send(measurement, target);
}

int ClassicalChannel::recv_measure(std::string& origin)
{
    return pimpl_->recv(origin);
}

void ClassicalChannel::connect(std::vector<std::string>& endpoints)
{
    LOGGER_DEBUG("MPI does not need to set connections.");
}
} // End of comm namespace
} // End of cunqa namespace

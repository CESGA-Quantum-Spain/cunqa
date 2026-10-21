#pragma once

#include <string>
#include <filesystem>

#include "utils/constants.hpp"
#include "utils/json.hpp"

/*
 * Registry of the deployed vQPUs and of their communication endpoints:
 *
 *     QPUS_DIR/<family>/<qpu_id>.json
 *     COMM_DIR/<slurm_job_id>/<id>.json
 *
 * Every file has a single writer (the process it describes), which writes it once atomically, so
 * no locks are needed. A shared file updated with read-modify-write loses entries on NFS when
 * processes of several nodes write it at the same time, even under fcntl locks.
 */

namespace cunqa {

// vQPU and executor ids are of the form <slurm_job_id>_<suffix>
inline std::string job_id_of(const std::string& id) { return id.substr(0, id.find('_')); }

inline std::string family_dirpath(const std::string& family) { return QPUS_DIR + "/" + family; }

inline std::string qpu_filepath(const std::string& family, const std::string& qpu_id)
{
    return family_dirpath(family) + "/" + qpu_id + ".json";
}

inline std::string comm_dirpath(const std::string& job_id) { return COMM_DIR + "/" + job_id; }

inline std::string comm_filepath(const std::string& id)
{
    return comm_dirpath(job_id_of(id)) + "/" + id + ".json";
}

// Returns {<qpu_id>: <qpu info>} for the vQPUs of a family, or of all families if none is given
inline JSON read_qpus(const std::string& family = "")
{
    if (!family.empty())
        return read_dir(family_dirpath(family));

    JSON qpus = JSON::object();
    std::error_code ec;
    std::filesystem::directory_iterator it(QPUS_DIR, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        std::error_code type_ec;
        if (it->is_directory(type_ec))
            qpus.update(read_dir(it->path().string()));
    }
    return qpus;
}

} // End of cunqa namespace

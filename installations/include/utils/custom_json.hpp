#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class CustomJson {
    json custom_json;
    std::string filename;
public:
    CustomJson();
    ~CustomJson();

    void write(json local_data, const std::string &filename);    
    const std::string dump();
private:
    void _write_MPI(json local_data);
    void _write_locks(json local_data);
};
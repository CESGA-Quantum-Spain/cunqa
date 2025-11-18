#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

#include "utils/json.hpp"

using namespace cunqa;

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Error, two arguments have to be provided: " << argv[0] << " <job_id> <info_path>\n";
            return 1;
        }

        const std::string job_id = argv[1];
        const std::string info_path = argv[2];
        remove_from_file(info_path, job_id);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Excepción no controlada: " << e.what() << "\n";
        return 1;
    }
}

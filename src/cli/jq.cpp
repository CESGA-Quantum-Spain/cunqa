// filter_json.cpp
// Uso:      ./filter_json <job_id> <info_path>
// Compilar: g++ -std=c++17 -O2 -o filter_json filter_json.cpp
// Requiere: nlohmann/json (single-header: json.hpp)

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Uso: " << argv[0] << " <job_id> <info_path>\n";
            return 1;
        }

        const std::string job_id = argv[1];
        const fs::path info_path = argv[2];

        // Leer JSON de info_path
        std::ifstream in(info_path);
        if (!in) {
            std::cerr << "Error: no se pudo abrir " << info_path << " para lectura.\n";
            return 1;
        }

        json j;
        try {
            in >> j;
        } catch (const std::exception& e) {
            std::cerr << "Error parseando JSON en " << info_path << ": " << e.what() << "\n";
            return 1;
        }

        if (!j.is_object()) {
            std::cerr << "Error: el contenido de " << info_path << " no es un objeto JSON.\n";
            return 1;
        }

        // Filtrar: mantener entradas cuya clave NO empieza por job_id
        json out = json::object();
        for (auto it = j.begin(); it != j.end(); ++it) {
            const std::string& key = it.key();
            bool starts_with = key.rfind(job_id, 0) == 0; // true si empieza por job_id
            if (!starts_with) {
                out[it.key()] = it.value();
            }
        }

        // Escribir a fichero temporal y reemplazar atómicamente
        fs::path tmp_path = info_path.parent_path() / "tmp_info.json";
        {
            std::ofstream outFile(tmp_path, std::ios::trunc);
            if (!outFile) {
                std::cerr << "Error: no se pudo abrir " << tmp_path << " para escritura.\n";
                return 1;
            }
            outFile << out.dump(2) << std::endl; // indentación bonita
            if (!outFile) {
                std::cerr << "Error: fallo al escribir " << tmp_path << ".\n";
                return 1;
            }
        }

        std::error_code ec;
        fs::rename(tmp_path, info_path, ec); // atómico en el mismo FS
        if (ec) {
            fs::remove(info_path, ec); // intenta quitar destino si necesario
            fs::rename(tmp_path, info_path, ec);
            if (ec) {
                std::cerr << "Error: no se pudo mover " << tmp_path << " a " << info_path
                          << ": " << ec.message() << "\n";
                return 1;
            }
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Excepción no controlada: " << e.what() << "\n";
        return 1;
    }
}

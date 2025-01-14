#include <iostream>
#include <vector>
#include "helpers.hpp"
#include <bitset>
 #include <nlohmann/json.hpp>
using json = nlohmann::json;

std::streamsize get_file_size(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate); // Abrir en modo binario y posicionar al final
    if (!file) {
        std::cerr << "No se pudo abrir el archivo: " << filename << std::endl;
        return -1;
    }
    return file.tellg(); // Obtener la posición actual, que es el tamaño del archivo
}


void saveincsv(std::vector<int> x, std::vector<int> y) {
    std::ofstream file("data.csv");
    if (!file.is_open()) {
        std::cerr << "Error al abrir el archivo.\n";
        return 1;
    }

    // Escribir encabezado
    file << "x,y\n";

    // Escribir datos
    for (size_t i = 0; i < x.size(); ++i) {
        file << x[i] << "," << y[i] << "\n";
    }

    file.close();
}

int main(int argc, const char * argv[]) { 

    if (argc > 1) {
        std::cout << "Generating new file of size " << argv[1] << "...\n";
        generate_random_circuit(std::stoul(argv[1], nullptr, 0), "prueba.json");
    }
    
    json j;
    std::ifstream file_in("prueba.json");

    if (file_in.peek() != std::ifstream::traits_type::eof())
        file_in >> j;
    file_in.close();

    auto start = std::chrono::high_resolution_clock::now();
    auto bin_circ = from_json_to_bin(j);
    auto end = std::chrono::high_resolution_clock::now();

    double binario = static_cast<double>(bin_circ.size());
    double string = static_cast<double>(get_file_size("prueba.json"));
/* 
    std::cout << binario << "\n";
    std::cout << string << "\n";
    std::cout << binario/string << "\n\n"; */

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Tiempo transcurrido: " << duration.count() << " ms." << std::endl;
    


    /* start = std::chrono::high_resolution_clock::now();
    auto circ_json = from_bin_to_json(bin_circ); 
    end = std::chrono::high_resolution_clock::now();

    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Tiempo transcurrido: " << duration.count() << " ms." << std::endl; */
    return 0;
}


#include <string>
#include <fstream>
#include <vector>
#include <optional>
#include <cstdlib>
#include <map>
#include <nlohmann/json.hpp>
#include "argparse.hpp"

#include "logger/logger.hpp"

using json = nlohmann::json;
using namespace std::literals;

struct MyArgs : public argparse::Args
{
    std::optional<std::string>& node     = kwarg("node", "Info about the QPUs on the selected node.");
    bool& my_node                        = flag("mynode", "Info about the QPUs on the current node.");

    void welcome() {
        std::cout << "Command to get information about the deployed QPUs." << "\n";
    }
};

int main(int argc, char* argv[]) {

    const std::string indent = "    ";

    auto args = argparse::parse<MyArgs>(argc, argv);

    const char* store = std::getenv("STORE");
    std::string info_path = std::string(store) + "/.cunqa/qpus.json";

    std::ifstream file(info_path);
    if (!file.is_open()) {
        std::cerr << "\033[31mCould not open the QPUs info file! Check if there are deployed QPUs. \033[0m " << "\n";
        return 1;
    }

    json qpus_json;
    file >> qpus_json;

    if (qpus_json.empty()) {
        std::cerr << "\033[31mThere are not deployed QPUs!\033[0m" << "\n";
        return 1;
    }

    std::map<std::string, std::map<std::string, int>> family_counts_per_node;
    std::map<std::string, std::vector<std::string>> id_per_node;
    
    for (auto& [key, inner] : qpus_json.items()) {
        std::string node = inner["net"]["node_name"];
        std::string family_name = inner["family_name"];
        
        family_counts_per_node[node][family_name]++;
        id_per_node[node].push_back(key);
    }

    if(args.node.has_value()) {
        std::vector<std::string> node_ids = id_per_node[args.node.value()];

        std::cout << "QPUs in \033[34mNode " << args.node.value() << "\033[0m:" << "\n";
        for (auto& id : node_ids) {
            std::cout << "ID: \033[32m" << id << "\033[0m \n";
            std::cout << indent << "Name: " << qpus_json[id]["backend"]["name"] << "\n";
            std::cout << indent << "Simulator: " << qpus_json[id]["backend"]["simulator"] << "\n";
            std::cout << indent << "Family_name: " << qpus_json[id]["family_name"] << "\n";
            std::cout << indent << "Description: " << qpus_json[id]["backend"]["description"] << "\n";
        }
    } else if (args.my_node) {
        const char* slurm_nodename = std::getenv("SLURMD_NODENAME");
        if (slurm_nodename == nullptr) 
        {
            std::cerr << "\033[31mProblem accessing to the QPUs on the current node. Probably the command was run on a login node.\033[0m" << "\n";
            return 1;
        }
        
        if (family_counts_per_node.find(slurm_nodename) != family_counts_per_node.end()) {
            std::cout << "In current \033[34mNode " << slurm_nodename << "\033[0m there are: " << "\n";
            for (auto& [family_name, number] : family_counts_per_node[slurm_nodename]) {
                std::cout << indent << number << " QPUs with family name: " << family_name << "\n";
            }
        } else {
            std::cout << "\033[33mNode No QPUs deployed on the current node " << slurm_nodename << "\033[0m \n";
        }
    } else {
        for (auto& [node_name, node_info] : family_counts_per_node) {
            std::cout << "In \033[34mNode " << node_name << "\033[0m there are: " << "\n";
            for (auto& [family_name, number] : node_info) {
                std::cout << indent << number << " QPUs with family name: " << family_name << "\n";
            }
        }
    }

    return 0;
}
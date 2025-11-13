
#include <string>
#include <fstream>
#include <vector>
#include <optional>
#include <cstdlib>
#include <map>

#include "utils/json.hpp"
#include "argparse/argparse.hpp"
#include "logger.hpp"

using namespace std::literals;

struct CunqaArgs : public argparse::Args
{
    std::optional<std::string>& node     = arg("node", "Info about the QPUs on the selected node.");
    bool& my_node                        = flag("mynode", "Info about the QPUs on the current node.");

    void welcome() {
        std::cout << "Command to get information about the deployed QPUs." << "\n";
    }
};

int main(int argc, char* argv[]) {

    const std::string indent = "    ";

    auto args = argparse::parse<CunqaArgs>(argc, argv);

    const char* store = std::getenv("STORE");
    std::string info_path = std::string(store) + "/.cunqa/qpus.json";

    std::ifstream file(info_path);
    if (!file.is_open()) {
        std::cerr << "\033[31mCould not open the QPUs info file! Check if there are deployed QPUs. \033[0m " << "\n";
        return 1;
    }

    cunqa::JSON qpus_json;
    file >> qpus_json;
    file.close();

    if (qpus_json.empty()) {
        std::cerr << "\033[31mThere are not deployed QPUs!\033[0m" << "\n";
        return 1;
    }

    std::map<std::string, std::map<std::string, int>> family_counts_per_node;
    std::map<std::string, std::vector<std::string>> id_per_node;
    
    for (auto& [key, inner] : qpus_json.items()) {
        std::string node = inner["net"]["nodename"];
        std::string family = inner["family"];
        
        family_counts_per_node[node][family]++;
        id_per_node[node].push_back(key);
    }

    if(args.node.has_value()) {
        std::vector<std::string> node_ids = id_per_node[args.node.value()];

        std::cout << "QPUs in \033[34mNode " << args.node.value() << "\033[0m:" << "\n";
        for (auto& id : node_ids) {
            std::cout << "ID: \033[32m" << id << "\033[0m \n";
            std::cout << indent << "Name: " << qpus_json[id]["backend"]["name"] << "\n";
            std::cout << indent << "Description: " << qpus_json[id]["backend"]["description"] << "\n";
            std::cout << indent << "family: " << qpus_json[id]["family"] << "\n";
            std::cout << indent << "Simulator: " << qpus_json[id]["backend"]["simulator"] << "\n";
            std::cout << indent << "Mode: " << qpus_json[id]["net"]["mode"] << "\n";
            
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
            for (auto& [family, number] : family_counts_per_node[slurm_nodename]) {
                std::cout << indent << number << " QPUs with family name " << family << "\n";
            }
        } else {
            std::cout << "\033[33mNode No QPUs deployed on the current node " << slurm_nodename << "\033[0m \n";
        }
    } else {
        for (auto& [node_name, node_info] : family_counts_per_node) {
            std::cout << "In \033[34mNode " << node_name << "\033[0m there are: " << "\n";
            for (auto& [family, number] : node_info) {
                std::cout << indent << number << " QPUs with family name: " << family << "\n";
            }
        }
    }

    return 0;
}
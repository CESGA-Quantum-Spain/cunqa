#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <sys/types.h>
#include <string_view>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "utils/constants.hpp"


#ifndef __USE_POSIX
    #define __USE_POSIX
#endif
#include <limits.h>

using namespace std::literals;
using json = nlohmann::json;

namespace config {

class NetConfig {
public:
    std::string mode;
    std::string hostname;
    std::string nodename;
    std::unordered_map<std::string, std::string> IPs;
    std::string port;

    NetConfig();
    NetConfig(const std::string& mode, const std::string& hostname, const std::string& nodename, std::unordered_map<std::string, std::string> IPs, std::string port);
    NetConfig(const json& server_info);

    static NetConfig myNetConfig(const std::string& mode);
    std::string get_endpoint(const std::string_view& net = INFINIBAND) const;

    NetConfig& operator=(const NetConfig& other);
};

std::string get_hostname();
std::string get_nodename();
std::unordered_map<std::string, std::string> get_IP_addresses();
std::string get_port();

void to_json(json& j, const NetConfig& net_config);
void from_json(const json& j, NetConfig& NetConfig);

NetConfig::NetConfig() = default;

NetConfig::NetConfig(const std::string& mode, const std::string& hostname, const std::string& nodename, std::unordered_map<std::string, std::string> IPs, std::string port)
    :   mode{mode},
        hostname{hostname},
        nodename{nodename},
        IPs{IPs},
        port{port} 
{ }

NetConfig::NetConfig(const json& server_info)
{
    from_json(server_info, *this);
}

NetConfig NetConfig::myNetConfig(const std::string& mode) 
{
    return NetConfig(mode, get_hostname(), get_nodename(), get_IP_addresses(), get_port());
}

std::string NetConfig::get_endpoint(const std::string_view& net) const 
{
    return IPs.at(std::string(net)) + ":" + port;
}

NetConfig& NetConfig::operator=(const NetConfig& other) 
{
    if (this != &other) {
        mode = other.mode;
        hostname = other.hostname;
        nodename = other.nodename;
        IPs = other.IPs;
        port = other.port;
    }
    return *this;
}

// ------------------------------------------------
// ------------- Net getter functions -------------
// ------------------------------------------------

std::string get_hostname(){
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return hostname;
}

std::string get_nodename(){
    auto nodename = std::getenv("SLURMD_NODENAME");
    return (std::string)(nodename);
}

std::unordered_map<std::string, std::string> get_IP_addresses() 
{
    std::unordered_map<std::string, std::string> ips{};
    struct ifaddrs *interfaces, *ifa;

    if (getifaddrs(&interfaces) == -1) {
        std::cerr << "Error getting the network interfaces\n";
        return std::unordered_map<std::string, std::string>();
    }

    for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            char ip[INET6_ADDRSTRLEN];
            void *addr;
            
            addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(family, addr, ip, sizeof(ip));

            ips[ifa->ifa_name] = ip;
        }
    }

    freeifaddrs(interfaces);
    return ips;
}
 
std::string get_port() 
{
    auto id = std::getenv("SLURM_LOCALID");
    std::string ports = std::getenv("SLURM_STEP_RESV_PORTS");

    if(ports != "" && id != "")
    {
        size_t pos = ports.find('-');
        if (pos != std::string::npos) 
        {
            std::string base_port_str = ports.substr(0, pos);
            int base_port = std::stoi(base_port_str);
            
            return std::to_string(base_port + std::stoi(id));
            
        } else 
        {
            std::cerr << "Not a valid expression format of the ports.\n";
            return "-1"s;
        }
    } else 
    {
        std::cerr << "The required environment variables are not set (not in a SLURM job).\n";
        return "-1"s;
    }
}

// ------------------------------------------------
// ----- Conversion functions for json format -----
// ------------------------------------------------

void to_json(json& j, const NetConfig& net_config)
{
    json ips{};

    for (const auto& net_bind : net_config.IPs) {
        ips[net_bind.first] = net_bind.second;
    }

    j = {   
            {"mode", net_config.mode},
            {"hostname", net_config.hostname}, 
            {"node_name", net_config.nodename},
            {"IPs", ips},
            {"port", net_config.port},

        };
}

void from_json(const json& j, NetConfig& NetConfig) 
{
    j.at("mode").get_to(NetConfig.mode);
    j.at("hostname").get_to(NetConfig.hostname);
    j.at("node_name").get_to(NetConfig.nodename);
    for (auto& netbind : j.at("IPs").items()) {
        NetConfig.IPs[netbind.key()] = netbind.value();
    }
    j.at("port").get_to(NetConfig.port);
}; 

};

std::ostream& operator<<(std::ostream& os, const config::NetConfig& config) {
    os << "\nIPs: \n";
    for (const auto& net_bind : config.IPs) {
            os << net_bind.first << " ---> " << net_bind.second << "\n";
    }
    os << "\nPuerto: " << config.port
        << "\nNodename: " << config.nodename 
       << "\nHostname: " << config.hostname 
       << "\nMode: " << config.mode << "\n\n";
    return os;
}
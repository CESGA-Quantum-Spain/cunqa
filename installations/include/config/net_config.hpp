#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "utils/constants.hpp"
#include "config.hpp"

#ifndef __USE_POSIX
    #define __USE_POSIX
#endif
#include <limits.h>

using namespace std::literals;
using json = nlohmann::json;

namespace config {
namespace net {

class NetConfig : Config {
public:
    std::string hostname;
    std::unordered_map<std::string, std::string> IPs;
    std::string port;

    NetConfig();
    NetConfig(const std::string& hostname, std::unordered_map<std::string, std::string> IPs, std::string port);
    NetConfig(const json& server_info);

    static NetConfig myNetConfig();
    std::string get_endpoint(const std::string_view& net = INFINIBAND) const;

    inline void load(const json& config) override;
    inline json dump() const override;

    NetConfig& operator=(const NetConfig& other);
};

std::string get_hostname();
std::unordered_map<std::string, std::string> get_IP_addresses();
std::string get_port();

void to_json(json& j, const NetConfig& net_config);
void from_json(const json& j, NetConfig& NetConfig);

NetConfig::NetConfig() = default;

NetConfig::NetConfig(const std::string& hostname, std::unordered_map<std::string, std::string> IPs, std::string port)
    : hostname{hostname},
        IPs{IPs},
        port{port} 
{ }

NetConfig::NetConfig(const json& server_info)
{
    load(server_info);
}

NetConfig NetConfig::myNetConfig() 
{
    return NetConfig(get_hostname(), get_IP_addresses(), get_port());
}

std::string NetConfig::get_endpoint(const std::string_view& net) const 
{
    return IPs.at(std::string(net)) + ":" + port;
}

inline void NetConfig::load(const json& config) 
{
    from_json(config, *this);
}

inline json NetConfig::dump() const 
{
    return json(*this);
}

NetConfig& NetConfig::operator=(const NetConfig& other) 
{
    if (this != &other) {
        hostname = other.hostname;
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

// TODO: Hacer que el get_port no esté tan ligado a SLURM 
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
            {"hostname", net_config.hostname}, 
            {"IPs", ips},
            {"port", net_config.port},
        };
}

void from_json(const json& j, NetConfig& NetConfig) 
{
        
    j.at("hostname").get_to(NetConfig.hostname);
    for (auto& netbind : j.at("IPs").items()) {
        NetConfig.IPs[netbind.key()] = netbind.value();
    }
    j.at("port").get_to(NetConfig.port);
}; 


}
};

std::ostream& operator<<(std::ostream& os, const config::net::NetConfig& config) {
    os << "\nIPs: \n";
    for (const auto& net_bind : config.IPs) {
            os << net_bind.first << " ---> " << net_bind.second << "\n";
    }
    os << "\nPuerto: " << config.port
       << "\nHostname: " << config.hostname << "\n\n";
    return os;
}
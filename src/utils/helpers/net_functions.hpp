#pragma once

#include <iostream>
#include <sys/types.h>
#include <string>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "logger.hpp"
#include "utils/constants.hpp"

using namespace std::string_literals;

// Secure cast of size
template<typename TO, typename FROM>
TO legacy_size_cast(FROM value)
{
    static_assert(std::is_unsigned_v<FROM> && std::is_unsigned_v<TO>,
                  "Only unsigned types can be cast here!");
    TO result = value;
    return result;
}

// ------------------------------------------------
// ------------- Net getter functions -------------
// ------------------------------------------------

inline std::string get_hostname(){
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return hostname;
}

inline std::string get_nodename(){
    auto nodename = std::getenv("SLURMD_NODENAME");
    if (!nodename)
        return "login"s;
    return nodename;
}

inline std::string get_IP_address(const std::string& mode) 
{
    struct ifaddrs *interfaces, *ifa;
    
    if(mode == "hpc") 
        return "127.0.0.1"s;

    if (getifaddrs(&interfaces) == -1) {
        std::cerr << "Error getting the network interfaces\n";
        return std::string();
    }

    char ip[INET6_ADDRSTRLEN];
    for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || std::string(ifa->ifa_name) != "ib0") continue;
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            void *addr;
            addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(family, addr, ip, sizeof(ip));
            break;
        }
    }
    freeifaddrs(interfaces);
    return ip;
}

inline std::string get_global_IP_address() 
{
    struct ifaddrs *interfaces, *ifa;

    if (getifaddrs(&interfaces) == -1) {
        std::cerr << "Error getting the network interfaces\n";
        return std::string();
    }

    char ip[INET6_ADDRSTRLEN];
    for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || std::string(ifa->ifa_name) != "ib0") continue;
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            void *addr;
            addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(family, addr, ip, sizeof(ip));
            break;
        }
    }
    freeifaddrs(interfaces);
    return ip;
}
 
inline std::string get_port(const bool comm = false) 
{
    auto id = std::getenv("SLURM_PROCID");
    auto ports = std::getenv("SLURM_STEP_RESV_PORTS");

    if(ports && id) {
        std::string ports_str(ports);
        size_t pos = ports_str.find('-');
        if (pos != std::string::npos) {
            if (comm) {
                std::string base_port_str = ports_str.substr(pos + 1);
                int base_port = std::stoi(base_port_str);
                
                return std::to_string(base_port - std::stoi(id));
            } else {
                std::string base_port_str = ports_str.substr(0, pos);
                int base_port = std::stoi(base_port_str);
                
                return std::to_string(base_port + std::stoi(id));
            }
            
        }
        throw std::runtime_error("Not a valid expression format of the ports.");
    }
    throw std::runtime_error("The required environment variables are not set (not in a SLURM job).");
    return std::string();
}
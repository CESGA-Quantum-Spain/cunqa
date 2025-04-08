#pragma once

#include <string_view>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include<complex>

using json = nlohmann::json;

// NETWORK INTERFACES NAMES
constexpr std::string_view INFINIBAND = "ib0";
constexpr std::string_view VLAN120 = "VLAN120";
constexpr std::string_view VLAN117 = "VLAN117";

enum communications {
  no_comm,
  class_comm,
  quantum_comm
};

std::unordered_map<std::string, int> comm_map = {

  {"no_comm", no_comm},
  {"class_comm", class_comm},
  {"quantum_comm", quantum_comm}


};


namespace CUNQA {

  enum INSTRUCTIONS {
    MEASURE,
    ID,
    X,
    Y,
    Z,
    H,
    SX,
    RX,
    RY,
    RZ,
    CX,
    CY,
    CZ,
    ECR,
    C_IF_H,
    C_IF_X,
    C_IF_Y,
    C_IF_Z,
    C_IF_RX,
    C_IF_RY,
    C_IF_RZ,
    C_IF_CX,
    C_IF_CY,
    C_IF_CZ,
    C_IF_ECR,
    D_C_IF_H,
    D_C_IF_X,
    D_C_IF_Y,
    D_C_IF_Z,
    D_C_IF_RX,
    D_C_IF_RY,
    D_C_IF_RZ,
    D_C_IF_CX,
    D_C_IF_CY,
    D_C_IF_CZ,
    D_C_IF_ECR,
};

std::unordered_map<std::string, int> INSTRUCTIONS_MAP = {
    // MEASURE
    {"measure", MEASURE},

    // ONE GATE NO PARAM
    {"id", ID},
    {"x", X},
    {"y", Y},
    {"z", Z},
    {"h", H},
    {"sx", SX},

    // ONE GATE PARAM
    {"rx", RX},
    {"ry", RY},
    {"rz", RZ},

    // TWO GATE NO PARAM
    {"cx", CX},
    {"cy", CY},
    {"cz", CZ},
    {"ecr", ECR},

    //CONTROLLED GATES
    {"c_if_h", C_IF_H},
    {"c_if_x", C_IF_X},
    {"c_if_y", C_IF_Y},
    {"c_if_z", C_IF_Z},
    {"c_if_rx", C_IF_RX},
    {"c_if_ry", C_IF_RY},
    {"c_if_rz", C_IF_RZ},
    {"c_if_cx", C_IF_CX},
    {"c_if_cy", C_IF_CY},
    {"c_if_cz", C_IF_CZ},
    {"c_if_ecr", C_IF_ECR},

    //DISTRIBUTED GATES
    {"d_c_if_h", D_C_IF_H},
    {"d_c_if_x", D_C_IF_X},
    {"d_c_if_y", D_C_IF_Y},
    {"d_c_if_z", D_C_IF_Z},
    {"d_c_if_rx", D_C_IF_RX},
    {"d_c_if_ry", D_C_IF_RY},
    {"d_c_if_rz", D_C_IF_RZ},
    {"d_c_if_cx", D_C_IF_CX},
    {"d_c_if_cy", D_C_IF_CY},
    {"d_c_if_cz", D_C_IF_CZ},
    {"d_c_if_ecr", D_C_IF_ECR},
};

std::unordered_map<int, std::string> INVERTED_GATE_NAMES = {
    {MEASURE, "measure"},
    {ID, "id"},
    {X, "x"},
    {Y, "y"},
    {Z, "z"},
    {H, "h"},
    {SX, "sx"},
    {RX, "rx"},
    {RY, "ry"},
    {RZ, "rz"},
    {CX, "cx"},
    {CY, "cy"},
    {CZ, "cz"},
    {ECR, "ecr"},
};

const std::vector<std::string> BASIS_GATES = {
            "u1", "u2", "u3", "u", "p", "r", "rx", "ry", "rz", "id",
            "x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg",
            "swap", "cx", "cy", "cz", "csx", "cp", "cu", "cu1", "cu3",
            "rxx", "ryy", "rzz", "rzx", "ccx", "ccz", "crx", "cry", "crz",
            "cswap"};

  const std::vector<std::string> DISTRIBUTED_GATES = {
    "d_c_if_h", "d_c_if_x","d_c_if_y","d_c_if_z","d_c_if_rx","d_c_if_ry","d_c_if_rz","d_c_if_cx","d_c_if_cy","d_c_if_cz", "d_c_if_ecr"
  };

  std::unordered_map<std::string, std::string> CORRESPONDENCE_D_GATE_MAP = {
    {"d_c_if_h", "h"},
    {"d_c_if_x", "x"},
    {"d_c_if_y", "y"},
    {"d_c_if_z", "z"},
    {"d_c_if_rx", "rx"},
    {"d_c_if_ry", "ry"},
    {"d_c_if_rz", "rz"},
    {"d_c_if_cx", "cx"},
    {"d_c_if_cy", "cy"},
    {"d_c_if_cz", "cz"},
    {"d_c_if_ecr", "ecr"},
  };

} //End namespace CUNQA





const std::string cafe = R"(                                                                                                
                                                            ##*%                                                                                                
                                                          ###%###                                                                                               
                                                      *  ###%%%%%                                                                                               
                                                  #%  #%%#%%%%%%%                                                                                               
                                                  *%%%######%%%##%%%%%%#****                                                                                    
                                                  *###############%%%%%%%%##%##                                                                                 
                                                  %%%%##%**#*%######%%%#%%#####%%#                                                                             
                                                  *#%#%#%##%*%**%###%######%####%%%###                                #%%%%%%%%%                                
                                                   +#%##%%####*%%%%###%##*#####%%#%%#%###                          #%%%%%%%%%@%%###                             
                                                     ###%#%%%#*#@%#############%%#%%%%#######                     %%%%#%%%%%%%%%%@%%%                           
                                                     %####%%@@*%@%#%%###%%###%%#%#######*######                  #@%%%%%@%%%%%%%%#%%@%                          
                                                        %#*#@%#%%%%%#%%%##%####*####*#*#***#%#%##               #%%%*%%%%%%%%@@%%%%@%#                          
                                                        %##%%%%%@@%%@%%%%%%####%%#********####*##*%#         ##%%%%%%%%#%%%#%%%@%@@%%%                          
                                                         ##%%%%%%%@@%%%%%%%%%%%%####%#######%#####%#%#       %%%%%%%%%%%%%%%%#%@@%@%@%%                         
                                                          #%%%%%#@%@%%%%%%%%%%%%@%%%###%%#%##%##%*#####*   #%%%@%%%%%%%##%%@%@@@@%%@%%@                         
                                                                 #%%@%%%%%%%%@%@%@@%%%%%%%@%#%%#%###*##%#%%%%%%%%%%###%%%%%@@@@@@@%@@@%                         
                                                                  %%%@@@@@%@@@@@@@@%%%%#%#%%%####%%####%%%@%%%%%##%%%#%%@@%@@%%%%%@@%@%                         
                                                                   %%%%@@%@@@%%@%%%#%%%%%#####%########%@%%@@%%%%%%%%%%@%%%%%%%%%*@@%@%                         
                                                                    %%%%%%%%@%%%%%%%%%%%%##%%%%%%%#####%%%%#%%%%%%%%%%%@@%%%%%#%**@@%@                          
                                                                @@@   %%%%%%%%%%%%%@@@%%%%#%@%%%##%##%####%%%%%%%%%%%%@@%%@@@%%@@@@%@                           
                                                             %%%%%%#+*##%@%%#%%%%%%%%%%%%%%%%%%%%%##%#%####%%@%%%%%%@@%@%%%@%#%@@%@%%%%%%                       
                                                          %#%%%%%*%%%%%+=-*#%%%%@%%##%%%%%%#%%%#%%%%%##%%%#%@%%%%%%%%%%%%%#%*###%%%                             
                                                        ##%%%%%*=====+##%%*++=*%%%%#%#####%#%##%%#%%%#%%%%%%@@@%@%#%%%%%# ##* %%%                               
                                                      %%*%@%%%%%+=++=++=+%%%@*=*+*#%#*#%%%#**####%##%@%%%%#%@@@%%%%%@%%%###                                     
                                                     %#=:======%%%%#=====+*%%%#**+=*###%%%#**#*%%##%@%%%%##@@@@@@%@%## # #                                      
                                                   %%%+=+===--=+==*%##====+=*+**++***%####**####%%%%%%%%#%%@@@@@%%%%##                                          
                                                 ###+======-::..-==++%%*--=#%%#*#**+#+%%%*##%####%%%%%%%%%%%%%%%%%%%%%                                          
                                                %%==-=-==-=------::--=+%%%%%%%   *#*+*+*####%%%%%%%%%%##%%%%%###%%%                                             
                                              %%+=+**=+=-+++++=++=====+=*#%%#        #**+%%%%%%%%%%%%%#%%%%##*#*                                                
                                            +*#*#%%*++++++++++++++*==#%%%%%*         -#*#%%%%%%%%%%%%%%%%#* %*                                                  
                                           *#*-%#%%%%%@@@@@@@@@@@@@%%#@=#%%           #%#%%%@@@@%%%%%%%%#*+                                                     
                                          +**=  #%%%@@@@@@@@@@@@@%@@%%*-#@            #%#%#%%%%%%##%%%%#%                                                       
                                          ++=   #*#%%%####@@@@@@@@@%##%@%%             %%%%@%%#%%%###%                                                          
                                         =++=    *##########%@%@@%%%#%#%#             %%%%%@%@%@%                                                               
                                         ++=-    -=%%##########%###@%%+.              :+%%%%%%%%%:                                                              
                                        :-=:      :-=#%#######=--%#%%                  :-%%%%*+#-                                                               
                                        :-=         ::+##%##=-:##*%                      :.=+**-                                                                
                                       .--            ::+#*==%#*#                          +***.                                                                
                                                  @@     .+#*=                             +**#:                                                                
                                                 @@                                        +###.                                                              
                                                                                           =###****                                                           
                      @@@@@@    @@   @@@@@@@ @@@@@@@  @@@@@@                              :=###=                                                              
                      @@       @@@@  @@      @@           @@                         -+*+=*####-                                                               
                      @@      @@  @@ @@@@@@@ @@@@@@@    @@@                      =+#######%%##**#-                                                              
                      @@      @@  @@ @@@     @@         @@                               %+##  -##:                                                             
                      @@      @@@@@@ @@@     @@                                         -:+++                                                               
                      @@@@@@  @@  @@ @@      @@@@@@@@   @@                                                                                                                                                                                                                                                                   
)";
#pragma once

#include <vector>
#include <complex>
#include <unordered_map>

namespace cunqa {
namespace constants{

enum INSTRUCTIONS {
    UNITARY,
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
    P,
    R,
    U,
    U1,
    U3,
    CU,
    CU1,
    CU3,
    SWAP,
    CX,
    CY,
    CZ,
    CRX,
    CRY,
    CRZ,
    CP,
    RXX,
    RYY,
    RZZ,
    RZX,
    ECR,
    CECR,
    C_IF_X,
    C_IF_Y,
    C_IF_Z,
    C_IF_H,
    C_IF_SX,
    C_IF_RX,
    C_IF_RY,
    C_IF_RZ,
    C_IF_P,
    C_IF_U,
    C_IF_U1,
    C_IF_CX,
    C_IF_CY,
    C_IF_CZ,
    C_IF_CRX,
    C_IF_CRY,
    C_IF_CRZ,
    C_IF_CP,
    C_IF_CU,
    C_IF_CU1,
    C_IF_CU3,
    C_IF_ECR,
    C_IF_SWAP,
    C_IF_RXX,
    C_IF_RYY,
    C_IF_RZZ,
    C_IF_RZX,
    C_IF_CECR,
    C_IF_CSWAP,
    MEASURE_AND_SEND,
    RECV,
    QSEND,
    QRECV
};

const std::unordered_map<std::string, int> INSTRUCTIONS_MAP = {
    // UNITARY
    {"unitary", UNITARY},

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
    {"p", P},
    {"u1", U1},

    // ONE GATE TWO PARAM
    {"r", R},

    // ONE GATE THREE PARAM
    {"u", U},
    {"u3", U3},

    // TWO GATE NO PARAM
    {"swap", SWAP},
    {"cx", CX},
    {"cy", CY},
    {"cz", CZ},
    {"ecr", ECR},

    // TWO GATE ONE PARAM
    {"crx", CRX},
    {"cry", CRY},
    {"crz", CRZ},
    {"rxx", RXX},
    {"ryy", RYY},
    {"rzz", RZZ},
    {"rzx", RZX},
    {"cp", CP},
    {"cu1", CU1},

    // TWO GATE THREE PARAM
    {"cu", CU},
    {"cu3", CU3},

    // THREE GATE NO PARAM
    {"cecr", CECR},

    //CLASSICAL CONTROLLED GATES
    {"c_if_x", C_IF_X},
    {"c_if_y", C_IF_Y},
    {"c_if_z", C_IF_Z},
    {"c_if_h", C_IF_H},
    {"c_if_sx", C_IF_H},
    {"c_if_rx", C_IF_SX},
    {"c_if_ry", C_IF_RY},
    {"c_if_rz", C_IF_RZ},
    {"c_if_p", C_IF_P},
    {"c_if_u", C_IF_U},
    {"c_if_u1", C_IF_U1},
    {"c_if_cx", C_IF_CX},
    {"c_if_cy", C_IF_CY},
    {"c_if_cz", C_IF_CZ},
    {"c_if_cp", C_IF_CP},
    {"c_if_cu", C_IF_CU},
    {"c_if_cu1", C_IF_CU1},
    {"c_if_cu3", C_IF_CU3},
    {"c_if_ecr", C_IF_ECR},
    {"c_if_swap", C_IF_SWAP},
    {"c_if_rxx", C_IF_RXX},
    {"c_if_ryy", C_IF_RYY},
    {"c_if_rzz", C_IF_RZZ},
    {"c_if_rzx", C_IF_RZX},
    {"c_if_cerc", C_IF_CECR},
    {"c_if_cswap", C_IF_CSWAP},


    // SEND CLASSICAL QUBIT
    {"measure_and_send", MEASURE_AND_SEND},
    {"recv", RECV},

    // REMOTE CONTROLLED GATES
    {"qsend", QSEND},
    {"qrecv", QRECV}
};

const std::vector<std::string> BASIS_GATES = {
    "u1", "u2", "u3", "u", "p", "r", "rx", "ry", "rz", "id",
    "x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg",
    "swap", "cx", "cy", "cz", "csx", "cp", "cu", "cu1", "cu3",
    "rxx", "ryy", "rzz", "rzx", "ccx", "ccz", "crx", "cry", "crz",
    "cswap"
};

const std::vector<std::string> BASIS_AND_DISTRIBUTED_GATES = {
    "id", "x", "y", "z", "h", "sx", "cx", "cy", "cz", "ecr", "c_if_x","c_if_y","c_if_z", "c_if_h", "c_if_sx", "c_if_rx","c_if_ry","c_if_rz","c_if_cx","c_if_cy","c_if_cz", "measure_and_send", "remote_c_if_x","remote_c_if_y","remote_c_if_z", "remote_c_if_h", "remote_c_if_sx", "remote_c_if_rx","remote_c_if_ry","remote_c_if_rz","remote_c_if_cx","remote_c_if_cy","remote_c_if_cz", "remote_c_if_ecr"
};

const std::unordered_map<std::string, std::string> CORRESPONDENCE_REMOTE_GATE_MAP = {
    {"remote_c_if_x", "x"},
    {"remote_c_if_y", "y"},
    {"remote_c_if_z", "z"},
    {"remote_c_if_h", "h"},
    {"remote_c_if_sx", "sx"},
    {"remote_c_if_rx", "rx"},
    {"remote_c_if_ry", "ry"},
    {"remote_c_if_rz", "rz"},
    {"remote_c_if_cx", "cx"},
    {"remote_c_if_cy", "cy"},
    {"remote_c_if_cz", "cz"},
    {"remote_c_if_ecr", "ecr"},
};

} // End namespace constants
} //End namespace cunqa





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
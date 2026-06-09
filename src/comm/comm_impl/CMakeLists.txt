# Selecting communication library between the client and the servers (QPUs)
option(USE_ASIO "Using the ASIO library for communication" OFF)
option(USE_ZMQ "Using the ZMQ library for communication" OFF)

if(USE_ASIO)
    add_subdirectory(asio)
elseif(USE_ZMQ)
    add_subdirectory(zmq)
else()
    # We set the default communication library to ZMQ
    add_subdirectory(zmq)
endif()
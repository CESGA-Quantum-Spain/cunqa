#pragma once

#include "strategy_def.h"

#if COMM_LIB == ASIO
    #include "strategies/asio/asio_client.hpp"
    using Future = AsioFuture;
#elif COMM_LIB == ZMQ
    using Future = std::future;
#elif COMM_LIB == CROW
    using Future = std::future;
#else
    #error "A valid library should be defined (ASIO, ZMQ o CROW) in COMM_LIB."
#endif

class FutureWrapper {
public:
    explicit FutureWrapper(Future f) : fut_(std::move(f)) {}

    // Obtener el resultado (bloquea hasta que esté disponible)
    std::string get() {
        return fut_.get();
    }

    // Comprobar si el futuro aún es válido
    bool valid() {
        return fut_.valid();
    }

private:
    Future fut_;
}; 

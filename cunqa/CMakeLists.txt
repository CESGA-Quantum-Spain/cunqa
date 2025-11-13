add_subdirectory(circuit)

pybind11_add_module(qclient bindings.cpp)

#find_library(GNUTLS_LIB gnutls REQUIRED PATHS /mnt/netapp1/Optcesga_FT2_RHEL7/2022/gentoo/31032022/usr/lib64/)
#find_library(SODIUM_LIB sodium REQUIRED PATHS /mnt/netapp1/Optcesga_FT2_RHEL7/2022/gentoo/31032022/usr/lib64/)
target_link_libraries(qclient PRIVATE client) #${GNUTLS_LIB} ${SODIUM_LIB} bsd)

install(TARGETS qclient DESTINATION cunqa)

file(GLOB PYTHON_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${PYTHON_SCRIPTS} DESTINATION cunqa)
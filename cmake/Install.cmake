include_guard(GLOBAL)

function(cunqa_setup_install_paths)
  include(GNUInstallDirs)

  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH "Install prefix" FORCE)
  endif()

  message(STATUS "Install location will be: ${CMAKE_INSTALL_PREFIX}")
  message(STATUS "Install bindir will be: ${CMAKE_INSTALL_BINDIR}")
  message(STATUS "Install libdir will be: ${CMAKE_INSTALL_LIBDIR}")

  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" PARENT_SCOPE)
endfunction()
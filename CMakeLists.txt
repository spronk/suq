# The CMake files here are provided mostly for developer convenience: they
# are not the preferred way of installing suq. Try autoconf.
    
cmake_minimum_required (VERSION 2.6)
project (suq)

add_subdirectory (src)


include (CPack)


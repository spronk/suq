# Make sure the compiler can find include files from our Hello library. 
#include_directories (.)

# Make sure the linker can find the Hello library once it is built. 
#link_directories (${2DHD_BINARY_DIR}/simt)

# Add executable called "helloDemo" that is built from the source files 
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found. 
add_executable (suq main.c err.c client_conn.c server.c job.c connection.c
                request.c request_process.c sig_handler.c settings.c
                wait.c usage.c)
               
# Link the executable to the Hello library. 
#target_link_libraries (2dhd  simt-cpp)

add_definitions(-DDATADIR="${CMAKE_INSTALL_PREFIX}/share")

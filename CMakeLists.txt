cmake_minimum_required(VERSION 4.0)
project(Proyectos_C C)

set(CMAKE_C_STANDARD 11)

add_executable(servidor socket_serv.c)
add_executable(cliente socket_client.c)

# Linkear la librería de Windows para sockets
target_link_libraries(servidor ws2_32)
target_link_libraries(cliente ws2_32)
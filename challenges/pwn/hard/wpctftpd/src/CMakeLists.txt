cmake_minimum_required(VERSION 3.10)
project(wpctftpd C)

set(CMAKE_C_STANDARD 11)

add_executable(wpctftpd main.c
        include/wp_pool.h
        wp_pool.c
        include/conf.h
        include/os.h
        include/wp_ftp_session.h
        include/wp_ftp_command.h
        wp_ftp_session.c
        wp_ftp_command.c)

add_executable(test_wp_pool
        test_wp_pool.c
        wp_pool.c
        include/wp_pool.h
        include/conf.h
        include/os.h)

find_library(CRYPT_LIB crypt)
if (CRYPT_LIB)
    target_link_libraries(wpctftpd ${CRYPT_LIB})
endif()

target_link_libraries(wpctftpd pthread)

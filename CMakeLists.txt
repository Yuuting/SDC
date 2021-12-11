cmake_minimum_required(VERSION 3.20)
project(distributed_cache)

set(CMAKE_CXX_STANDARD 14)
find_package (Threads REQUIRED)

add_compile_options(-D_LOG_ALL)

add_executable(cache log/colorlog.h common/utils.h config.h cache/lru_cache.h cache/cache_socket.h cache/cache.cpp mysql/locker.h mysql/sqlconn.h mysql/connectionRAII.h)
add_executable(client log/colorlog.h common/utils.h config.h client/client.h client/client.cpp)
add_executable(master master/master.h master/master.cpp)
add_executable(partLog partLog.cpp)
add_executable(mainLog mainLog.cpp)
add_executable(testpressure bench/testpressure.cpp)
add_executable(testMYSQL mysql/mysqlTest.cpp mysql/locker.h mysql/sqlconn.h mysql/connectionRAII.h)

target_link_libraries(cache  ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(cache mysqlclient)
target_link_libraries(testMYSQL  ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(testMYSQL mysqlclient)

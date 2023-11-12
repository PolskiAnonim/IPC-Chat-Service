cmake_minimum_required (VERSION 3.8)

set( CMAKE_C_COMPILER "/usr/bin/gcc" )

project ("serwer")

add_executable (serwer "serwer.c" "serwer.h")
add_executable(klient "klient.c" "klient.h")

if(EXISTS channels.cfg)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/channels.cfg
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
	 
if(EXISTS groups.cfg)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/groups.cfg
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET serwer PROPERTY C_STANDARD 11)
  set_property(TARGET klient PROPERTY C_STANDARD 11)
endif()
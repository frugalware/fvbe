cmake_minimum_required (VERSION 2.8)
project (fvbe)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DUI_NEWT")

ADD_SUBDIRECTORY(src)



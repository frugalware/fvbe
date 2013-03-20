set(FVBE_SOURCES
  block.c
  finale.c
  format.c
  greeter.c
  install.c
  layout.c
  locale.c
  local.h
  main.c
  partition.c
  postconfig.c
  preconfig.c
  raid.c
  skeleton.c
  ui_newt.c
  utility.c
)

find_file( blkid.h blkid/blkid.h )
if(NOT blkid.h)
  message (FATAL_ERROR "ERROR: could not find blkid.h header file")
endif(NOT blkid.h)
message( STATUS "Found blkid.h : " ${blkid.h} )
get_filename_component(BLKID_INCLUDE ${blkid.h} PATH)
include_directories(${BLKID_INCLUDE})
find_library( blkid libblkid.so)
if(NOT blkid)
  message (FATAL_ERROR "ERROR: could not find libblkid.so library")
endif(NOT blkid)
message( STATUS "Found libblkid.so  : " ${blkid} )

find_file( newt.h newt.h )
if(NOT newt.h)
  message (FATAL_ERROR "ERROR: could not find newt.h header file")
endif(NOT newt.h)
message( STATUS "Found newt.h : " ${newt.h} )
find_library( newt libnewt.so)
if(NOT newt)
  message (FATAL_ERROR "ERROR: could not find libnewt.so library")
endif(NOT newt)
message( STATUS "Found libnewt.so  : " ${newt} )

find_library( pacman libpacman.so)
if(NOT pacman)
  message (FATAL_ERROR "ERROR: could not find libpacman.so library")
endif(NOT pacman)
message( STATUS "Found libpacman.so  : " ${pacman} )

add_executable(${CMAKE_PROJECT_NAME} ${FVBE_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${blkid} ${newt} ${pacman})

install(TARGETS "${CMAKE_PROJECT_NAME}"  DESTINATION bin)
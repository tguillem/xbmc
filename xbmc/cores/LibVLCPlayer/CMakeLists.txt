if(LIBVLC_FOUND)
  set(SOURCES LibVLCPlayer.cpp)

  set(HEADERS LibVLCPlayer.h)
  core_add_library(libvlc-player)
  target_include_directories(libvlc-player
                             PRIVATE ${LIBVLC_INCLUDE_DIRS} ${LIBVLCPP_INCLUDE_DIRS})
endif()

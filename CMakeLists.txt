cmake_minimum_required( VERSION 2.8 )
project( rmanPtcSop )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/config/cmake )

# find houdini components
find_package( Houdini REQUIRED )

# try to find a rman lib (set by the RMAN envvar)
set( RMAN "3Delight" )
if ( $ENV{RMAN} MATCHES "" )
  set( RMAN $ENV{RMAN} )
endif ( $ENV{RMAN} MATCHES "" )
find_package( ${RMAN} REQUIRED )

# this stops the HDK causing a backwards include warning
add_definitions( -D_BACKWARD_BACKWARD_WARNING_H )

include_directories(
  ${Houdini_INCLUDE_DIR}
  ${${RMAN}_INCLUDE_DIR}
  )

add_library( rman_ptc_sop
  SHARED
  ${CMAKE_SOURCE_DIR}/src/SOP_rmanPtc.cpp
  ${CMAKE_SOURCE_DIR}/src/GR_rmanPtc.cpp
  )

set_target_properties( rman_ptc_sop
  PROPERTIES
  OUTPUT_NAME "rmanPtcSop"
  PREFIX ""
  COMPILE_FLAGS "-g ${Houdini_COMPILE_FLAGS}"
  LINK_FLAGS "${Houdini_LINK_FLAGS}"
  )

target_link_libraries( rman_ptc_sop
  ${Houdini_LIBRARIES}
  ${${RMAN}_LIBRARIES}
  )

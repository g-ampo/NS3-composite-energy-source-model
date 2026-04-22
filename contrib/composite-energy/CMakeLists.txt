# CMakeLists.txt for ns-3 contrib module: composite-energy
#
# Drop this directory into <ns-3-dev>/contrib/ and re-run:
#   ./ns3 configure --enable-examples --enable-tests
#   ./ns3 build
#
# Supported on ns-3 >= 3.36 (CMake build). For ns-3 <= 3.35 use wscript.

build_lib(
  LIBNAME composite-energy
  SOURCE_FILES
    model/composite-energy-source.cc
  HEADER_FILES
    model/composite-energy-source.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libenergy}
  TEST_SOURCES
    test/composite-energy-source-test-suite.cc
)

if(${ENABLE_EXAMPLES})
  add_subdirectory(examples)
endif()

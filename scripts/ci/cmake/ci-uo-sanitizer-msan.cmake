# Client maintainer: chuck.atkins@kitware.com

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

CMAKE_TOOLCHAIN_FILE:FILEPATH=/opt/msan/toolchain.cmake
ADIOS2_USE_Fortran:STRING=OFF
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF

HDF5_C_COMPILER_EXECUTABLE:FILEPATH=/opt/msan/bin/h5cc
HDF5_DIFF_EXECUTABLE:FILEPATH=/opt/msan/bin/h5diff
")

set(dashboard_track "Analysis")
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_MEMORYCHECK_TYPE "MemorySanitizer")

list(APPEND EXCLUDE_EXPRESSIONS
  "Engine.BP.BPBufferSizeTest.SyncDeferredIdenticalUsage.BP3.Serial"
  "Engine.BP.BPBufferSizeTest.SyncDeferredIdenticalUsage.BP4.Serial"
  )
list(JOIN EXCLUDE_EXPRESSIONS "|" TEST_EXCLUDE_STRING)
set(CTEST_MEMCHECK_ARGS EXCLUDE "${TEST_EXCLUDE_STRING}")

set(ADIOS_TEST_REPEAT 0)
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)

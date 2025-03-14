set(mpi_sources)
set(mpi_headers)
set(mpi_libraries)

if(${ENABLE_MPI})
  set(mpi_sources
      model/point-to-point-remote-channel.cc
  )
  set(mpi_headers
      model/point-to-point-remote-channel.h
  )
  set(mpi_libraries
      ${libmpi}
      ${MPI_CXX_LIBRARIES}
  )
endif()

build_lib(
  LIBNAME point-to-point
  SOURCE_FILES
    ${mpi_sources}
    helper/point-to-point-helper.cc
    model/point-to-point-channel.cc
    model/point-to-point-net-device.cc
    model/ppp-header.cc
    model/mpls-header.cc
    model/compress-ipv4-header.cc
    model/compress-ipv6-header.cc
    model/port-header.cc
    model/command-header.cc
    model/point-to-point-queue.cc
    model/switch-node.cc
    model/nic-node.cc
    model/control-node.cc
  HEADER_FILES
    ${mpi_headers}
    helper/point-to-point-helper.h
    model/point-to-point-channel.h
    model/point-to-point-net-device.h
    model/ppp-header.h
    model/mpls-header.h
    model/compress-ipv4-header.h
    model/compress-ipv6-header.h
    model/port-header.h
    model/command-header.h
    model/point-to-point-queue.h
    model/switch-node.h
    model/nic-node.h
    model/control-node.h
  LIBRARIES_TO_LINK ${libnetwork}
                    ${mpi_libraries}
  TEST_SOURCES test/point-to-point-test.cc
)

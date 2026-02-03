

list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Library/OpenclIface )

set( OPENCLIFACE_HEADERS
    OpenclIface.hpp
)

set( TARGET_LIBRARIES OpenCL )
if( DEFINED QC_CL_EXTRA_LIBS )
    list( APPEND TARGET_LIBRARIES ${QC_CL_EXTRA_LIBS} )
endif()

add_library(OpenclIface SHARED OpenclIface.cpp )
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
target_compile_definitions(OpenclIface PUBLIC CL_TARGET_OPENCL_VERSION=200)
else()
target_compile_definitions(OpenclIface PUBLIC CL_TARGET_OPENCL_VERSION=300)
endif()
target_include_directories(OpenclIface PUBLIC ${HEADERS_DIR})
target_link_libraries(OpenclIface PUBLIC ${TARGET_LIBRARIES})
target_compile_options( OpenclIface PRIVATE -Wall )
add_library(Ride::Hal::Libs::OpenclIface ALIAS OpenclIface)
install(TARGETS OpenclIface DESTINATION lib)

install( FILES ${OPENCLIFACE_HEADERS} DESTINATION include/QC/Library/OpenclIface )

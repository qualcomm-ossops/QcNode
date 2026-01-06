set( CMAKE_SYSTEM_NAME QNX )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_ntoaarch64le )

set( CMAKE_C_COMPILER aarch64-unknown-nto-qnx7.1.0-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-unknown-nto-qnx7.1.0-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{QNX_TARGET}/aarch64le/ )

# common header files and libraries
link_libraries( libstd slog2 socket )

# pmem
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/AMSS/inc )
include_directories( $ENV{BSP_ROOT}/install/usr/include )
endif()
link_libraries( pmem_client pmemext fastrpc_pmem mmap_peer OSAbstraction )

# apdf
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/apdf/ )
include_directories( $ENV{BSP_ROOT}/install/usr/include/WF )
include_directories( $ENV{QNX_ROOT}_patches/target/qnx7/usr/include )
include_directories( $ENV{QNX_ROOT}_patches/target/qnx7/usr/include/WF )
endif()
link_libraries( apdf aosal )

# c2d
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/graphics/include/private/C2D/ )
include_directories( $ENV{BSP_ROOT}/AMSS/inc/graphics/include/private/C2D/ )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/usr/lib/graphics/qc/" )
add_link_options( "-L$ENV{BSP_ROOT}/install_remote/aarch64le/lib" )
endif()
set( QC_C2D_LIBS c2d30 OSUser GSLUser )
link_libraries( planedef )

# rsm_v2
if( ENABLE_RSM_V2 )
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss )
endif()
add_compile_definitions( WITH_RSM_V2 )
link_libraries( rsm_client )
endif()

# vidc
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/inc/ )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/video/source/common/drivers/inc/ )
endif()
set( QC_VIDC_LIBS ioctlClient )
set( QC_VIDC_FILEDEMUX_LIBS
    FileDemux_Common FileSource dal
)

# qcarcam
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/qcamera/camera_qcx/cdk_qcx/api/qcarcam/ )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/lib/camera_qcx/" )
endif()
link_libraries( xml2 )

# gtest
include_directories( $ENV{QCNODE_INSTALL_DIR}/opt/qcnode/include )
add_link_options( "-L$ENV{QCNODE_INSTALL_DIR}/opt/qcnode/lib" )
link_libraries( gtest regex )

# fadas
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/fadas/fadas/inc/ )
include_directories( $ENV{BSP_ROOT}/AMSS/platform/qal/clients/fastrpc_lib/inc )
include_directories( $ENV{BSP_ROOT}/install_remote/usr/include/amss/multimedia/fadas/ )
endif()
link_libraries( safe_xml )

# OpenCL
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/graphics/include/public )
include_directories( $ENV{BSP_ROOT}/AMSS/inc/graphics/include/public )
endif()
add_compile_definitions( CL_TARGET_OPENCL_VERSION=300 )
link_libraries( OSUser GSLUser )

# eva
if( DEFINED ENV{BSP_ROOT} )
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/eva )
include_directories( $ENV{BSP_ROOT}/install_remote/usr/include/amss/multimedia/eva/ )
endif()
link_libraries( devioClient cdsprpc smmu_client npa_client clock_client icb_client )



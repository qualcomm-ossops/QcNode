// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "QC/Node/Camera.hpp"
#include "QC/sample/SharedBufferPool.hpp"
#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Node;
using namespace QC::sample;

QC::Node::Camera g_camera;
uint32_t g_frameIdx = 0;
std::vector<SharedBufferPool> g_bufferPools;

void ReadJsonFile( const std::string &filePath, nlohmann::json &jsonData )
{
    std::ifstream file( filePath );
    if ( file.is_open() )
    {
        file >> jsonData;
        file.close();
    }
    else
    {
        QC_LOG_ERROR( "Failed to open file %s", filePath );
    }
}

void ProcessDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    QCFrameDescriptorNodeIfs &frameDescIfs = eventInfo.frameDesc;
    QCBufferDescriptorBase_t &bufDesc = frameDescIfs.GetBuffer( 0 );
    NodeFrameDescriptor frameDesc( 1 );

    const CameraFrameDescriptor_t *pCamFrameDesc =
            dynamic_cast<const CameraFrameDescriptor_t *>( &bufDesc );

    if ( QC_STATUS_OK == ret )
    {
        CameraFrameDescriptor_t camFrameDesc = *pCamFrameDesc;
        ret = frameDesc.SetBuffer( 0, camFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = g_camera.ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        std::cout << "Process Frame index: " << g_frameIdx << std::endl;
        g_frameIdx++;
    }
}

static void AllocateFrameBuffers( DataTree &staticCfg, QCNodeInit_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::vector<DataTree> streamConfigs;
    std::string name = staticCfg.Get<std::string>( "name", "" );

    QCNodeID_t nodeId;
    nodeId.name = name;
    nodeId.type = QC_NODE_TYPE_QCX;
    nodeId.id = staticCfg.Get<uint32_t>( "id", UINT32_MAX );
    ASSERT_EQ( nodeId.id, 0 );

    ret = staticCfg.Get( "streamConfigs", streamConfigs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    DataTree streamConfig;
    ImageProps_t imgProp;
    uint32_t streamId = 0;
    uint32_t bufferId = 0;
    uint32_t streamNum = streamConfigs.size();
    uint32_t bufferNum = 0;
    g_bufferPools.resize( streamNum );

    for ( uint32_t i = 0; i < streamNum; i++ )
    {
        std::string bufPoolName = name + std::to_string( i );
        streamConfig = streamConfigs[i];
        streamId = streamConfig.Get<uint32_t>( "streamId", UINT32_MAX );
        std::vector<uint32_t> bufferIds =
                streamConfig.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );
        bufferNum = bufferIds.size();
        imgProp.format = streamConfig.GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
        imgProp.width = streamConfig.Get<uint32_t>( "width", UINT32_MAX );
        imgProp.height = streamConfig.Get<uint32_t>( "height", UINT32_MAX );

        if ( ( QC_IMAGE_FORMAT_RGB888 == imgProp.format ) ||
             ( QC_IMAGE_FORMAT_BGR888 == imgProp.format ) )
        {
            imgProp.batchSize = 1;
            imgProp.stride[0] = QC_ALIGN_SIZE( imgProp.width * 3, 16 );
            imgProp.actualHeight[0] = imgProp.height;
            imgProp.numPlanes = 1;
            imgProp.planeBufSize[0] = 0;

            ret = g_bufferPools[i].Init( bufPoolName, nodeId, LOGGER_LEVEL_ERROR, bufferNum,
                                         imgProp );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        else
        {
            ret = g_bufferPools[i].Init( bufPoolName, nodeId, LOGGER_LEVEL_ERROR, bufferNum,
                                         imgProp.width, imgProp.height, imgProp.format );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        ret = g_bufferPools[i].GetBuffers( config.buffers );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

void DeinitBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;
    for ( uint32_t i = 0; i < g_bufferPools.size(); i++ )
    {
        ret = g_bufferPools[i].Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

void SANITY_Test( DataTree &dt )
{
    QCStatus_e ret;
    DataTree staticCfg;
    std::vector<DataTree> streamConfigs;
    QCNodeInit_t config;

    config = { dt.Dump() };
    std::cout << "config: " << config.config << std::endl;

    config.callback = ProcessDoneCb;

    ret = dt.Get( "static", staticCfg );
    ASSERT_EQ( QC_STATUS_OK, ret );

    std::string name = staticCfg.Get<std::string>( "name", "" );

    AllocateFrameBuffers( staticCfg, config );

    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = g_camera.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    sleep( 1 );

    ret = g_camera.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = g_camera.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    DeinitBuffers();
}

void Set_U32_StaticConfig( DataTree &srcCfg, DataTree &dstCfg, QCNodeInit_t &config,
                           const std::string &key, uint32_t val )
{
    dstCfg.Set( "static", srcCfg );
    dstCfg.Set<uint32_t>( key, val );
    config = { dstCfg.Dump() };
}

void Exception_Test_EmptyConfig( DataTree &dt )
{
    QCStatus_e ret;
    DataTree staticCfg;
    DataTree errorCfg;
    std::vector<DataTree> streamConfigs;
    QCNodeInit_t config;

    ret = dt.Get( "static", staticCfg );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // empty name
    errorCfg.Set( "static", staticCfg );
    errorCfg.Set<std::string>( "static.name", "" );
    config = { errorCfg.Dump() };

    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty nodeId
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.id", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty inputId
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.inputId", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty srcId
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.srcId", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty clientId
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.clientId", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty inputMode
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.inputMode", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty ispUseCase
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.ispUseCase", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty camFrameDropPattern
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.camFrameDropPattern", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty camFrameDropPattern
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.camFrameDropPeriod", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // empty opMode
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.opMode", UINT32_MAX );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
}

void Exception_Test_ErrorConfig( DataTree &dt )
{
    QCStatus_e ret;
    DataTree staticCfg;
    DataTree errorCfg;
    std::vector<DataTree> streamConfigs;
    QCNodeInit_t config;

    ret = dt.Get( "static", staticCfg );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // error inputId
    Set_U32_StaticConfig( staticCfg, errorCfg, config, "static.inputId", 20 );
    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
}

TEST( Camera, SANITY_Test_IMX728_RequestMode )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config_imx728_request.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    SANITY_Test( dt );
}

TEST( Camera, SANITY_Test_OV3F_RequestMode )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    std::vector<DataTree> streamConfigs;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config_ov3f_request.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    SANITY_Test( dt );
}

TEST( Camera, SANITY_Test_IMX728_ReleaseMode )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config_imx728_release.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    SANITY_Test( dt );
}

TEST( Camera, SANITY_Test_OV3F_ReleaseMode )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    std::vector<DataTree> streamConfigs;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config_ov3f_release.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    SANITY_Test( dt );
}


TEST( Camera, SANITY_Test_IMX728_MultiStream )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    std::vector<DataTree> streamConfigs;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config_imx728_2stream.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    SANITY_Test( dt );
}

TEST( Camera, EXCEPTION_Test_EmptyConfig )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config_imx728_request.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    Exception_Test_EmptyConfig( dt );
}

TEST( Camera, EXCEPTION_Test_ErrorConfig )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config_imx728_request.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    Exception_Test_ErrorConfig( dt );
}

#ifndef GTEST_QCNODE
#if __CTC__
extern "C" void ctc_append_all( void );
#endif
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
#if __CTC__
    ctc_append_all();
#endif
    return nVal;
}
#endif

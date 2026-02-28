// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/SampleComputeLidarCoord.hpp"
#include <algorithm>
#include <assert.h>

namespace QC
{
namespace sample
{

SampleComputeLidarCoord::SampleComputeLidarCoord() {}
SampleComputeLidarCoord::~SampleComputeLidarCoord() {}

QCStatus_e SampleComputeLidarCoord::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferCache = QC_CACHEABLE_NON;
    }
    else
    {
        m_bufferCache = QC_CACHEABLE;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_cols = Get( config, "cols", 1000 );
    m_blocks = Get( config, "blocks", 100 );

    return ret;
}

QCStatus_e SampleComputeLidarCoord::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( CPU );
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    TensorProps_t outputTensorProp;
    outputTensorProp.tensorType = QC_TENSOR_TYPE_FLOAT_32;
    outputTensorProp.numDims = 2;
    outputTensorProp.dims[0] = m_blocks * m_cols;
    outputTensorProp.dims[1] = 4;
    outputTensorProp.allocatorType = QC_MEMORY_ALLOCATOR_DMA_GPU;
    outputTensorProp.cache = m_bufferCache;
    ret = m_outputBufferPool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize,
                                   outputTensorProp );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to init buffer pool for output buffer" );
    }

    return ret;
}

QCStatus_e SampleComputeLidarCoord::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = false;
    m_thread = std::thread( &SampleComputeLidarCoord::ThreadMain, this );

    return ret;
}

void SampleComputeLidarCoord::ThreadMain()
{
    QCStatus_e ret;

    while ( false == m_stop )
    {
        DataFrames_t tensors;
        ret = m_sub.Receive( tensors );
        if ( QC_STATUS_OK == ret )
        {
            PROFILER_BEGIN();
            TRACE_BEGIN( tensors.FrameId( 0 ) );
            ret = ComputeLidarCoordCPU( tensors );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "failed to compute lidar coord" );
                break;
            }
            PROFILER_END();
            TRACE_END( tensors.FrameId( 0 ) );
        }
    }
}

QCStatus_e SampleComputeLidarCoord::ComputeLidarCoordCPU( DataFrames_t &tensors )
{
    QCStatus_e ret = QC_STATUS_OK;
    QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", tensors.FrameId( 0 ),
              tensors.Timestamp( 0 ) );

    QCBufferDescriptorBase_t &rawDesc = tensors.GetBuffer( 0 );
    QCBufferDescriptorBase_t &firetimeDesc = tensors.GetBuffer( 1 );
    QCBufferDescriptorBase_t &correctionDesc = tensors.GetBuffer( 2 );

    TensorDescriptor_t *pRawDesc = dynamic_cast<TensorDescriptor_t *>( &rawDesc );
    if ( nullptr == pRawDesc )
    {
        ret = QC_STATUS_NULL_PTR;
        QC_ERROR( "raw data is not a valid tensor" );
    }
    else if ( m_cols != static_cast<int>( pRawDesc->dims[0] ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "raw data cols not match with config" );
    }
    else if ( m_blocks * 2 + 2 != static_cast<int>( pRawDesc->dims[1] ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "raw data blocks not match with config" );
    }


    TensorDescriptor_t *pFiretimeDesc = dynamic_cast<TensorDescriptor_t *>( &firetimeDesc );
    if ( nullptr == pFiretimeDesc )
    {
        ret = QC_STATUS_NULL_PTR;
        QC_ERROR( "firetime is not a valid tensor" );
    }

    TensorDescriptor_t *pCorrectionDesc = dynamic_cast<TensorDescriptor_t *>( &correctionDesc );
    if ( nullptr == pCorrectionDesc )
    {
        ret = QC_STATUS_NULL_PTR;
        QC_ERROR( "correction data is not a valid tensor" );
    }


    std::shared_ptr<SharedBuffer_t> pOutputBuffer = m_outputBufferPool.Get();
    if ( nullptr == pOutputBuffer )
    {
        ret = QC_STATUS_NULL_PTR;
        QC_ERROR( "failed to get output buffer from pool" );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCBufferDescriptorBase_t &bufDesc = pOutputBuffer->buffer;
        TensorDescriptor_t *pTensor = dynamic_cast<TensorDescriptor_t *>( &bufDesc );
        if ( nullptr == pTensor )
        {
            ret = QC_STATUS_NULL_PTR;
            QC_ERROR( "Not a valid tensor descriptor" );
        }
        else
        {
            float *raw = reinterpret_cast<float *>( pRawDesc->pBuf );
            float *firetime = reinterpret_cast<float *>( pFiretimeDesc->pBuf );
            float *correction = reinterpret_cast<float *>( pCorrectionDesc->pBuf );

            // Pre-calculate constants to avoid repeated computations
            const float degToRad = M_PI / 180.0f;
            const float inv256 = 1.0f / 256.0f;
            const float inv1e9 = 1.0f / 1e9f;
            const float radToDeg = 180.0f / M_PI;

            // Process multiple elements at once when possible
            int i = 0;

            // Vectorized processing for better cache utilization
            for ( ; i <= m_cols - 4; i += 4 )
            {
                // Load 4 sets of raw data simultaneously
                const int baseIdx = i * ( m_blocks * 2 + 2 );

                // Process 4 columns in parallel
                for ( int j = 0; j < m_blocks; j++ )
                {
                    // Load distance and intensity for 4 columns at once
                    const float *rawBase = &raw[baseIdx + j * 2];

                    // Process 4 elements in parallel using NEON
                    // Load 4 distances and 4 intensities
                    float32x4_t distances = vld1q_f32( &rawBase[0] );
                    float32x4_t intensities = vld1q_f32( &rawBase[1] );

                    // Load azimuth and motor values (same for all 4 columns)
                    const float azimuth = raw[baseIdx + m_blocks * 2];
                    const float motor = raw[baseIdx + m_blocks * 2 + 1];

                    // Load correction values for current block (j)
                    const int16_t correctionVal = correction[j];
                    const float elevation = (float) correctionVal * inv256 * degToRad;

                    // Calculate azimuth for all 4 columns
                    const float azimuthBase =
                            azimuth * inv256 + firetime[j] * motor * inv1e9 / 8.0f;
                    const float azimuthDeg = azimuthBase * radToDeg;
                    const float azimuthRad = azimuthDeg * degToRad;

                    // Pre-compute trigonometric values
                    const float cosElevation = std::cos( elevation );
                    const float sinElevation = std::sin( elevation );
                    const float sinAzimuth = std::sin( azimuthRad );
                    const float cosAzimuth = std::cos( azimuthRad );

                    // Vectorized coordinate calculations
                    float32x4_t xyDistances = vmulq_n_f32( distances, cosElevation );
                    float32x4_t xValues = vmulq_n_f32( xyDistances, sinAzimuth );
                    float32x4_t yValues = vmulq_n_f32( xyDistances, cosAzimuth );
                    float32x4_t zValues = vmulq_n_f32( distances, sinElevation );

                    // Store results for 4 columns - using direct array access to avoid lane
                    // extraction issues
                    float *pPoints = reinterpret_cast<float *>( pTensor->pBuf );

                    // Extract values using temporary arrays to avoid vgetq_lane_f32 issues
                    float xVals[4], yVals[4], zVals[4], intensityVals[4];

                    // Manually extract each element (this avoids the compile-time constant
                    // requirement)
                    xVals[0] = vgetq_lane_f32( xValues, 0 );
                    xVals[1] = vgetq_lane_f32( xValues, 1 );
                    xVals[2] = vgetq_lane_f32( xValues, 2 );
                    xVals[3] = vgetq_lane_f32( xValues, 3 );

                    yVals[0] = vgetq_lane_f32( yValues, 0 );
                    yVals[1] = vgetq_lane_f32( yValues, 1 );
                    yVals[2] = vgetq_lane_f32( yValues, 2 );
                    yVals[3] = vgetq_lane_f32( yValues, 3 );

                    zVals[0] = vgetq_lane_f32( zValues, 0 );
                    zVals[1] = vgetq_lane_f32( zValues, 1 );
                    zVals[2] = vgetq_lane_f32( zValues, 2 );
                    zVals[3] = vgetq_lane_f32( zValues, 3 );

                    intensityVals[0] = vgetq_lane_f32( intensities, 0 );
                    intensityVals[1] = vgetq_lane_f32( intensities, 1 );
                    intensityVals[2] = vgetq_lane_f32( intensities, 2 );
                    intensityVals[3] = vgetq_lane_f32( intensities, 3 );

                    // Store results for 4 columns
                    for ( int k = 0; k < 4; k++ )
                    {
                        const int idx = ( i + k ) * m_blocks + j * 4;
                        pPoints[idx + 0] = xVals[k];
                        pPoints[idx + 1] = yVals[k];
                        pPoints[idx + 2] = zVals[k];
                        pPoints[idx + 3] = intensityVals[k];
                    }
                }
            }

            // Handle remaining columns (non-vectorizable part)
            for ( ; i < m_cols; i++ )
            {
                const int baseIdx = i * ( m_blocks * 2 + 2 );

                for ( int j = 0; j < m_blocks; j++ )
                {
                    const float distance = raw[baseIdx + j * 2];
                    const float azimuth = raw[baseIdx + m_blocks * 2];
                    const float motor = raw[baseIdx + m_blocks * 2 + 1];
                    const float fireTime = firetime[j];
                    const float elevation = (float) ( correction[j] ) * inv256 * degToRad;
                    const float azimuthDeg =
                            ( azimuth * inv256 + fireTime * motor * inv1e9 / 8.0f ) * radToDeg;
                    const float azimuthRad = azimuthDeg * degToRad;

                    float xyDistance = distance * std::cos( elevation );
                    float x = xyDistance * std::sin( azimuthRad );
                    float y = xyDistance * std::cos( azimuthRad );
                    float z = distance * std::sin( elevation );
                    float intensity = raw[baseIdx + j * 2 + 1];

                    float *pPoints = reinterpret_cast<float *>( pTensor->pBuf );
                    pPoints[i * m_blocks + j * 4 + 0] = x;
                    pPoints[i * m_blocks + j * 4 + 1] = y;
                    pPoints[i * m_blocks + j * 4 + 2] = z;
                    pPoints[i * m_blocks + j * 4 + 3] = intensity;
                }
            }
        }
    }

    DataFrames_t outFrames;
    DataFrame_t frame;
    frame.buffer = pOutputBuffer;
    frame.frameId = tensors.FrameId( 0 );
    frame.timestamp = tensors.Timestamp( 0 );
    outFrames.Add( frame );
    m_pub.Publish( outFrames );

    return ret;
}

QCStatus_e SampleComputeLidarCoord::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleComputeLidarCoord::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr != m_pBufMgr )
    {
        BufferManager::Put( m_pBufMgr );
        m_pBufMgr = nullptr;
    }
    return ret;
}

REGISTER_SAMPLE( ComputeLidarCoord, SampleComputeLidarCoord );

}   // namespace sample
}   // namespace QC

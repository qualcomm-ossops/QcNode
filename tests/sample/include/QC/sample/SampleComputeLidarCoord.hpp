// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_SAMPLE_COMPUTE_LIDAR_COORD_HPP
#define QC_SAMPLE_COMPUTE_LIDAR_COORD_HPP

#include "QC/sample/SampleIF.hpp"
#include <arm_neon.h>

using namespace QC;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleComputeLidarCoord
///
/// SampleComputeLidarCoord do lidar raw data preprocessing
class SampleComputeLidarCoord : public SampleIF
{
public:
    SampleComputeLidarCoord();
    ~SampleComputeLidarCoord();

    /// @brief Initialize the ComputeLidarCoord
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the ComputeLidarCoord
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the ComputeLidarCoord
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the ComputeLidarCoord
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    QCStatus_e ComputeLidarCoordCPU( DataFrames_t &inputsFrame );

private:
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    bool m_stop;

    uint32_t m_blocks = 100;
    uint32_t m_cols = 1000;

    uint32_t m_poolSize = 4;
    QCAllocationCache_e m_bufferCache = QC_CACHEABLE;
    SharedBufferPool m_outputBufferPool;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    BufferManager *m_pBufMgr = nullptr;
};   // class SampleComputeLidarCoord

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_COMPUTE_LIDAR_COORD_HPP

// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_CAMERA_BUFFER_DESCRIPTOR_HPP
#define QC_CAMERA_BUFFER_DESCRIPTOR_HPP

#include "QC/Infras/Memory/ImageDescriptor.hpp"

namespace QC
{
namespace Memory
{

/** @brief The maximum input/output stream number of QCNode Camera */
#define QCNODE_CAMERA_MAX_INPUT_STREAM_NUM 4U
#define QCNODE_CAMERA_MAX_STREAM_NUM 32U

/** @brief The maximum buffer number for each stream of QCNode Camera */
#define QCNODE_CAMERA_MAX_BUFFER_NUM 20U

/**
 * @brief Camera buffer identification for each request
 * @param bufferListId      The index of buffer group for each request
 * @param bufferIds         The indices of buffers for each request
 */
typedef struct
{
    uint32_t bufferListId = 0;
    uint32_t bufferIdx = 0;
} CameraBufferRequest_t;

/**
 * @brief Descriptor for QCNode Shared Camera Frame Descriptor.
 * This structure represents the camera frame descriptor for QCNode. It
 * extends the ImageDescriptor and includes additional members specific
 * to camera frames.
 *
 * Inherited Members from ImageDescriptor:
 * @param name The name of the buffer.
 * @param pBuf The virtual address of the dma buffer.
 * @param size The dma size of the buffer.
 * @param type The type of the buffer.
 * @param alignment The alignment of the buffer.
 * @param cache The cache type of the buffer.
 * @param allocatorType The allocaor type used for allocation the buffer.
 * @param dmaHandle The dmaHandle of the buffer.
 * @param pid The process ID of the buffer.
 * @param validSize The size of valid data currently stored in the buffer.
 * @param offset The offset of the valid buffer within the shared buffer.
 * @param id A identifier assigned by the user application to distinguish the buffer.
 * @param format The image format.
 * @param batchSize The image batch size.
 * @param width The image width in pixels.
 * @param height The image height in pixels.
 * @param stride The image stride along the width in bytes for each plane.
 * @param actualHeight The actual height of the image in scanlines for each plane.
 * @param planeBufSize The actual buffer size of the image for each plane, calculated as (stride *
 * actualHeight + padding size).
 * @param numPlanes The number of image planes.
 *
 * New Members:
 * @param timestamp The hardware timestamp of the frame in nanoseconds.
 * @param timestampQGPTP The Generic Precision Time Protocol (GPTP) timestamp in nanoseconds.
 * @param frameIdx The index of the camera frame.
 * @param flags Indicating the error state of the buffer.
 * @param streamId The identifier for the Qcarcam buffer list.
 */
typedef struct CameraFrameDescriptor : public ImageDescriptor
{
public:
    CameraFrameDescriptor() : ImageDescriptor() {}

    /**
     * @brief Sets up the camera frame descriptor from another buffer descriptor base object.
     * @param[in] other The camera frame descriptor object from which buffer members are copied.
     * @return The updated camera frame descriptor object.
     */
    CameraFrameDescriptor &operator=( const QCBufferDescriptorBase &other );

    uint64_t timestamp;
    uint64_t timestampQGPTP;
    uint32_t frameIdx;
    uint32_t flags;
    uint32_t streamId;
} CameraFrameDescriptor_t;


/**
 * @brief Descriptor for QCNode Shared Camera MetaData Descriptor.
 * This structure represents the camera metadata descriptor for QCNode. It
 * extends the ImageDescriptor and includes additional members specific
 * to camera frames.
 *
 * Inherited Members from ImageDescriptor:
 * @param name The name of the buffer.
 * @param pBuf The virtual address of the dma buffer.
 * @param size The dma size of the buffer.
 * @param type The type of the buffer.
 * @param alignment The alignment of the buffer.
 * @param cache The cache type of the buffer.
 * @param allocatorType The allocaor type used for allocation the buffer.
 * @param dmaHandle The dmaHandle of the buffer.
 * @param pid The process ID of the buffer.
 * @param validSize The size of valid data currently stored in the buffer.
 * @param offset The offset of the valid buffer within the shared buffer.
 * @param id A identifier assigned by the user application to distinguish the buffer.
 * @param format The image format.
 * @param batchSize The image batch size.
 * @param width The image width in pixels.
 * @param height The image height in pixels.
 * @param stride The image stride along the width in bytes for each plane.
 * @param actualHeight The actual height of the image in scanlines for each plane.
 * @param planeBufSize The actual buffer size of the image for each plane, calculated as (stride *
 * actualHeight + padding size).
 * @param numPlanes The number of image planes.
 *
 * New Members:
 * @param requestId The unique id of request in QCarCamera.
 * @param streamRequestNum The number of request for each stream.
 * @param syncId Used for request synchronization across multiple cameras that are frame synced.
 * @param flags Indicating the error state of the buffer.
 * @param inputBuffer Input buffer for injection usecase.
 * @param inputCommonMetadata Common input metadata to be applied to all inputs.
 * @param inputMetadata Input metadata for each of the individual inputs.
 * @param outputMetadata Output metadata for each of the individual inputs.
 * @param streamRequests Output buffer indices for each stream.
 */
typedef struct CameraMetaDataDescriptor : public ImageDescriptor
{
public:
    CameraMetaDataDescriptor() : ImageDescriptor() {}

    uint32_t requestId;
    uint32_t streamRequestNum;
    uint32_t syncId;
    uint32_t flags;

    CameraBufferRequest_t inputBuffer;
    CameraBufferRequest_t inputCommonMetadata;
    CameraBufferRequest_t inputMetadata[QCNODE_CAMERA_MAX_INPUT_STREAM_NUM];
    CameraBufferRequest_t outputMetadata[QCNODE_CAMERA_MAX_INPUT_STREAM_NUM];
    CameraBufferRequest_t streamRequests[QCNODE_CAMERA_MAX_STREAM_NUM];

} CameraMetaDataDescriptor_t;

}   // namespace Memory
}   // namespace QC

#endif   // QC_CAMERA_BUFFER_DESCRIPTOR_HPP

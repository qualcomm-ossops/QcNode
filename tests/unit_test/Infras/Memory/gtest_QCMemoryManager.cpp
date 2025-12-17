// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/ManagerLocal.hpp"
#include "QC/Infras/Memory/Pool.hpp"
#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Memory;


class FakeAllocator : public QCMemoryAllocatorIfs
{
public:
    FakeAllocator() : QCMemoryAllocatorIfs( { "Fake Allocator" }, QC_MEMORY_ALLOCATOR_HEAP ) {}
    ~FakeAllocator(){};

    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response )
    {
        response.size = request.size;
        response.allocatorType = QC_MEMORY_ALLOCATOR_HEAP;
        return QC_STATUS_OK;
    };
    virtual QCStatus_e Free( const QCBufferDescriptorBase_t &buff ) { return QC_STATUS_FAIL; };
};

class Test_QCMemorymanager : public testing::Test
{
protected:
    void SetUp() override
    {
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                allocators = { allocatorIfs1, allocatorIfs2, allocatorIfs1, allocatorIfs2,
                               allocatorIfs1, allocatorIfs2, allocatorIfs1 };

        Ifs = reinterpret_cast<QCMemoryManagerIfs *>( &mm );
        ASSERT_NE( nullptr, Ifs );

        QCMemoryManagerInit_t memorymanagerInit( 4, allocators );

        status = Ifs->Initialize( memorymanagerInit );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    void TearDown() override
    {
        status = Ifs->DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    QCStatus_e status;
    ManagerLocal mm;
    QCMemoryManagerIfs *Ifs;
    HeapAllocator allocatorIfs1;
    HeapAllocator allocatorIfs2;
};


TEST_F( Test_QCMemorymanager, SANITY_creation_and_destruction )
{
    {

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 3 };
        QCMemoryHandle_t handle1;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        QCNodeID_t nodeExtra = { "Test node", QC_NODE_TYPE_LAST, 1 };
        QCMemoryHandle_t handleExtra;
        handleExtra.SetNodeCount( 255 );
        handleExtra.SetNodeType( (QCNodeType_e) ( QC_NODE_TYPE_LAST + 1 ) );

        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        nodeExtra.type = QC_NODE_TYPE_QNN;
        nodeExtra.id = 5;
        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        nodeExtra.id = 4;
        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ManagerLocal instance;
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                allocators = { allocatorIfs1, allocatorIfs2, allocatorIfs1, allocatorIfs2,
                               allocatorIfs1, allocatorIfs2, allocatorIfs1 };

        QCMemoryManagerInit_t mmInit( 0, allocators );

        status = instance.Initialize( mmInit );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    }

    {
        ManagerLocal instance;

        status = instance.DeInitialize();
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );
    }

    {
        ManagerLocal *instance = new ManagerLocal();
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                allocators = { allocatorIfs1, allocatorIfs2, allocatorIfs1, allocatorIfs2,
                               allocatorIfs1, allocatorIfs2, allocatorIfs1 };

        QCMemoryManagerInit_t mmInit( 4, allocators );

        status = instance->Initialize( mmInit );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = instance->Initialize( mmInit );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        instance->~ManagerLocal();
    }
}


TEST_F( Test_QCMemorymanager, SANITY_creation_and_registration )
{
    {

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 2, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 3, handle3.GetNodeCount() );

        QCMemoryHandle_t handleDummy;
        status = Ifs->Register( node3, handleDummy );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 4, handle4.GetNodeCount() );

        QCNodeID_t nodeExtra = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handleExtra;

        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;
        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_basic_stand_alone_buffer_allocation )
{
    {

        QCBufferPropBase_t request[4];
        request[0].size = 10;
        request[0].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[0].cache = QC_CACHEABLE;

        request[1].size = 20;
        request[1].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[1].cache = QC_CACHEABLE;

        request[2].size = 1024 * 2;
        request[2].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[2].cache = QC_CACHEABLE;

        request[3].size = 1024 * 10;
        request[3].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[3].cache = QC_CACHEABLE;

        QCBufferDescriptorBase_t response[4];

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 2, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 3, handle3.GetNodeCount() );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 4, handle4.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle3, QC_MEMORY_ALLOCATOR_HEAP, request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle4, QC_MEMORY_ALLOCATOR_HEAP, request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle1, response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle3, response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle4, response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 1, handle4.GetNodeCount() );

        request[3].size = (size_t) -1;
        status = Ifs->AllocateBuffer( handle4, QC_MEMORY_ALLOCATOR_HEAP, request[3], response[3] );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }
}


TEST_F( Test_QCMemorymanager, SANITY_basic_stand_alone_buffer_allocation_2 )
{
    {
        QCBufferPropBase_t request;
        request.size = 0;
        request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request.cache = QC_CACHEABLE;

        QCBufferDescriptorBase_t response;

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        request.size = 10;

        QCMemoryHandle_t handle2;

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_LAST, request, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle2, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        response.allocatorType = QC_MEMORY_ALLOCATOR_LAST;
        status = Ifs->FreeBuffer( handle1, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_stand_alone_buffer_allocation )
{
    {

        QCBufferPropBase_t request[4];
        request[0].size = 10;
        request[0].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[0].cache = QC_CACHEABLE;

        request[1].size = 20;
        request[1].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[1].cache = QC_CACHEABLE;

        request[2].size = 1024 * 2;
        request[2].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[2].cache = QC_CACHEABLE;

        request[3].size = 1024 * 10;
        request[3].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[3].cache = QC_CACHEABLE;

        QCBufferDescriptorBase_t response[4];

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 2, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 3, handle3.GetNodeCount() );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 4, handle4.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle3, QC_MEMORY_ALLOCATOR_HEAP, request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle4, QC_MEMORY_ALLOCATOR_HEAP, request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        // incorrect free 1
        status = Ifs->FreeBuffer( handle1, response[3] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle2, response[2] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle3, response[1] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle4, response[0] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        // incorrect free 2
        QCBufferDescriptorBase_t responseTmp = response[0];
        response[0].pBuf = nullptr;
        status = Ifs->FreeBuffer( handle1, response[0] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        response[0] = responseTmp;

        responseTmp = response[1];
        response[1].pBuf = (void *) 0xFFFFFFFFFFFFFFFF;
        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        response[1] = responseTmp;

        // correct free
        status = Ifs->FreeBuffer( handle1, response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle3, response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle4, response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ManagerLocal instance;

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        status = instance.Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        QCBufferPropBase_t request;
        QCBufferDescriptorBase_t response;

        status = instance.AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.FreeBuffer( handle1, response );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_reclaim_resources )
{
    {

        QCBufferPropBase_t request[4];
        request[0].size = 10;
        request[0].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[0].cache = QC_CACHEABLE;

        request[1].size = 20;
        request[1].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[1].cache = QC_CACHEABLE;

        request[2].size = 1024 * 2;
        request[2].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[2].cache = QC_CACHEABLE;

        request[3].size = 1024 * 10;
        request[3].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[3].cache = QC_CACHEABLE;

        QCBufferDescriptorBase_t response[4];

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 2, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 3, handle3.GetNodeCount() );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 4, handle4.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle3, QC_MEMORY_ALLOCATOR_HEAP, request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle4, QC_MEMORY_ALLOCATOR_HEAP, request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCMemoryPoolConfig_t poolCfg( allocatorIfs1 );
        poolCfg.buff.size = 1024;
        poolCfg.buff.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        poolCfg.buff.cache = QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES;
        poolCfg.maxElements = 10;
        poolCfg.name = "test pool";

        QCMemoryPoolHandle_t poolHandle;

        status = Ifs->CreatePool( handle1, poolCfg, poolHandle );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCBufferDescriptorBase_t buff1;
        status = Ifs->AllocateBufferFromPool( poolHandle, buff1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCBufferDescriptorBase_t buff2;
        status = Ifs->AllocateBufferFromPool( poolHandle, buff2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCMemoryPoolHandle_t poolHandleDummy;
        QCBufferDescriptorBase_t buffDummy;
        status = Ifs->AllocateBufferFromPool( poolHandleDummy, buffDummy );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        poolHandleDummy.SetMemoryHandle( handle1 );
        status = Ifs->AllocateBufferFromPool( poolHandleDummy, buffDummy );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        QCMemoryHandle_t handle;
        status = Ifs->ReclaimResources( handle );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->ReclaimResources( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle3, response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle4, response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_pool_create_destroy )
{
    QCMemoryPoolConfig_t poolCfg( allocatorIfs1 );
    poolCfg.buff.size = 1024;
    poolCfg.buff.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    poolCfg.buff.cache = QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES;
    poolCfg.maxElements = 10;
    poolCfg.name = "test pool";

    QCMemoryPoolHandle_t poolHandle;

    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
    QCMemoryHandle_t handle1;

    status = Ifs->Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = Ifs->CreatePool( handle1, poolCfg, poolHandle );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCBufferDescriptorBase_t buff;
    status = Ifs->AllocateBufferFromPool( poolHandle, buff );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCMemoryPoolHandle_t poolHandleDummy;
    status = Ifs->AllocateBufferFromPool( poolHandleDummy, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->PutBufferToPool( poolHandle, buff );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = Ifs->PutBufferToPool( poolHandleDummy, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->DestroyPool( poolHandle );
    ASSERT_EQ( QC_STATUS_OK, status );

    poolCfg.buff.alignment = 0;
    status = Ifs->CreatePool( handle1, poolCfg, poolHandle );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    QCMemoryHandle_t handle1Dummy;
    QCMemoryPoolHandle_t poolHandle1Dummy;
    poolCfg.buff.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    status = Ifs->CreatePool( handle1Dummy, poolCfg, poolHandle );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    poolHandle1Dummy = poolHandle;
    poolHandle1Dummy.SetMemoryHandle( handle1Dummy );
    status = Ifs->AllocateBufferFromPool( poolHandle1Dummy, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    poolHandleDummy.SetMemoryHandle( handle1Dummy );
    status = Ifs->DestroyPool( poolHandleDummy );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    poolHandleDummy.SetMemoryHandle( handle1 );
    status = Ifs->DestroyPool( poolHandleDummy );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    poolHandle1Dummy = poolHandle;
    poolHandle1Dummy.SetMemoryHandle( handle1Dummy );
    status = Ifs->PutBufferToPool( poolHandle, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    {
        ManagerLocal instance;

        QCMemoryPoolHandle_t poolHandle1Dummy = poolHandle;
        poolHandle1Dummy.SetMemoryHandle( handle1Dummy );
        status = instance.DestroyPool( poolHandle1Dummy );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.CreatePool( handle1Dummy, poolCfg, poolHandle );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        poolHandle.SetMemoryHandle( handle1 );
        status = instance.AllocateBufferFromPool( poolHandle, buff );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        poolHandle.SetMemoryHandle( handle1Dummy );
        status = instance.PutBufferToPool( poolHandle, buff );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );
    }

    {
        FakeAllocator allocator;
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                fakeAllocators = { allocator, allocator, allocator, allocator,
                                   allocator, allocator, allocator };

        QCMemoryManagerInit_t mmInit( 4, fakeAllocators );

        ManagerLocal instance;
        status = instance.Initialize( mmInit );
        ASSERT_EQ( QC_STATUS_OK, status );

        poolCfg.maxElements = 0;

        status = instance.Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = instance.CreatePool( handle1, poolCfg, poolHandle );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_pool_create_destroy_max )
{
    QCMemoryPoolConfig_t poolCfg( allocatorIfs1 );
    poolCfg.buff.size = 16;
    poolCfg.buff.alignment = 16;
    poolCfg.buff.cache = QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES;
    poolCfg.maxElements = 1;
    poolCfg.name = "test pool";

    QCMemoryPoolHandle_t poolHandle[QC_MEMORY_MAX_POOLS_PER_NODE];

    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
    QCMemoryHandle_t handle1;

    status = Ifs->Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_OK, status );

    for ( uint32_t i = 0; i < ( QC_MEMORY_MAX_POOLS_PER_NODE ); i++ )
    {
        status = Ifs->CreatePool( handle1, poolCfg, poolHandle[i] );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    QCMemoryPoolHandle_t poolHandleLast;
    status = Ifs->CreatePool( handle1, poolCfg, poolHandleLast );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, status );
}

TEST_F( Test_QCMemorymanager, SANITY_registration_fail )
{
    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_MAX, 0 };
    QCMemoryHandle_t handle1;

    status = Ifs->Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    node1.type = QC_NODE_TYPE_RESERVED;

    status = Ifs->Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
}

TEST_F( Test_QCMemorymanager, SANITY_fake_allocator )
{
    FakeAllocator allocator;
    std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
            fakeAllocators = { allocator, allocator, allocator, allocator,
                               allocator, allocator, allocator };

    QCMemoryManagerInit_t mmInit( 4, fakeAllocators );

    ManagerLocal instance;
    status = instance.Initialize( mmInit );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
    QCMemoryHandle_t handle1;

    status = instance.Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCBufferPropBase_t request;
    request.size = 20;
    QCBufferDescriptorBase_t response;

    status = instance.AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = instance.FreeBuffer( handle1, response );
    ASSERT_EQ( QC_STATUS_FAIL, status );
}

TEST_F( Test_QCMemorymanager, SANITY_fake_allocator_reclaimresources )
{
    FakeAllocator allocator;
    std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
            fakeAllocators = { allocator, allocator, allocator, allocator,
                               allocator, allocator, allocator };

    QCMemoryManagerInit_t mmInit( 4, fakeAllocators );

    ManagerLocal instance;
    status = instance.Initialize( mmInit );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
    QCMemoryHandle_t handle1;

    status = instance.Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCBufferPropBase_t request;
    request.size = 20;
    QCBufferDescriptorBase_t response;

    status = instance.AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = instance.ReclaimResources( handle1 );
    ASSERT_EQ( QC_STATUS_FAIL, status );
}

TEST_F( Test_QCMemorymanager, SANITY_fake_allocator_reclaimresources_2 )
{
    FakeAllocator allocator;
    std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
            fakeAllocators = { allocator, allocator, allocator, allocator,
                               allocator, allocator, allocator };

    QCMemoryManagerInit_t mmInit( 4, fakeAllocators );

    ManagerLocal instance;
    status = instance.Initialize( mmInit );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
    QCMemoryHandle_t handle1;

    status = instance.Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCBufferPropBase_t request;
    request.size = 20;
    QCBufferDescriptorBase_t response;

    status = instance.AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = instance.DeInitialize();
    ASSERT_EQ( QC_STATUS_FAIL, status );
}

TEST_F( Test_QCMemorymanager, SANITY_fake_allocator_reclaimresources_3 )
{
    FakeAllocator allocator;
    std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
            fakeAllocators = { allocator, allocator, allocator, allocator,
                               allocator, allocator, allocator };

    QCMemoryManagerInit_t mmInit( 4, fakeAllocators );

    ManagerLocal instance;
    status = instance.Initialize( mmInit );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
    QCMemoryHandle_t handle1;

    status = instance.Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCBufferPropBase_t request;
    request.size = 20;
    QCBufferDescriptorBase_t response;

    status = instance.AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = instance.UnRegister( handle1 );
    ASSERT_EQ( QC_STATUS_FAIL, status );
}

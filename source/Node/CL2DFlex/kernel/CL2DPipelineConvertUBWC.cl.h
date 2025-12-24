// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_CL2D_PIPELINE_CONVERTUBWC_CLH
#define QC_CL2D_PIPELINE_CONVERTUBWC_CLH

KernelCode(

        __kernel void ConvertUBWC( __read_only image2d_t srcYPlane,
                                   __read_only image2d_t srcUVPlane, sampler_t sampler,
                                   __global uchar *dstPtr, uint dstOffset, uint inputHeight,
                                   uint inputWidth, uint resizeHeight, uint resizeWidth,
                                   uint inputStride0, uint inputPlane0Size, uint inputStride1,
                                   uint outputStride0, uint outputPlane0Size, uint outputStride1,
                                   uint roiX, uint roiY ) {
            const int x = get_global_id( 0 );
            const int y = get_global_id( 1 );

            int xIn1 = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn1 = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );

            const int2 coord1 = (int2) ( xIn1, yIn1 );
            const float4 pixelY = read_imagef( srcYPlane, sampler, coord1 );
            __global uchar *dst1 = dstPtr + dstOffset + mad24( y, (int) outputStride0, x );
            dst1[0] = convert_uchar_sat( pixelY.s0 * 255 );

            const int2 coord2 = (int2) ( xIn1 / 2, yIn1 / 2 );
            const float4 pixelUV = read_imagef( srcUVPlane, sampler, coord2 );
            __global uchar *dst2 = dstPtr + dstOffset + outputPlane0Size +
                                   mad24( y / 2, (int) outputStride1, ( x / 2 ) << 1 );
            dst2[0] = convert_uchar_sat( pixelUV.s0 * 255 );
            dst2[1] = convert_uchar_sat( pixelUV.s1 * 255 );
        }

)

#endif   // QC_CL2D_PIPELINE_CONVERTUBWC_CLH

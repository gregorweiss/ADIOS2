/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BigWhoopOperator.cpp
 *
 *  Created on: Aug 5, 2024
 *      Author: Gregor Weiss <gregor.weiss@hlrs.de>
 */

#include "BigWhoopOperator.h"

#include <fstream>
#include <string>

#include <bwc.h>

namespace adios2
{
namespace plugin
{

struct BigWhoopOperator::BWCImpl
{
    ~BWCImpl() {}
};

BigWhoopOperator::BigWhoopOperator(const Params &parameters)
: PluginOperatorInterface(parameters), Impl(new BWCImpl)
{}

BigWhoopOperator::~BigWhoopOperator() {}

size_t
BigWhoopOperator::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                            const DataType type, char *bufferOut)
{
    // offset for writing into bufferOut
    size_t offset = 0;

    // need to return the size of data in the buffer
    return offset;
}

size_t
BigWhoopOperator::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t offset = 0;

    // need to grab any parameter(s) we saved in Operate()
    const size_t dataBytes = GetParameter<size_t>(bufferIn, offset);

    // return the size of the data
    return dataBytes;
}

bool BigWhoopOperator::IsDataTypeValid(const DataType type) const { return true; }

size_t BigWhoopOperator::GetEstimatedSize(const size_t ElemCount, const size_t ElemSize,
                                            const size_t ndims, const size_t *dims) const
{
    size_t sizeIn = ElemCount * ElemSize;
    return sizeIn;
}
} // end namespace plugin
} // end namespace adios2

extern "C" {

adios2::plugin::BigWhoopOperator *OperatorCreate(const adios2::Params &parameters)
{
    return new adios2::plugin::BigWhoopOperator(parameters);
}

void OperatorDestroy(adios2::plugin::BigWhoopOperator *obj) { delete obj; }
}

/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BigWhoopOperator.h
 *
 *  Created on: Aug 5, 2024
 *      Author: Gregor Weiss <gregor.weiss@hlrs.de>
 */

#ifndef BIGWHOOPOPERATOR_H_
#define BIGWHOOPOPERATOR_H_

#include <memory>

#include "adios2/common/ADIOSTypes.h"
#include "adios2/operator/plugin/PluginOperatorInterface.h"

namespace adios2
{
namespace plugin
{

class BigWhoopOperator : public PluginOperatorInterface
{
public:
    BigWhoopOperator(const Params &parameters);
    virtual ~BigWhoopOperator();

    size_t Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                   const DataType type, char *bufferOut) override;

    size_t InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut) override;

    bool IsDataTypeValid(const DataType type) const override;

    size_t GetEstimatedSize(const size_t ElemCount, const size_t ElemSize, const size_t ndims,
                            const size_t *dims) const override;

private:
    struct BWCImpl;
    std::unique_ptr<BWCImpl> Impl;
};

} // end namespace plugin
} // end namespace adios2

extern "C" {

adios2::plugin::BigWhoopOperator *OperatorCreate(const adios2::Params &parameters);
void OperatorDestroy(adios2::plugin::BigWhoopOperator *obj);
}

#endif /* BIGWHOOPOPERATOR_H_ */

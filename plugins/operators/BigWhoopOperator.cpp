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

#include "adios2/helper/adiosFunctions.h"

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

Dims ConvertBwcDims(const Dims &dimensions, const DataType type, const size_t targetDims,
                    const bool enforceDims, const size_t defaultDimSize);

/**
 * Returns supported bwc_precision based on adios string type
 * @param type adios type as string, see GetDataType<T> in
 * helper/adiosType.inl
 * @return bwc_precision
 */
bwc_precision GetBwcPrecision(DataType type);

/**
 * Construct bwc_stream based on input information around the data pointer
 * @param data
 * @param shape
 * @param type
 * @return bwc_stream*
 */
bwc_stream *GetBwcStream(const char *data, char *outbuf, const Dims &shape, bwc_mode mode);

/**
 * Construct bwc_codec based on input information around the data pointer
 * @param data
 * @param shape
 * @param type
 * @return bwc_codec*
 */
bwc_codec *GetBwcCoder(const Params &parameters, bwc_precision precision, const Dims &dimensions,
                       bwc_mode mode);

BigWhoopOperator::BigWhoopOperator(const Params &parameters)
: PluginOperatorInterface(parameters), Impl(new BWCImpl)
{}

BigWhoopOperator::~BigWhoopOperator() {}

size_t BigWhoopOperator::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                                 const DataType type, char *bufferOut)
{
    // offset for writing into bufferOut
    size_t bufferOutOffset = 0;

    const size_t ndims = blockCount.size();

    // bwc metadata
    PutParameter(bufferOut, bufferOutOffset, ndims);
    for (const auto &d : blockCount)
    {
        PutParameter(bufferOut, bufferOutOffset, d);
    }
    PutParameter(bufferOut, bufferOutOffset, type);

    Dims convertedDims = ConvertBwcDims(blockCount, type, 5, true, 1);

    bwc_precision precision = GetBwcPrecision(type);
    bwc_codec *coder = GetBwcCoder(m_Parameters, precision, convertedDims, comp);

    std::string rate = "4";
    auto itRate = m_Parameters.find("rate");
    const bool hasRate = itRate != m_Parameters.end();
    rate = hasRate ? itRate->second : rate;

    bwc_stream *stream =
        bwc_init_stream(const_cast<char *>(dataIn), bufferOut + bufferOutOffset, comp);
    bwc_create_compression(coder, stream, const_cast<char *>(rate.data()));
    size_t sizeOut = (size_t)bwc_compress(coder, stream);

    size_t sizeIn =
        std::accumulate(convertedDims.begin(), convertedDims.end(), 1, std::multiplies<size_t>());
    double compression_ratio = static_cast<double>(sizeIn) / static_cast<double>(sizeOut);
    double expected_ratio = 1.0 / std::stod(rate);
    if (type == DataType::Double)
    {
        compression_ratio *= 8.0;
        expected_ratio *= 64;
    }
    else if (type == DataType::Float)
    {
        compression_ratio *= 4.0;
        expected_ratio *= 32;
    }
    std::cout << "BWC set rate " << rate << "; expected ratio " << expected_ratio
              << "; actual compression ratio " << compression_ratio << std::endl;

    if (sizeOut == 0)
    {
        helper::Throw<std::runtime_error>("Plugin", "BigWhoopOperator", "Operate(Compress)",
                                          "BigWhoop failed, compressed buffer size is 0");
    }

    bufferOutOffset += sizeOut;

    bwc_free_codec(coder);

    return bufferOutOffset;
}

size_t BigWhoopOperator::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t bufferInOffset = 0;

    const size_t ndims = GetParameter<size_t>(bufferIn, bufferInOffset);
    Dims blockCount(ndims);
    for (size_t i = 0; i < ndims; ++i)
    {
        blockCount[i] = GetParameter<size_t>(bufferIn, bufferInOffset);
    }
    const DataType type = GetParameter<DataType>(bufferIn, bufferInOffset);

    Dims convertedDims = ConvertBwcDims(blockCount, type, 5, true, 1);

    bwc_codec *decoder = bwc_alloc_decoder();
    bwc_stream *stream =
        bwc_init_stream(const_cast<char *>(bufferIn) + bufferInOffset, dataOut, decomp);
    bwc_create_decompression(decoder, stream, 0);
    bwc_decompress(decoder, stream);

    bwc_free_codec(decoder);

    return helper::GetTotalSize(convertedDims, helper::GetDataTypeSize(type));
}

bool BigWhoopOperator::IsDataTypeValid(const DataType type) const
{
    if (type == DataType::Float || type == DataType::Double)
    {
        return true;
    }
    return false;
}

size_t BigWhoopOperator::GetEstimatedSize(const size_t ElemCount, const size_t ElemSize,
                                          const size_t ndims, const size_t *dims) const
{
    size_t sizeIn = ElemCount * ElemSize;
    return (sizeof(size_t)           // ndims
            + sizeof(size_t) * ndims // count
            + sizeof(DataType)       // type
            + sizeIn                 // data
    );
}

Dims ConvertBwcDims(const Dims &dimensions, const DataType type, const size_t targetDims,
                    const bool enforceDims, const size_t defaultDimSize)
{
    if (targetDims < 1)
    {
        helper::Throw<std::invalid_argument>("Plugin", "BigWhoopOperator", "ConvertBwcDims",
                                             "only accepts targetDims > 0");
    }

    Dims ret = dimensions;

    while (ret.size() > targetDims)
    {
        ret[1] *= ret[0];
        ret.erase(ret.begin());
    }

    while (enforceDims && ret.size() < targetDims)
    {
        ret.push_back(defaultDimSize);
    }

    return ret;
}

bwc_precision GetBwcPrecision(DataType type)
{
    bwc_precision precision;

    if (type == helper::GetDataType<double>())
    {
        precision = bwc_precision_double;
    }
    else if (type == helper::GetDataType<float>())
    {
        precision = bwc_precision_single;
    }
    else
    {
        helper::Throw<std::invalid_argument>("Plugin", "BigWhoopOperator", "GetBwcPrecision",
                                             "invalid data type " + ToString(type));
    }

    return precision;
}

bwc_stream *GetBwcStream(const char *data, char *outbuf, bwc_precision precision,
                         const Dims &dimensions, bwc_mode mode)
{
    bwc_stream *stream = bwc_init_stream(const_cast<char *>(data), outbuf, mode);

    if (stream == nullptr)
    {
        helper::Throw<std::runtime_error>("Plugin", "BigWhoopOperator", "GetBwcStream",
                                          "BigWhoop failed to make stream for" +
                                              std::to_string(dimensions.size()) + "D data.");
    }

    return stream;
}

bwc_codec *GetBwcCoder(const Params &parameters, bwc_precision precision, const Dims &dimensions,
                       bwc_mode mode)
{
    bwc_codec *coder = nullptr;

    if (dimensions.size() == 1)
    {
        coder = bwc_alloc_coder(dimensions[0], 1, 1, 1, 1, precision);
    }
    else if (dimensions.size() == 2)
    {
        coder = bwc_alloc_coder(dimensions[0], dimensions[1], 1, 1, 1, precision);
    }
    else if (dimensions.size() == 3)
    {
        coder = bwc_alloc_coder(dimensions[0], dimensions[1], dimensions[2], 1, 1, precision);
    }
    else if (dimensions.size() == 4)
    {
        coder = bwc_alloc_coder(dimensions[0], dimensions[1], dimensions[2], dimensions[3], 1,
                                precision);
    }
    else if (dimensions.size() == 5)
    {
        coder = bwc_alloc_coder(dimensions[0], dimensions[1], dimensions[2], dimensions[3],
                                dimensions[4], precision);
    }
    else
    {
        helper::Throw<std::invalid_argument>("Plugin", "BigWhoopOperator", "GetBwcCoder",
                                             "BigWhoop does not support " +
                                                 std::to_string(dimensions.size()) + "D data.");
    }

    auto itQM = parameters.find("qm");
    const bool hasQM = itQM != parameters.end();

    if (hasQM)
    {
        const int QM =
            helper::StringTo<int>(itQM->second, "setting 'qm' in call to CompressBigWhoop\n");
        bwc_set_qm(coder, QM);
    }
    else
    {
        bwc_set_qm(coder, 32);
    }

    if (coder == nullptr)
    {
        helper::Throw<std::runtime_error>("Plugin", "BigWhoopOperator", "GetBwcCoder",
                                          "BigWhoop failed to make coder codec");
    }

    return coder;
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

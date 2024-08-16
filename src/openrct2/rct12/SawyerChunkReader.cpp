/*****************************************************************************
 * Copyright (c) 2014-2024 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "SawyerChunkReader.h"

#include "../core/IStream.hpp"
#include "../core/Numerics.hpp"

using namespace OpenRCT2;

// malloc is very slow for large allocations in MSVC debug builds as it allocates
// memory on a special debug heap and then initialises all the memory to 0xCC.
#if defined(_WIN32) && defined(DEBUG)
#    define __USE_HEAP_ALLOC__
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#endif

// Allow chunks to be uncompressed to a maximum of 16 MiB
constexpr size_t MAX_UNCOMPRESSED_CHUNK_SIZE = 16 * 1024 * 1024;

constexpr const char* EXCEPTION_MSG_CORRUPT_CHUNK_SIZE = "Corrupt chunk size.";
constexpr const char* EXCEPTION_MSG_CORRUPT_RLE = "Corrupt RLE compression data.";
constexpr const char* EXCEPTION_MSG_DESTINATION_TOO_SMALL = "Chunk data larger than allocated destination capacity.";
constexpr const char* EXCEPTION_MSG_INVALID_CHUNK_ENCODING = "Invalid chunk encoding.";
constexpr const char* EXCEPTION_MSG_ZERO_SIZED_CHUNK = "Encountered zero-sized chunk.";

SawyerChunkReader::SawyerChunkReader(OpenRCT2::IStream* stream)
    : _stream(stream)
{
}

void SawyerChunkReader::SkipChunk()
{
    uint64_t originalPosition = _stream->GetPosition();
    try
    {
        auto header = _stream->ReadValue<SawyerCodingChunkHeader>();
        _stream->Seek(header.length, OpenRCT2::STREAM_SEEK_CURRENT);
    }
    catch (const std::exception&)
    {
        // Rewind stream back to original position
        _stream->SetPosition(originalPosition);
        throw;
    }
}

std::shared_ptr<SawyerChunk> SawyerChunkReader::ReadChunk()
{
    uint64_t originalPosition = _stream->GetPosition();
    try
    {
        auto header = _stream->ReadValue<SawyerCodingChunkHeader>();
        if (header.length >= MAX_UNCOMPRESSED_CHUNK_SIZE)
            throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_CHUNK_SIZE);

        switch (header.encoding)
        {
            case CHUNK_ENCODING_NONE:
            case CHUNK_ENCODING_RLE:
            case CHUNK_ENCODING_RLECOMPRESSED:
            case CHUNK_ENCODING_ROTATE:
            {
                auto compressedData = std::make_unique<uint8_t[]>(header.length);
                if (_stream->TryRead(compressedData.get(), header.length) != header.length)
                {
                    throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_CHUNK_SIZE);
                }

                auto buffer = DecodeChunk(compressedData.get(), header);
                if (buffer.empty())
                {
                    throw SawyerChunkException(EXCEPTION_MSG_ZERO_SIZED_CHUNK);
                }
                return std::make_shared<SawyerChunk>(static_cast<SAWYER_ENCODING>(header.encoding), std::move(buffer));
            }
            default:
                throw SawyerChunkException(EXCEPTION_MSG_INVALID_CHUNK_ENCODING);
        }
    }
    catch (const std::exception&)
    {
        // Rewind stream back to original position
        _stream->SetPosition(originalPosition);
        throw;
    }
}

std::shared_ptr<SawyerChunk> SawyerChunkReader::ReadChunkTrack()
{
    uint64_t originalPosition = _stream->GetPosition();
    try
    {
        // Remove 4 as we don't want to touch the checksum at the end of the file
        int64_t compressedDataLength64 = _stream->GetLength() - _stream->GetPosition() - 4;
        if (compressedDataLength64 < 0 || compressedDataLength64 > std::numeric_limits<uint32_t>::max())
        {
            throw SawyerChunkException(EXCEPTION_MSG_ZERO_SIZED_CHUNK);
        }
        uint32_t compressedDataLength = compressedDataLength64;
        auto compressedData = std::make_unique<uint8_t[]>(compressedDataLength);

        if (_stream->TryRead(compressedData.get(), compressedDataLength) != compressedDataLength)
        {
            throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_CHUNK_SIZE);
        }

        SawyerCodingChunkHeader header{ CHUNK_ENCODING_RLE, compressedDataLength };
        auto buffer = DecodeChunk(compressedData.get(), header);
        if (buffer.empty())
        {
            throw SawyerChunkException(EXCEPTION_MSG_ZERO_SIZED_CHUNK);
        }
        return std::make_shared<SawyerChunk>(SAWYER_ENCODING::RLE, std::move(buffer));
    }
    catch (const std::exception&)
    {
        // Rewind stream back to original position
        _stream->SetPosition(originalPosition);
        throw;
    }
}

void SawyerChunkReader::ReadChunk(void* dst, size_t length)
{
    auto chunk = ReadChunk();
    auto chunkData = static_cast<const uint8_t*>(chunk->GetData());
    auto chunkLength = chunk->GetLength();
    if (chunkLength > length)
    {
        std::memcpy(dst, chunkData, length);
    }
    else
    {
        std::memcpy(dst, chunkData, chunkLength);
        auto remainingLength = length - chunkLength;
        if (remainingLength > 0)
        {
            auto offset = static_cast<uint8_t*>(dst) + chunkLength;
            std::fill_n(offset, remainingLength, 0x00);
        }
    }
}

std::vector<uint8_t> SawyerChunkReader::DecodeChunk(const void* src, const SawyerCodingChunkHeader& header)
{
    std::vector<uint8_t> buf;
    switch (header.encoding)
    {
        case CHUNK_ENCODING_NONE:
            buf.resize(header.length);
            std::memcpy(buf.data(), src, header.length);
            break;
        case CHUNK_ENCODING_RLE:
            buf = DecodeChunkRLE(src, header.length);
            break;
        case CHUNK_ENCODING_RLECOMPRESSED:
            buf = DecodeChunkRLERepeat(src, header.length);
            break;
        case CHUNK_ENCODING_ROTATE:
            buf = DecodeChunkRotate(src, header.length);
            break;
        default:
            throw SawyerChunkException(EXCEPTION_MSG_INVALID_CHUNK_ENCODING);
    }
    return buf;
}

std::vector<uint8_t> SawyerChunkReader::DecodeChunkRLERepeat(const void* src, size_t srcLength)
{
    auto tempBuf = DecodeChunkRLE(src, srcLength);
    return DecodeChunkRepeat(tempBuf.data(), tempBuf.size());
}

std::vector<uint8_t> SawyerChunkReader::DecodeChunkRLE(const void* src, size_t srcLength)
{
    std::vector<uint8_t> buf;
    buf.reserve(srcLength);

    auto src8 = static_cast<const uint8_t*>(src);
    for (size_t i = 0; i < srcLength; i++)
    {
        uint8_t rleCodeByte = src8[i];
        if (rleCodeByte & 128)
        {
            i++;
            size_t count = 257 - rleCodeByte;

            if (i >= srcLength)
            {
                throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_RLE);
            }
            if (buf.size() + count > MAX_UNCOMPRESSED_CHUNK_SIZE)
            {
                throw SawyerChunkException(EXCEPTION_MSG_DESTINATION_TOO_SMALL);
            }

            buf.insert(buf.end(), count, src8[i]);
        }
        else
        {
            const auto len = rleCodeByte + 1;

            if (i + 1 >= srcLength)
            {
                throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_RLE);
            }
            if (buf.size() + len > MAX_UNCOMPRESSED_CHUNK_SIZE)
            {
                throw SawyerChunkException(EXCEPTION_MSG_DESTINATION_TOO_SMALL);
            }
            if (i + 1 + len > srcLength)
            {
                throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_RLE);
            }

            const auto* pos = src8 + i + 1;
            buf.insert(buf.end(), pos, pos + len);

            i += len;
        }
    }

    return buf;
}

std::vector<uint8_t> SawyerChunkReader::DecodeChunkRepeat(const void* src, size_t srcLength)
{
    std::vector<uint8_t> buf;
    buf.reserve(srcLength);

    auto src8 = static_cast<const uint8_t*>(src);
    for (size_t i = 0; i < srcLength; i++)
    {
        if (src8[i] == 0xFF)
        {
            if (i + 1 >= srcLength)
            {
                throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_RLE);
            }
            buf.push_back(src8[++i]);
        }
        else
        {
            size_t count = (src8[i] & 7) + 1;
            int32_t offset = static_cast<int32_t>(src8[i] >> 3) - 32;
            const uint8_t* copySrc = buf.data() + (buf.size() + offset);

            if (copySrc < buf.data() || copySrc + count > buf.data() + buf.size())
            {
                throw SawyerChunkException(EXCEPTION_MSG_CORRUPT_RLE);
            }

            // We need a temporary buffer as the vector might invalidate the pointer.
            uint8_t temp[16];
            std::memcpy(temp, copySrc, count);

            buf.insert(buf.end(), std::begin(temp), std::begin(temp) + count);
        }
    }

    return buf;
}

std::vector<uint8_t> SawyerChunkReader::DecodeChunkRotate(const void* src, size_t srcLength)
{
    std::vector<uint8_t> buf;
    buf.reserve(srcLength);

    auto src8 = static_cast<const uint8_t*>(src);

    uint8_t code = 1;
    for (size_t i = 0; i < srcLength; i++)
    {
        buf.push_back(Numerics::ror8(src8[i], code));
        code = (code + 2) % 8;
    }

    return buf;
}

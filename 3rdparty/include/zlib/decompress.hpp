#include "config.hpp"

// std
#include <limits>
#include <stdexcept>
#include <string>

// zlib
#include "zlib.h"

namespace gzip
{

    class Decompressor
    {
        std::size_t max_;

    public:
        Decompressor(std::size_t max_bytes = 1000000000) // by default refuse operation if compressed data is > 1GB
            : max_(max_bytes)
        {}

        template <typename OutputType>
        void decompress(OutputType& output, const char* data, std::size_t size) const
        {
            z_stream inflate_s;

            inflate_s.zalloc = Z_NULL;
            inflate_s.zfree = Z_NULL;
            inflate_s.opaque = Z_NULL;
            inflate_s.avail_in = 0;
            inflate_s.next_in = Z_NULL;

            // The windowBits parameter is the base two logarithm of the window size (the size of the history buffer).
            // It should be in the range 8..15 for this version of the library.
            // Larger values of this parameter result in better compression at the expense of memory usage.
            // This range of values also changes the decoding type:
            //  -8 to -15 for raw deflate
            //  8 to 15 for zlib
            // (8 to 15) + 16 for gzip
            // (8 to 15) + 32 to automatically detect gzip/zlib header
            constexpr int window_bits = 15 + 32; // auto with windowbits of 15

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            if (inflateInit2(&inflate_s, window_bits) != Z_OK) {
                throw std::runtime_error("inflate init failed");
            }
#pragma GCC diagnostic pop
            inflate_s.next_in = reinterpret_cast<z_const Bytef*>(data);

#ifdef DEBUG
            // Verify if size (long type) input will fit into unsigned int, type used for zlib's avail_in
            std::uint64_t size_64 = size * 2;
            if (size_64 > std::numeric_limits<unsigned int>::max()) {
                inflateEnd(&inflate_s);
                throw std::runtime_error("size arg is too large to fit into unsigned int type x2");
            }
#endif
            if (size > max_ || (size * 2) > max_) {
                inflateEnd(&inflate_s);
                throw std::runtime_error("size may use more memory than intended when decompressing");
            }
            inflate_s.avail_in = static_cast<unsigned int>(size);
            std::size_t size_uncompressed = 0;
            do {
                std::size_t resize_to = size_uncompressed + 2 * size;
                if (resize_to > max_) {
                    inflateEnd(&inflate_s);
                    throw std::runtime_error(
                        "size of output string will use more memory then intended when decompressing");
                }
                output.resize(resize_to);
                inflate_s.avail_out = static_cast<unsigned int>(2 * size);
                inflate_s.next_out = reinterpret_cast<Bytef*>(&output[0] + size_uncompressed);
                int ret = inflate(&inflate_s, Z_FINISH);
                if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {
                    std::string error_msg = inflate_s.msg;
                    inflateEnd(&inflate_s);
                    // throw std::runtime_error(error_msg);
                    output.clear();
                    return;
                }

                size_uncompressed += (2 * size - inflate_s.avail_out);
            } while (inflate_s.avail_out == 0);
            inflateEnd(&inflate_s);
            output.resize(size_uncompressed);
        }
    };

    inline std::string decompress(const char* data, std::size_t size)
    {
        Decompressor decomp;
        std::string output;
        decomp.decompress(output, data, size);
        return output;
    }
} // namespace gzip

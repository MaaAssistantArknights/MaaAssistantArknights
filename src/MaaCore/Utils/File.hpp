#pragma once

#include <Utils/Ranges.hpp>
#include <filesystem>

namespace asst::utils
{
    template <typename ValueType>
    concept ByteValueType = std::is_integral_v<ValueType> && sizeof(ValueType) == 1;

    template <typename ContainerType>
    concept AppendableBytesContainer = requires(ContainerType a) {
        requires ranges::contiguous_range<ContainerType>;
        requires ByteValueType<typename ContainerType::value_type>;
        requires std::is_constructible_v<ContainerType>;
        requires std::is_constructible_v<ContainerType, size_t,
                                         typename ContainerType::value_type>; // std::string(count, ch),
                                                                              // std::vector(count, value)
        a.insert(a.end(), a.begin(), a.begin() + (size_t)1);
        a.resize(a.size());
    };

    template <AppendableBytesContainer ContainerType>
    ContainerType read_file(const std::filesystem::path& path)
    {
        ContainerType result;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        auto fileSize = file.tellg();
        if (fileSize != -1) {
            result.resize(fileSize);
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char*>(result.data()), fileSize);
        }
        else {
            // no size available, read to EOF
            constexpr auto chunksize = 4096;
            ContainerType chunk(chunksize, 0);
            while (!file.fail()) {
                file.read(reinterpret_cast<char*>(chunk.data()), chunksize);
                result.insert(result.end(), chunk.data(), chunk.data() + file.gcount());
            }
        }
        return result;
    }
}

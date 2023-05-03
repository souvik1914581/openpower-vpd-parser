#pragma once

#include <sdbusplus/asio/property.hpp>
#include <unordered_map>

namespace vpd
{
namespace types
{
// This covers mostly all the data type supported over Dbus for a property.
// clang-format off
using DbusVariantType = std::variant<
    std::vector<std::tuple<std::string, std::string, std::string>>,
    std::vector<std::string>,
    std::vector<double>,
    std::string,
    int64_t,
    uint64_t,
    double,
    int32_t,
    uint32_t,
    int16_t,
    uint16_t,
    uint8_t,
    bool,
    std::vector<uint32_t>,
    std::vector<uint16_t>,
    sdbusplus::message::object_path,
    std::tuple<uint64_t, std::vector<std::tuple<std::string, std::string, double, uint64_t>>>,
    std::vector<std::tuple<std::string, std::string>>,
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>,
    std::vector<std::tuple<uint32_t, size_t>>,
    std::vector<std::tuple<sdbusplus::message::object_path, std::string,
                           std::string, std::string>>
 >;

 using MapperGetObject =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

using ParsedVPD = std::unordered_map<std::string,
                                  std::unordered_map<std::string, std::string>>;
using BinaryVector = std::vector<uint8_t>;
using kwdVpdValueTypes = std::variant<size_t, BinaryVector, std::string>;
using KeywordVpdMap = std::unordered_map<std::string, kwdVpdValueTypes>;

using KwSize = uint8_t;
using RecordId = uint8_t;
using RecordSize = uint16_t;
using RecordType = uint16_t;
using RecordOffset = uint16_t;
using RecordLength = uint16_t;
using ECCOffset = uint16_t;
using ECCLength = uint16_t;
using PoundKwSize = uint16_t;

using RecordOffsetList = std::vector<uint32_t>;

    

} // namespace types
} // namespace vpd
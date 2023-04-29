#pragma once

#include "parser_interface.hpp"
#include "types.hpp"

#include <memory>

namespace vpd
{
/**
 * @brief Factory calss to instantiate concrete parser class.
 *
 * This class should be used to instantiate an instance of parser class based
 * on the type of vpd file.
 */
class ParserFactory
{
  public:
    // Deleted APIs
    ParserFactory() = delete;
    ~ParserFactory() = delete;
    ParserFactory(const ParserFactory&) = delete;
    ParserFactory& operator=(const ParserFactory&) = delete;
    ParserFactory(ParserFactory&&) = delete;
    ParserFactory& operator=(ParserFactory&&) = delete;

    /**
     * @brief An API to get object of concrete parser class.
     *
     * The method is used to get object of respective parser class based on the
     * type of VPD extracted from VPD vector passed to the API.
     *
     * Note: API throws DataException in case vpd type check fails for any
     * unknown type. Caller responsibility to handle the exception.
     *
     * @param[in] vpdVector - vpd file content to check for the type.
     * @param[in] inventoryPath - InventoryPath to call out FRU in case PEL is
     * logged by concrere parser class.
     * @param[in] vpdFilePath - FRU EEPROM path.
     * @param[in] vpdStartOffset - Offset from where VPD starts in the VPD file.
     *
     * @return - Pointer to concrete parser class object.
     */
    static std::shared_ptr<ParserInterface>
        getParser(const types::BinaryVector& vpdVector,
                  const std::string& inventoryPath,
                  const std::string& vpdFilePath, uint32_t vpdStartOffset);
};
} // namespace vpd
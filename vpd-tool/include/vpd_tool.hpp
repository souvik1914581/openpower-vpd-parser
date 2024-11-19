#pragma once

#include "utils.hpp"

#include <string>

namespace vpd
{
/**
 * @brief Class to support operations on VPD.
 *
 * The class provides API's to,
 * Read keyword value from DBus/hardware.
 * Update keyword value to DBus/hardware.
 * Dump DBus object's critical information.
 * Fix system VPD if any mismatch between DBus and hardware data.
 * Reset specific system VPD keywords to its default value.
 * Force VPD collection for hardware.
 */
class VpdTool
{
    /**
     * @brief Get Present property in JSON.
     *
     * API to get the Present property of a FRU in JSON format.
     *
     * @param[in] i_fruPath - DBus object path
     * @return On success, returns the Present property in JSON format,
     * otherwise return empty JSON
     *
     * @throw Does not throw
     */
    nlohmann::json
        getPresentPropertyJson(const std::string& i_fruPath) const noexcept;

    /**
     * @brief Get Pretty Name property in JSON.
     *
     * API to get the Pretty Name property of a FRU in JSON format.
     *
     * @param[in] i_fruPath - DBus object path.
     * @return On success, returns the Pretty Name property in JSON format,
     * otherwise return empty JSON.
     *
     * @throw Does not throw.
     */
    nlohmann::json
        getPrettyNameJson(const std::string& i_fruPath) const noexcept;

    /**
     * @brief Get VINI properties.
     *
     * API to get properties [SN, PN, CC, FN, DR] under VINI interface in JSON
     * format.
     *
     * @param[in] i_fruPath - DBus object path.
     *
     * @return On success, returns JSON output with the above properties under
     * VINI interface, otherwise returns empty JSON.
     *
     * @throw Does not throw.
     */
    nlohmann::json
        getVINIPropertiesJson(const std::string& i_fruPath) const noexcept;

  public:
    /**
     * @brief Read keyword value.
     *
     * API to read VPD keyword's value from the given input path.
     * If the provided i_onHardware option is true, read keyword's value from
     * the hardware. Otherwise read keyword's value from DBus.
     *
     * Note: In case read keyword's value from DBus, the return value from DBus
     * call expected as array of bytes.
     *
     * @param[in] i_fruPath - DBus object path or EEPROM path.
     * @param[in] i_recordName - Record name.
     * @param[in] i_keywordName - Keyword name.
     * @param[in] i_onHardware - True if i_fruPath is EEPROM path, false
     * otherwise.
     * @param[in] i_fileToSave - File path to save keyword's value, if not given
     * result will redirect to a console.
     *
     * @return On success return 0, otherwise return -1.
     */
    int readKeyword(const std::string& i_fruPath,
                    const std::string& i_recordName,
                    const std::string& i_keywordName, const bool i_onHardware,
                    const std::string& i_fileToSave = {});

    /**
     * @brief Dump the given inventory object in JSON format.
     *
     * For a given Object Path of a FRU, this API dumps the following properties
     * of the FRU in JSON format: Present property, Pretty Name, SN, PN, CC, FN,
     * DR keywords under VINI record.
     *
     * @param[in] i_fruPath - DBus object path.
     *
     * @param[in] io_resultJson - JSON object which holds the result.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int dumpObject(const std::string& i_fruPath,
                   nlohmann::json::object_t& io_resultJson) const noexcept;

    /**
     * @brief Dump all the objects in Inventory in JSON format.
     *
     * This API dumps the following properties
     * of all the FRUs in the Inventory in JSON format: Present property, Pretty
     * Name, SN, PN, CC, FN, DR keywords under VINI record.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int dumpInventory() const noexcept;
};
} // namespace vpd

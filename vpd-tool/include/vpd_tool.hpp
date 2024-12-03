#pragma once

#include "tool_types.hpp"

#include <nlohmann/json.hpp>

#include <optional>
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
  private:
    // Map of System VPD Keywords which can be reset by vpd-tool manufacturing
    // clean option
    static const types::SystemKeywordsMap g_systemVpdKeywordMap;

    /**
     * @brief Get specific properties of a FRU in JSON format.
     *
     * For a given object path of a FRU, this API returns the following
     * properties of the FRU in JSON format:
     * - Present property, Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     *
     * @param[in] i_fruPath - DBus object path
     *
     * @return On success, returns the properties of the FRU in JSON format,
     * otherwise throws a std::runtime_error exception.
     * Note: The caller of this API should handle this exception.
     *
     * @throw std::runtime_error
     */
    nlohmann::json getFruProperties(const std::string& i_fruPath) const;

  public:
    /**
     * @brief Read keyword value.
     *
     * API to read VPD keyword's value from the given input path.
     * If the provided i_onHardware option is true, read keyword's value from
     * the hardware. Otherwise read keyword's value from DBus.
     *
     * @param[in] i_vpdPath - DBus object path or EEPROM path.
     * @param[in] i_recordName - Record name.
     * @param[in] i_keywordName - Keyword name.
     * @param[in] i_onHardware - True if i_vpdPath is EEPROM path, false
     * otherwise.
     * @param[in] i_fileToSave - File path to save keyword's value, if not given
     * result will redirect to a console.
     *
     * @return On success return 0, otherwise return -1.
     */
    int readKeyword(const std::string& i_vpdPath,
                    const std::string& i_recordName,
                    const std::string& i_keywordName, const bool i_onHardware,
                    const std::string& i_fileToSave = {});

    /**
     * @brief Dump the given inventory object in JSON format to console.
     *
     * For a given object path of a FRU, this API dumps the following properties
     * of the FRU in JSON format to console:
     * - Present property, Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     *
     * @param[in] i_fruPath - DBus object path.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int dumpObject(const std::string& i_fruPath) const noexcept;

    /**
     * @brief API to fix system VPD keywords.
     *
     * The API to fix the system VPD keywords. Mainly used when there
     * is a mismatch between the primary and backup(secondary) VPD. User can
     * choose option to update all primary keywords value with corresponding
     * backup keywords value or can choose primary keyword value to sync
     * secondary VPD. Otherwise, user can also interactively choose different
     * action for individual keyword.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int fixSystemVpd() noexcept;

    /**
     * @brief Write keyword's value.
     *
     * API to update VPD keyword's value to the given input path.
     * If i_onHardware value in true, i_vpdPath is considered has hardware path
     * otherwise it will be considered as DBus object path.
     *
     * For provided DBus object path both primary path or secondary path will
     * get updated, also redundant EEPROM(if any) path with new keyword's value.
     *
     * In case of hardware path, only given hardware path gets updated with new
     * keyword’s value, any backup or redundant EEPROM (if exists) paths won't
     * get updated.
     *
     * @param[in] i_vpdPath - DBus object path or EEPROM path.
     * @param[in] i_recordName - Record name.
     * @param[in] i_keywordName - Keyword name.
     * @param[in] i_keywordValue - Keyword value.
     * @param[in] i_onHardware - True if i_vpdPath is EEPROM path, false
     * otherwise.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int writeKeyword(std::string i_vpdPath, const std::string& i_recordName,
                     const std::string& i_keywordName,
                     const std::string& i_keywordValue,
                     const bool i_onHardware) noexcept;

    /**
     * @brief Reset specific keywords on System VPD to default value.
     *
     * This API resets specific System VPD keywords to default value. The
     * keyword values are reset on:
     * 1. Primary EEPROM path.
     * 2. Secondary EEPROM path.
     * 3. D-Bus cache.
     * 4. Backup path.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int cleanSystemVpd() const noexcept;
};
} // namespace vpd

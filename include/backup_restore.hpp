#pragma once

#include "types.hpp"

#include <nlohmann/json.hpp>

#include <tuple>

namespace vpd
{
/**
 * @brief class to implement backup and restore VPD.
 *
 */
class BackupRestore
{
  public:
    // delete functions
    BackupRestore() = delete;
    BackupRestore(const BackupRestore&) = delete;
    BackupRestore& operator=(const BackupRestore&) = delete;
    BackupRestore(BackupRestore&&) = delete;
    BackupRestore& operator=(BackupRestore&&) = delete;

    /**
     * @brief Constructor.
     *
     * @param[in] i_parsedSystemJson - Parsed system config JSON.
     *
     */
    BackupRestore(const nlohmann::json& i_parsedSystemJson);

    /**
     * @brief Default destructor.
     */
    ~BackupRestore() = default;

    /**
     * @brief An API to backup and restore VPD.
     *
     * Note: This API works on the keywords declared in the backup and restore
     * config JSON. Restore or backup action could be triggered for each
     * keyword, based on the keyword's value present in the source and
     * destination keyword.
     *
     * Restore source keyword's value with destination keyword's value,
     * when source keyword has default value but
     * destination's keyword has non default value.
     *
     * Backup the source keyword value to the destination's keyword's value,
     * when source keyword has non default value but
     * destination's keyword has default value.
     *
     * @return Tuple of updated source and destination VPD map variant.
     */
    std::tuple<types::VPDMapVariant, types::VPDMapVariant> backupAndRestore();

  private:
    /**
     * @brief An API to handle backup and restore of IPZ type VPD.
     *
     * @param[in,out] io_srcVpdMap - Source VPD map.
     * @param[in,out] io_dstVpdMap - Destination VPD map.
     * @param[in] i_srcPath - Source EEPROM file path or inventory path.
     * @param[in] i_dstPath - Destination EEPROM file path or inventory path.
     */
    void backupAndRestoreIpzVpd(types::IPZVpdMap& io_srcVpdMap,
                                types::IPZVpdMap& io_dstVpdMap,
                                const std::string& i_srcPath,
                                const std::string& i_dstPath);

    // Parsed backup and restore's JSON config file.
    nlohmann::json m_backupRestoreParsedJson{};
};
} // namespace vpd

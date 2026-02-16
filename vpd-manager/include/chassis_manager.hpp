#pragma once

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace vpd
{

// Chassis information structure
struct ChassisInfo
{
    std::string m_chassisId; // string representing chassis, for eg. chassis0,
                             // chassis1, etc.
    std::string m_chassisPath; // object path for the chassis, for eg.
                               // /xyz/openbmc_project/inventory/system/chassis0
    nlohmann::json m_chassisConfig; // Chassis-specific config
};

/**
 * @brief Main chassis manager class
 *
 * This class manages chassis configuration for multi-chassis systems.
 * It provides O(1) lookup for chassis-to-EEPROM mapping and maintains
 * backward compatibility with single-chassis systems.
 *
 * @note This class uses the singleton pattern with restricted instantiation.
 * Only the Worker class can create instances of ChassisConfigManager using
 * the passkey idiom.
 */
class ChassisConfigManager
{
  public:
    /**
     * @brief Passkey class to restrict instantiation to Worker class only
     *
     * This is a private nested class that can only be constructed by Worker.
     * It acts as a "key" that must be passed to ChassisConfigManager's
     * constructor, ensuring only Worker can create instances.
     */
    class ConstructorKey
    {
      private:
        // Only Worker can construct this key
        ConstructorKey() = default;
        friend class Worker;
    };
    /**
     * @brief Constructor with passkey - can only be called by Worker
     *
     * This constructor is public but requires a ConstructorKey that only
     * Worker can create, effectively restricting instantiation to Worker.
     *
     * @param[in] key - Constructor key (only Worker can create this)
     * @param[in] i_systemConfigJson - System config JSON object
     * @throw JsonException on parsing errors
     */
    explicit ChassisConfigManager(ConstructorKey key,
                                   const nlohmann::json& i_systemConfigJson) :
        m_systemConfigJson{i_systemConfigJson}
    {
        (void)key; // Suppress unused parameter warning
        buildChassisToFruMap();
    }

    /**
     * @brief Check if system is multi-chassis
     * @return true if multi-chassis, false for single-chassis
     */
    bool isMultiChassis() const noexcept;

    /**
     * @brief Get config JSON (for backward compatibility)
     * - If input parameter is std::nullopt, then return main system config JSON
     * - If input parameter is EEPROM path, return Chassis specific JSON
     * - If input parameter is Object path, return Chassis specific JSON
     * @param[in] i_vpdPath - Optional VPD path (EEPROM or Object path)
     * @return Complete system config JSON or chassis-specific JSON
     */
    const nlohmann::json&
        getJsonObj(const std::optional<std::string> i_vpdPath =
                       std::nullopt) const noexcept;

    /**
     * @brief Deleted copy constructor
     */
    ChassisConfigManager(const ChassisConfigManager&) = delete;

    /**
     * @brief Deleted copy assignment operator
     */
    ChassisConfigManager& operator=(const ChassisConfigManager&) = delete;

    /**
     * @brief Deleted move constructor
     */
    ChassisConfigManager(ChassisConfigManager&&) = delete;

    /**
     * @brief Deleted move assignment operator
     */
    ChassisConfigManager& operator=(ChassisConfigManager&&) = delete;

    /**
     * @brief Destructor
     */
    ~ChassisConfigManager() = default;

  private:

    /**
     * @brief Build EEPROM to chassis mapping - O(n) at initialization
     *
     * This method iterates through the system config JSON and builds
     * the necessary maps for O(1) lookup during runtime.
     *
     * TODO:
     * 1. Iterate through "frus" under system config JSON
     *    1.i. For each FRU, iterate through the sub FRUS
     *         1.i.i. For each FRU, extract Chassis ID using Object path at
     *                index 0, and build EEPROM to Chassis Map.
     *         1.i.ii. For each sub FRU, use the object path to get the chassis
     *                 ID, and add the sub JSON to the ChassisToFRU Map.
     */
    void buildChassisToFruMap();

    /**
     * @brief Get chassis ID for a given object path - O(1)
     * @param[in] i_inventoryPath - Inventory path
     * @param[out] o_errCode - Error code if lookup fails
     * @return Chassis ID string, empty on failure
     */
    std::string getChassisIdForObjectPath(const std::string& i_inventoryPath,
                                          uint16_t& o_errCode) const noexcept;

    /**
     * @brief Get chassis-specific JSON config - O(1)
     * @param[in] i_chassisId - Chassis identifier
     * @param[out] o_errCode - Error code if lookup fails
     * @return Chassis-specific JSON object
     */
    nlohmann::json getChassisConfig(const std::string& i_chassisId,
                                    uint16_t& o_errCode) const noexcept;

    // System config JSON
    nlohmann::json m_systemConfigJson;

    // Chassis ID to chassis info map - O(1) lookup
    std::unordered_map<std::string, ChassisInfo> m_chassisInfoMap;

    // EEPROM path to chassis ID - O(1) lookup
    std::unordered_map<std::string, std::string> m_eepromToChassisIdMap;

    // Flag indicating multi-chassis system
    bool m_isMultiChassis{false};
};

} // namespace vpd

// Made with Bob

#pragma once

#include "types.hpp"

#include <nlohmann/json.hpp>

namespace vpd
{
/**
 * @brief A class to process and publish VPD data.
 *
 * The class works on VPD and is mainly responsible for following tasks:
 * 1) Select appropriate device tree and JSON. Reboot if required.
 * 2) Get desired parser using parser factory.
 * 3) Calling respective parser class to get parsed VPD.
 * 4) Arranging VPD data under required interfaces.
 * 5) Calling PIM to publish VPD.
 *
 * The class may also implement helper functions required for VPD handling.
 */
class Worker
{
  public:
    /**
     * List of deleted functions.
     */
    Worker(const Worker&);
    Worker& operator=(const Worker&);
    Worker(Worker&&) = delete;

    /**
     * @brief Constructor.
     *
     * Constructor will also, based on symlink pick the correct JSON and
     * initialize the parsed JSON variable.
     *
     * Note: Throws std::exception in case of construction failure. Caller needs
     * to handle to detect successful object creation.
     */
    Worker();

    /**
     * @brief Destructor
     */
    ~Worker() = default;

    /**
     * @brief An API to set appropriate device tree and JSON.
     *
     * This API based on system chooses corresponding device tree and JSON.
     * If device tree change is required, it updates the "fitconfig" and reboots
     * the system. Else it is NOOP.
     *
     * Note: In case of any error, exception is thrown. Caller need to handle.
     */
    void setDeviceTreeAndJson();

    /**
     * @brief An API to parse and publish system VPD on D-Bus.
     */
    void publishSystemVPD();

    /**
     * @brief An API to process all FRUs.
     *
     * This API based on inventory JSON selected for the system will parse and
     * publish their VPD over Dbus.
     */
    void processAllFRU();

  private:
    /**
     * @brief API to select system specific JSON.
     *
     * The api based on the IM value of VPD, will select appropriate JSON for
     * the system. In case no system is found corresponding to the extracted IM
     * value, error will be logged.
     *
     * @param[out] systemJson - System JSON name.
     */
    void getSystemJson(std::string& systemJson);

    /**
     * @brief An API to read IM value from VPD.
     *
     * @param[in] parsedVpd - Parsed VPD.
     */
    std::string getIMValue(const types::ParsedVPD& parsedVpd) const;

    /**
     * @brief An API to read HW version from VPD.
     *
     * @param[in] parsedVpd - Parsed VPD.
     */
    std::string getHWVersion(const types::ParsedVPD& parsedVpd) const;

    /**
     * @brief An API to parse given VPD file path.
     *
     * @param[in] vpdFilePath - EEPROM file path.
     */
    void fillVPDMap(const std::string& vpdFilePath);

    /**
     * @brief An API to check if motherboard path is already published.
     */
    bool isMotherboardPathOnDBus() const;

    // Parsed JSON file.
    nlohmann::json m_parsedJson{};

    // Parsed VPD.
    types::VPDMapVariant m_vpdMap{};

    // Hold if symlink is present or not.
    bool m_isSymlinkPresent = false;
};
} // namespace vpd
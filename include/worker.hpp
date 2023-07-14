#pragma once

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
     */
    Worker() = default;

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
     * Similarly for JSON, synlink is craeted if not found else ignored.
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
};
} // namespace vpd
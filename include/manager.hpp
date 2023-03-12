#pragma once

#include <sdbusplus/asio/object_server.hpp>

namespace vpd
{
/**
 * @brief Class to manage VPD processing.
 * The class is responsible to implement methods to manage VPD on the system.
 * It also implements methods to be exposed over D-Bus required to access/edit
 * VPD data.
 */
class Manager
{
  public:
    /**
     * List of deleted methods.
     */
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;

    /**
     * @brief Constructor.
     * @param[in] ioCon - IO context.
     * @param[in] iFace - interface to implement.
     * @param[in] connection - Dbus Connection.
     */
    Manager(const std::shared_ptr<boost::asio::io_context>& ioCon,
            const std::shared_ptr<sdbusplus::asio::dbus_interface>& iFace,
            const std::shared_ptr<sdbusplus::asio::connection>& conn);

    /**
     * @brief Destructor.
     */
    ~Manager() = default;

  private:
    // Shared pointer to asio context object.
    const std::shared_ptr<boost::asio::io_context>& ioContext;

    // Shared pointer to Dbus interface class.
    const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface;

    // Shared pointer to bus connection.
    const std::shared_ptr<sdbusplus::asio::connection>& conn;
};

} // namespace vpd
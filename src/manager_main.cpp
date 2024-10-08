#include "config.h"

#include "bios_handler.hpp"
#include "logger.hpp"
#include "manager.hpp"

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>

/**
 * @brief Main function for VPD parser application.
 */
int main(int, char**)
{
    try
    {
        auto io_con = std::make_shared<boost::asio::io_context>();
        auto connection =
            std::make_shared<sdbusplus::asio::connection>(*io_con);
        connection->request_name(BUSNAME);

        auto server = sdbusplus::asio::object_server(connection);

        std::shared_ptr<sdbusplus::asio::dbus_interface> interface =
            server.add_interface(OBJPATH, IFACE);

        auto vpdManager = std::make_shared<vpd::Manager>(io_con, interface,
                                                         connection);

        // TODO: Take this under conditional compilation for IBM
        auto biosHandler =
            std::make_shared<vpd::BiosHandler<vpd::IbmBiosHandler>>(connection,
                                                                    vpdManager);

        interface->initialize();

        vpd::logging::logMessage("Start VPD-Manager event loop");

        // Start event loop.
        io_con->run();

        exit(EXIT_SUCCESS);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    exit(EXIT_FAILURE);
}

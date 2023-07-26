#include "config.h"

#include "worker.hpp"

#include "exceptions.hpp"
#include "logger.hpp"
#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include <typeindex>

namespace vpd
{

Worker::Worker()
{
    auto jsonToParse = INVENTORY_JSON_DEFAULT;

    // Check if symlink is already there to confirm fresh boot/factory
    // reset.
    if (std::filesystem::exists(INVENTORY_JSON_SYM_LINK))
    {
        jsonToParse = INVENTORY_JSON_SYM_LINK;
        m_isSymlinkPresent = true;
    }
    // implies it is a fresh boot/factory reset.
    else
    {
        // Create the directory for hosting the symlink
        std::filesystem::create_directories(VPD_SYMLIMK_PATH);
    }

    std::ifstream inventoryJson(jsonToParse);
    if (!inventoryJson)
    {
        throw(JsonException("Failed to access Json path", jsonToParse));
    }

    try
    {
        m_parsedJson = nlohmann::json::parse(inventoryJson);
    }
    catch (const nlohmann::json::parse_error& ex)
    {
        throw(JsonException("Json parsing failed", jsonToParse));
    }
}

static std::string readFitConfigValue()
{
    std::vector<std::string> output = utils::executeCmd("/sbin/fw_printenv");
    std::string fitConfigValue;

    for (const auto& entry : output)
    {
        auto pos = entry.find("=");
        auto key = entry.substr(0, pos);
        if (key != "fitconfig")
        {
            continue;
        }

        if (pos + 1 < entry.size())
        {
            fitConfigValue = entry.substr(pos + 1);
        }
    }

    return fitConfigValue;
}

bool Worker::isMotherboardPathOnDBus() const
{
    const std::string& mboardPath =
        m_parsedJson["frus"][SYSTEM_VPD_FILE_PATH].at(0).value("inventoryPath",
                                                               "");

    if (mboardPath.empty())
    {
        throw JsonException("Motherboard path missing in JSON",
                            INVENTORY_JSON_SYM_LINK);
    }

    std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};

    types::MapperGetObject objectMap =
        utils::getObjectMap(mboardPath.data(), interfaces);

    if (objectMap.empty())
    {
        return false;
    }
    return true;
}

std::string Worker::getIMValue(const types::ParsedVPD& parsedVpd) const
{
    (void)parsedVpd;
    return "";
}

std::string Worker::getHWVersion(const types::ParsedVPD& parsedVpd) const
{
    (void)parsedVpd;
    return "";
}

void Worker::fillVPDMap(const std::string& vpdFilePath)
{
    (void)vpdFilePath;
}

void Worker::getSystemJson(std::string& systemJson)
{
    types::SystemTypeMap systemType{
        {"50001001", {{"0001", "v2"}}},
        {"50001000", {{"0001", "v2"}}},
        {"50001002", {}},
        {"50003000",
         {{"000A", "v2"}, {"000B", "v2"}, {"000C", "v2"}, {"0014", "v2"}}},
        {"50004000", {}}};

    fillVPDMap(SYSTEM_VPD_FILE_PATH);

    if (auto pVal = std::get_if<types::ParsedVPD>(&m_vpdMap))
    {
        std::string hwKWdValue = getHWVersion(*pVal);
        const std::string& imKwdValue = getIMValue(*pVal);

        auto itrToIM = systemType.find(imKwdValue);
        if (itrToIM == systemType.end())
        {
            logging::logMessage("IM keyword does not map to any system type");
            return;
        }

        const auto hwVersionList = itrToIM->second;
        if (!hwVersionList.empty())
        {
            transform(hwKWdValue.begin(), hwKWdValue.end(), hwKWdValue.begin(),
                      ::toupper);

            auto itrToHW =
                std::find_if(hwVersionList.begin(), hwVersionList.end(),
                             [&hwKWdValue](const auto& aPair) {
                                 return aPair.first == hwKWdValue;
                             });

            if (itrToHW != hwVersionList.end())
            {
                // make it dynamic
                systemJson += imKwdValue + "_" + (*itrToHW).second + ".json";
                return;
            }
        }
        systemJson += imKwdValue + ".json";
    }
    else
    {
        logging::logMessage("Invalid VPD type returned from Parser");
    }
}

static void setEnvAndReboot(const std::string& key, const std::string& value)
{
    // set env and reboot and break.
    utils::executeCmd("/sbin/fw_setenv", key, value);
    logging::logMessage("Rebooting BMC to pick up new device tree");

    // make dbus call to reboot
    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "Reboot");
    bus.call_noreply(method);
}

void Worker::setDeviceTreeAndJson()
{
    try
    {
        if (m_isSymlinkPresent && isMotherboardPathOnDBus())
        {
            // As symlink is also present and motherboard path is also
            // populated, call to thi API implies, this is a situation where
            // VPD-manager service got restarted due to some reason. Do not
            // process for selection of device tree or JSON selection in this
            // case.
            logging::logMessage("Servcie restarted for some reason.");
            return;
        }

        // Do we have the entry for device tree in parsed JSON?
        if (m_parsedJson.find("devTree") == m_parsedJson.end())
        {
            // Implies it is default JSON.
            std::string systemJson{JSON_ABSOLUTE_PATH_PREFIX};
            getSystemJson(systemJson);

            if (!systemJson.compare(JSON_ABSOLUTE_PATH_PREFIX))
            {
                // TODO: Log a PEL saying that "System type not supported"
                throw DataException("Error in getting system JSON.");
            }

            // create a new symlink based on the system
            std::filesystem::create_symlink(systemJson,
                                            INVENTORY_JSON_SYM_LINK);

            // re-parse the JSON once appropriate JSON has been selected.
            try
            {
                m_parsedJson = nlohmann::json::parse(INVENTORY_JSON_SYM_LINK);
            }
            catch (const nlohmann::json::parse_error& ex)
            {
                throw(JsonException("Json parsing failed", systemJson));
            }
        }

        auto devTreeFromJson = m_parsedJson["devTree"].value("path", "");
        if (devTreeFromJson.empty())
        {
            throw JsonException(
                "Mandatory value for device tree missing from JSON",
                INVENTORY_JSON_SYM_LINK);
        }

        auto fitConfigVal = readFitConfigValue();

        if (fitConfigVal.find(devTreeFromJson) != std::string::npos)
        {
            // fitconfig is updated and correct JSON is set.
            return;
        }

        // Set fitconfig even if it is read as empty.
        setEnvAndReboot("fitconfig", devTreeFromJson);
        exit(EXIT_SUCCESS);
    }
    catch (const std::exception& ex)
    {
        if (typeid(ex) == std::type_index(typeid(DataException)))
        {
            // TODO:Catch logic to be implemented once PEL code goes in.
        }
        else if (typeid(ex) == std::type_index(typeid(EccException)))
        {
            // TODO:Catch logic to be implemented once PEL code goes in.
        }
        else if (typeid(ex) == std::type_index(typeid(JsonException)))
        {
            // TODO:Catch logic to be implemented once PEL code goes in.
        }

        logging::logMessage(ex.what());
        throw;
    }
}

void Worker::publishSystemVPD()
{
    // TODO: Implement to parse system VPD.
}

void Worker::processAllFRU()
{
    // TODO: Implement to process all other FRUs.
}
} // namespace vpd
#include "config.h"

#include "worker.hpp"

#include "configuration.hpp"
#include "constants.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include "parser_factory.hpp"
#include "parser_interface.hpp"
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
        utils::getObjectMap(PIM_PATH_PREFIX + mboardPath, interfaces);

    if (objectMap.empty())
    {
        return false;
    }
    return true;
}

std::string Worker::getIMValue(const types::ParsedVPD& parsedVpd) const
{
    if (parsedVpd.empty())
    {
        throw std::runtime_error("Empty VPD map. Can't Extract IM value");
    }

    const auto& itrToVSBP = parsedVpd.find("VSBP");
    if (itrToVSBP == parsedVpd.end())
    {
        throw DataException("VSBP record missing.");
    }

    const auto& itrToIM = (itrToVSBP->second).find("IM");
    if (itrToIM == (itrToVSBP->second).end())
    {
        throw DataException("IM keyword missing.");
    }

    types::BinaryVector imVal;
    std::copy(itrToIM->second.begin(), itrToIM->second.end(),
              back_inserter(imVal));

    std::ostringstream imData;
    for (auto& aByte : imVal)
    {
        imData << std::setw(2) << std::setfill('0') << std::hex
               << static_cast<int>(aByte);
    }

    return imData.str();
}

std::string Worker::getHWVersion(const types::ParsedVPD& parsedVpd) const
{
    if (parsedVpd.empty())
    {
        throw std::runtime_error("Empty VPD map. Can't Extract IM value");
    }

    const auto& itrToVINI = parsedVpd.find("VINI");
    if (itrToVINI == parsedVpd.end())
    {
        throw DataException("VINI record missing.");
    }

    const auto& itrToHW = (itrToVINI->second).find("HW");
    if (itrToHW == (itrToVINI->second).end())
    {
        throw DataException("HW keyword missing.");
    }

    types::BinaryVector hwVal;
    std::copy(itrToHW->second.begin(), itrToHW->second.end(),
              back_inserter(hwVal));

    // The planar pass only comes from the LSB of the HW keyword,
    // where as the MSB is used for other purposes such as signifying clock
    // termination.
    hwVal[0] = 0x00;

    std::ostringstream hwString;
    for (auto& aByte : hwVal)
    {
        hwString << std::setw(2) << std::setfill('0') << std::hex
                 << static_cast<int>(aByte);
    }

    return hwString.str();
}

void Worker::getVpdDataInVector(const std::string& vpdFilePath,
                                types::BinaryVector& vpdVector,
                                size_t& vpdStartOffset)
{
    std::fstream vpdFileStream;
    vpdFileStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);

    try
    {
        vpdFileStream.open(vpdFilePath,
                           std::ios::in | std::ios::out | std::ios::binary);
        auto vpdSizeToRead = std::min(std::filesystem::file_size(vpdFilePath),
                                      static_cast<uintmax_t>(65504));
        vpdVector.resize(vpdSizeToRead);

        vpdFileStream.seekg(vpdStartOffset, std::ios_base::beg);
        vpdFileStream.read(reinterpret_cast<char*>(&vpdVector[0]),
                           vpdSizeToRead);

        vpdVector.resize(vpdFileStream.gcount());
        vpdFileStream.clear(std::ios_base::eofbit);
    }
    catch (const std::ifstream::failure& fail)
    {
        std::cerr << "Exception in file handling [" << vpdFilePath
                  << "] error : " << fail.what();
        std::cerr << "Stream file size = " << vpdFileStream.gcount()
                  << std::endl;
        throw;
    }

    // Make sure we reset the EEPROM pointer to a "safe" location if it was
    // DIMM SPD that we just read.
    for (const auto& item : m_parsedJson["frus"][vpdFilePath])
    {
        if ((item.find("extraInterfaces") != item.end()) &&
            (item["extraInterfaces"].find(
                 "xyz.openbmc_project.Inventory.Item.Dimm") !=
             item["extraInterfaces"].end()))
        {
            // moves the EEPROM pointer to 2048 'th byte.
            vpdFileStream.seekg(2047, std::ios::beg);

            // Read that byte and discard - to affirm the move
            // operation.
            char ch;
            vpdFileStream.read(&ch, sizeof(ch));
            break;
        }
    }
}

void Worker::fillVPDMap(const std::string& vpdFilePath)
{
    logging::logMessage(std::string("Parsing file = ") + vpdFilePath);

    if (vpdFilePath.empty())
    {
        logging::logMessage("Empty file path. Unable to process.");
        return;
    }

    size_t vpdStartOffset = 0;
    std::string fruInventoryPath;

    // check if offset present.
    if (m_parsedJson["frus"].contains(vpdFilePath))
    {
        for (const auto& item : m_parsedJson["frus"][vpdFilePath])
        {
            if (item.find("offset") != item.end())
            {
                vpdStartOffset = item["offset"];
            }
        }

        fruInventoryPath =
            m_parsedJson["frus"][vpdFilePath][0]["inventoryPath"];
    }

    types::BinaryVector vpdVector;
    getVpdDataInVector(vpdFilePath, vpdVector, vpdStartOffset);

    std::shared_ptr<ParserInterface> parser = ParserFactory::getParser(
        vpdVector, (PIM_PATH_PREFIX + fruInventoryPath), vpdFilePath,
        vpdStartOffset);
    m_vpdMap = parser->parse();
}

void Worker::getSystemJson(std::string& systemJson)
{
    fillVPDMap(SYSTEM_VPD_FILE_PATH);

    if (auto pVal = std::get_if<types::ParsedVPD>(&m_vpdMap))
    {
        std::string hwKWdValue = getHWVersion(*pVal);
        if (hwKWdValue.empty())
        {
            throw DataException("HW value fetched is empty.");
        }

        const std::string& imKwdValue = getIMValue(*pVal);
        if (imKwdValue.empty())
        {
            throw DataException("IM value fetched is empty.");
        }

        auto itrToIM = config::systemType.find(imKwdValue);
        if (itrToIM == config::systemType.end())
        {
            throw DataException("IM keyword does not map to any system type");
        }

        const types::HWVerList hwVersionList = itrToIM->second.second;
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
                systemJson += imKwdValue + "_" + (*itrToHW).second + ".json";
                return;
            }
        }
        systemJson += itrToIM->second.first + ".json";
        return;
    }

    throw DataException("Invalid VPD type returned from Parser");
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
            // process for selection of device tree or JSON selection in
            // this case.
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
#include "config.h"

#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_types.hpp"
#include "tool_utils.hpp"

#include <iostream>
#include <tuple>

namespace vpd
{
int VpdTool::readKeyword(const std::string& i_vpdPath,
                         const std::string& i_recordName,
                         const std::string& i_keywordName,
                         const bool i_onHardware,
                         const std::string& i_fileToSave)
{
    int l_rc = constants::FAILURE;
    try
    {
        types::DbusVariantType l_keywordValue;
        if (i_onHardware)
        {
            l_keywordValue = utils::readKeywordFromHardware(
                i_vpdPath, std::make_tuple(i_recordName, i_keywordName));
        }
        else
        {
            std::string l_inventoryObjectPath(constants::baseInventoryPath +
                                              i_vpdPath);

            l_keywordValue = utils::readDbusProperty(
                constants::inventoryManagerService, l_inventoryObjectPath,
                constants::ipzVpdInfPrefix + i_recordName, i_keywordName);
        }

        if (const auto l_value =
                std::get_if<types::BinaryVector>(&l_keywordValue);
            l_value && !l_value->empty())
        {
            // ToDo: Print value in both ASCII and hex formats
            const std::string& l_keywordStrValue =
                utils::getPrintableValue(*l_value);

            if (i_fileToSave.empty())
            {
                utils::displayOnConsole(i_vpdPath, i_keywordName,
                                        l_keywordStrValue);
                l_rc = constants::SUCCESS;
            }
            else
            {
                if (utils::saveToFile(i_fileToSave, l_keywordStrValue))
                {
                    std::cout
                        << "Value read is saved on the file: " << i_fileToSave
                        << std::endl;
                    l_rc = constants::SUCCESS;
                }
                else
                {
                    std::cerr
                        << "Error while saving the read value on the file: "
                        << i_fileToSave
                        << "\nDisplaying the read value on console"
                        << std::endl;
                    utils::displayOnConsole(i_vpdPath, i_keywordName,
                                            l_keywordStrValue);
                }
            }
        }
        else
        {
            // TODO: Enable logging when verbose is enabled.
            // std::cout << "Invalid data type or empty data received." <<
            // std::endl;
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Read keyword's value for path: " << i_vpdPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << " is failed, exception: " << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

int VpdTool::dumpObject(const std::string& i_fruPath) const noexcept
{
    int l_rc{constants::FAILURE};
    try
    {
        nlohmann::json l_resultJsonArray = nlohmann::json::array({});
        const nlohmann::json l_fruJson = getFruProperties(i_fruPath);
        if (!l_fruJson.empty())
        {
            l_resultJsonArray += l_fruJson;

            utils::printJson(l_resultJsonArray);
        }
        else
        {
            std::cout << "FRU " << i_fruPath << " is not present in the system"
                      << std::endl;
        }
        l_rc = constants::SUCCESS;
    }
    catch (std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        // std::cerr << "Dump Object failed for FRU: " << i_fruPath
        //           << " Error: " << l_ex.what() << std::endl;
    }
    return l_rc;
}

nlohmann::json VpdTool::getFruProperties(const std::string& i_objectPath) const
{
    // check if FRU is present in the system
    if (!isFruPresent(i_objectPath))
    {
        return nlohmann::json::object_t();
    }

    nlohmann::json l_fruJson = nlohmann::json::object_t({});

    l_fruJson.emplace(i_objectPath, nlohmann::json::object_t({}));

    auto& l_fruObject = l_fruJson[i_objectPath];

    const auto l_prettyNameInJson = getInventoryPropertyJson<std::string>(
        i_objectPath, constants::inventoryItemInf, "PrettyName");
    if (!l_prettyNameInJson.empty())
    {
        l_fruObject.insert(l_prettyNameInJson.cbegin(),
                           l_prettyNameInJson.cend());
    }

    const auto l_locationCodeInJson = getInventoryPropertyJson<std::string>(
        i_objectPath, constants::locationCodeInf, "LocationCode");
    if (!l_locationCodeInJson.empty())
    {
        l_fruObject.insert(l_locationCodeInJson.cbegin(),
                           l_locationCodeInJson.cend());
    }

    const auto l_subModelInJson = getInventoryPropertyJson<std::string>(
        i_objectPath, constants::assetInf, "SubModel");

    if (!l_subModelInJson.empty() &&
        !l_subModelInJson.value("SubModel", "").empty())
    {
        l_fruObject.insert(l_subModelInJson.cbegin(), l_subModelInJson.cend());
    }

    // Get the properties under VINI interface.

    nlohmann::json l_viniPropertiesInJson = nlohmann::json::object({});

    auto l_readViniKeyWord = [i_objectPath, &l_viniPropertiesInJson,
                              this](const std::string& i_keyWord) {
        const nlohmann::json l_keyWordJson =
            getInventoryPropertyJson<vpd::types::BinaryVector>(
                i_objectPath, constants::kwdVpdInf, i_keyWord);
        l_viniPropertiesInJson.insert(l_keyWordJson.cbegin(),
                                      l_keyWordJson.cend());
    };

    const std::vector<std::string> l_viniKeywords = {"SN", "PN", "CC", "FN",
                                                     "DR"};

    std::for_each(l_viniKeywords.cbegin(), l_viniKeywords.cend(),
                  l_readViniKeyWord);

    if (!l_viniPropertiesInJson.empty())
    {
        l_fruObject.insert(l_viniPropertiesInJson.cbegin(),
                           l_viniPropertiesInJson.cend());
    }

    const auto l_typePropertyJson = getFruTypeProperty(i_objectPath);
    if (!l_typePropertyJson.empty())
    {
        l_fruObject.insert(l_typePropertyJson.cbegin(),
                           l_typePropertyJson.cend());
    }

    return l_fruJson;
}

template <typename PropertyType>
nlohmann::json VpdTool::getInventoryPropertyJson(
    const std::string& i_objectPath, const std::string& i_interface,
    const std::string& i_propertyName) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});
    try
    {
        types::DbusVariantType l_keyWordValue;

        l_keyWordValue =
            utils::readDbusProperty(constants::inventoryManagerService,
                                    i_objectPath, i_interface, i_propertyName);

        if (const auto l_value = std::get_if<PropertyType>(&l_keyWordValue))
        {
            if constexpr (std::is_same<PropertyType, std::string>::value)
            {
                l_resultInJson.emplace(i_propertyName, *l_value);
            }
            else if constexpr (std::is_same<PropertyType, bool>::value)
            {
                l_resultInJson.emplace(i_propertyName,
                                       *l_value ? "true" : "false");
            }
            else if constexpr (std::is_same<PropertyType,
                                            types::BinaryVector>::value)
            {
                const std::string& l_keywordStrValue =
                    vpd::utils::getPrintableValue(*l_value);

                l_resultInJson.emplace(i_propertyName, l_keywordStrValue);
            }
        }
        else
        {
            // TODO: Enable logging when verbose is enabled.
            // std::cout << "Invalid data type received." << std::endl;
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Read " << i_propertyName << " value for FRU path: " <<
           i_objectPath
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_resultInJson;
}

int VpdTool::fixSystemVpd() noexcept
{
    int l_rc = constants::FAILURE;
    // ToDo: Implementation needs to be added
    return l_rc;
}

int VpdTool::writeKeyword(std::string i_vpdPath,
                          const std::string& i_recordName,
                          const std::string& i_keywordName,
                          const std::string& i_keywordValue,
                          const bool i_onHardware) noexcept
{
    int l_rc = constants::FAILURE;
    try
    {
        if (i_vpdPath.empty() || i_recordName.empty() ||
            i_keywordName.empty() || i_keywordValue.empty())
        {
            throw std::runtime_error("Received input is empty.");
        }

        auto l_paramsToWrite =
            std::make_tuple(i_recordName, i_keywordName,
                            utils::convertToBinary(i_keywordValue));

        if (i_onHardware)
        {
            // ToDo: Update on hardware
        }
        else
        {
            i_vpdPath = constants::baseInventoryPath + i_vpdPath;
            l_rc = utils::writeKeyword(i_vpdPath, l_paramsToWrite);
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable log when verbose is enabled.
        std::cerr << "Write keyword's value for path: " << i_vpdPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << " is failed. Exception: " << l_ex.what() << std::endl;
    }
    return l_rc;
}

nlohmann::json VpdTool::getBackupRestoreCfgJsonObj() const noexcept
{
    nlohmann::json l_parsedBackupRestoreJson{};
    try
    {
        nlohmann::json l_parsedSystemJson =
            utils::getParsedJson(INVENTORY_JSON_SYM_LINK);

        // check for mandatory fields at this point itself.
        if (!l_parsedSystemJson.contains("backupRestoreConfigPath"))
        {
            throw std::runtime_error(
                "backupRestoreConfigPath tag is missing from system config JSON : " +
                std::string(INVENTORY_JSON_SYM_LINK));
        }

        l_parsedBackupRestoreJson =
            utils::getParsedJson(l_parsedSystemJson["backupRestoreConfigPath"]);
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        std::cerr << l_ex.what() << std::endl;
    }

    return l_parsedBackupRestoreJson;
}

int VpdTool::cleanSystemVpd() const noexcept
{
    try
    {
        // get the keyword map from backup_restore json
        // iterate through the keyword map get default value of
        // l_keywordName.
        // use writeKeyword API to update default value on hardware,
        // backup and D - Bus.
        const nlohmann::json l_parsedBackupRestoreJson =
            getBackupRestoreCfgJsonObj();

        // check for mandatory tags
        if (l_parsedBackupRestoreJson.contains("source") &&
            l_parsedBackupRestoreJson.contains("backupMap") &&
            l_parsedBackupRestoreJson["source"].contains("hardwarePath") &&
            l_parsedBackupRestoreJson["backupMap"].is_array())
        {
            // get the source hardware path
            const auto& l_hardwarePath =
                l_parsedBackupRestoreJson["source"]["hardwarePath"];

            // iterate through the backup map
            for (const auto& l_aRecordKwInfo :
                 l_parsedBackupRestoreJson["backupMap"])
            {
                // check if Manufacturing Reset is required for this entry
                const bool l_isMfgCleanRequired =
                    l_aRecordKwInfo.value("isManufactureResetRequired", false);

                if (l_isMfgCleanRequired)
                {
                    // get the Record name and Keyword name
                    const std::string& l_srcRecordName =
                        l_aRecordKwInfo.value("sourceRecord", "");
                    const std::string& l_srcKeywordName =
                        l_aRecordKwInfo.value("sourceKeyword", "");

                    // validate the Record name, Keyword name and the
                    // defaultValue
                    if (!l_srcRecordName.empty() && !l_srcKeywordName.empty() &&
                        l_aRecordKwInfo.contains("defaultValue") &&
                        l_aRecordKwInfo["defaultValue"].is_array())
                    {
                        const types::BinaryVector l_defaultBinaryValue =
                            l_aRecordKwInfo["defaultValue"]
                                .get<types::BinaryVector>();

                        // update the Keyword with default value, use D-Bus
                        // method UpdateKeyword exposed by vpd-manager.
                        // Note: writing to all paths (Primary EEPROM path,
                        // Secondary EEPROM path, D-Bus cache and Backup path)
                        // is the responsibility of vpd-manager's UpdateKeyword
                        // API
                        if (constants::FAILURE ==
                            utils::writeKeyword(
                                l_hardwarePath,
                                std::make_tuple(l_srcRecordName,
                                                l_srcKeywordName,
                                                l_defaultBinaryValue)))
                        {
                            // TODO: Enable logging when verbose
                            // is enabled.
                            std::cerr << "Failed to update " << l_srcRecordName
                                      << ":" << l_srcKeywordName << std::endl;
                        }
                    }
                    else
                    {
                        std::cerr << "Unrecognized Entry Record ["
                                  << l_srcRecordName << "] Keyword ["
                                  << l_srcKeywordName
                                  << "] in Backup Restore JSON backup map"
                                  << std::endl;
                    }
                } // mfgClean required check
            } // keyword list loop
        }
        else // backupRestoreJson is not valid
        {
            std::cerr << "Backup Restore JSON is not valid" << std::endl;
        }

        // success/failure message
        std::cout << "The critical keywords from system backplane VPD has "
                     "been reset successfully."
                  << std::endl;

    } // try block end
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        std::cerr
            << "Manufacturing reset on system vpd keywords is unsuccessful. Error : "
            << l_ex.what() << std::endl;
    }
    return constants::SUCCESS;
}

nlohmann::json
    VpdTool::getFruTypeProperty(const std::string& i_objectPath) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});
    std::vector<std::string> l_pimInfList;

    auto l_serviceInfMap = utils::GetServiceInterfacesForObject(
        i_objectPath, std::vector<std::string>{constants::inventoryItemInf});
    if (l_serviceInfMap.contains(constants::inventoryManagerService))
    {
        l_pimInfList = l_serviceInfMap[constants::inventoryManagerService];

        // iterate through the list and find
        // "xyz.openbmc_project.Inventory.Item.*"
        for (const auto& l_interface : l_pimInfList)
        {
            if (l_interface.find(constants::inventoryItemInf) !=
                    std::string::npos &&
                l_interface.length() >
                    std::string(constants::inventoryItemInf).length())
            {
                l_resultInJson.emplace("type", l_interface);
            }
        }
    }
    return l_resultInJson;
}

bool VpdTool::isFruPresent(const std::string& i_objectPath) const noexcept
{
    bool l_returnValue{false};
    try
    {
        types::DbusVariantType l_keyWordValue;

        l_keyWordValue = utils::readDbusProperty(
            constants::inventoryManagerService, i_objectPath,
            constants::inventoryItemInf, "Present");

        if (const auto l_value = std::get_if<bool>(&l_keyWordValue))
        {
            l_returnValue = *l_value;
        }
    }
    catch (const std::runtime_error& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        // std::cerr << "Failed to check \"Present\" property for FRU "
        //           << i_objectPath << " Error: " << l_ex.what() <<
        //           std::endl;
    }
    return l_returnValue;
}

} // namespace vpd

#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_types.hpp"
#include "tool_utils.hpp"

#include <iostream>
#include <tuple>

namespace vpd
{

// Map of system backplane records to list of keywords and its related data. {
//  Record : { Keyword, Default value, Is MFG
//  reset required, Backup Record, Backup Keyword }}
const types::SystemKeywordsMap VpdTool::g_systemVpdKeywordMap{
    {"VSYS",
     {types::SystemKeywordInfo("BR", types::BinaryVector{2, 0x20}, true, "VSBK",
                               "BR"),
      types::SystemKeywordInfo("TM", types::BinaryVector{8, 0x20}, true, "VSBK",
                               "TM"),
      types::SystemKeywordInfo("SE", types::BinaryVector{7, 0x20}, true, "VSBK",
                               "SE"),
      types::SystemKeywordInfo("SU", types::BinaryVector{6, 0x20}, true, "VSBK",
                               "SU"),
      types::SystemKeywordInfo("RB", types::BinaryVector{4, 0x20}, true, "VSBK",
                               "RB"),
      types::SystemKeywordInfo("WN", types::BinaryVector{12, 0x20}, true,
                               "VSBK", "WN"),
      types::SystemKeywordInfo("RG", types::BinaryVector{4, 0x20}, true, "VSBK",
                               "RG"),
      types::SystemKeywordInfo("FV", types::BinaryVector{32, 0x20}, true,
                               "VSBK", "FV")}},
    {"VCEN",
     {types::SystemKeywordInfo("FC", types::BinaryVector{8, 0x20}, false,
                               "VSBK", "FC"),
      types::SystemKeywordInfo("SE", types::BinaryVector{7, 0x20}, true, "VSBK",
                               "ES")}},
    {"LXR0",
     {types::SystemKeywordInfo("LX", types::BinaryVector{8, 0x00}, false,
                               "VSBK", "LX")}},
    {"UTIL",
     {types::SystemKeywordInfo("D0", types::BinaryVector{1, 0x00}, true, "VSBK",
                               "D0"),
      types::SystemKeywordInfo("D1", types::BinaryVector{1, 0x00}, true, "VSBK",
                               "D1"),
      types::SystemKeywordInfo("F0", types::BinaryVector{8, 0x00}, true, "VSBK",
                               "F0"),
      types::SystemKeywordInfo("F5", types::BinaryVector{16, 0x00}, true,
                               "VSBK", "F5"),
      types::SystemKeywordInfo("F6", types::BinaryVector{16, 0x00}, true,
                               "VSBK", "F6")}}};

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
        const nlohmann::json l_resultJson = getFruProperties(i_fruPath);

        utils::printJson(l_resultJson);
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

nlohmann::json VpdTool::getFruProperties(const std::string& i_fruPath) const
{
    nlohmann::json l_resultInJson = nlohmann::json::array({});

    // TODO: Implement getFruProperties
    (void)i_fruPath;
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
int VpdTool::cleanSystemVpd() const noexcept
{
    int l_rc{constants::FAILURE};
    try
    {
        // TODO:
        //     iterate through the keyword map get default value of
        //     l_keywordName.
        //     use readKeyword API to read hardware value from hardware.
        //     if hardware value != default value,
        //     use writeKeyword API to update default value on hardware,
        //     backup and D - Bus.
        (void)g_systemVpdKeywordMap;

        l_rc = constants::SUCCESS;
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        // std::cerr << "Manufacturing clean failed for " << i_recordName << ":"
        //           << i_keywordName << ". Error : " << l_ex.what() <<
        //           std::endl;
    }

    return l_rc;
}

} // namespace vpd

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
                nlohmann::json l_resultInJson = nlohmann::json::object({});
                nlohmann::json l_keywordValInJson = nlohmann::json::object({});

                l_keywordValInJson.emplace(i_keywordName, l_keywordStrValue);
                l_resultInJson.emplace(i_vpdPath, l_keywordValInJson);

                utils::printJson(l_resultInJson);
            }
            else
            {
                // TODO: Write result to a given file path.
            }
            l_rc = constants::SUCCESS;
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

int VpdTool::writeKeyword(const std::string& i_vpdPath,
                          const std::string& i_recordName,
                          const std::string& i_keywordName,
                          const std::string& i_keywordValue,
                          const bool i_onHardware,
                          const std::string& i_filePath)
{
    int l_rc = -1;
    try
    {
        if (i_vpdPath.empty() || i_recordName.empty() || i_keywordName.empty())
        {
            throw std::runtime_error("Received input is empty.");
        }

        if (!i_filePath.empty())
        {
            // ToDo: Take keyword value from the file
        }

        std::string l_vpdPath{i_vpdPath};
        if (!i_onHardware)
        {
            l_vpdPath = constants::baseInventoryPath + i_vpdPath;
        }

        auto l_paramsToWrite =
            std::make_tuple(i_recordName, i_keywordName,
                            utils::convertToBinary(i_keywordValue));
        l_rc = utils::writeKeyword(
            constants::vpdManagerService, constants::vpdManagerObjectPath,
            constants::vpdManagerInfName, l_vpdPath, l_paramsToWrite);
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Write keyword's value for path: " << i_vpdPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << " is failed. Exception: " << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

int VpdTool::cleanSystemVpd(const std::string& i_recordName,
                            const std::string& i_keywordName) const noexcept
{
    int l_rc{-1};

    // Map of Keywords which can be reset by vpd-tool manufacturing clean
    // option
    // {Record1 : {{Keyword1, Default value1},{Keyword2, Default value2} }},
    // {Record2 : {{Keyword1, Default value1},{Keyword2, Default value2} }}
    static const types::MfgCleanRecordMap l_mfgCleanKeywordMap{
        {"UTIL",
         {{"D0", types::BinaryVector{1, 0x00}},
          {"D1", types::BinaryVector{1, 0x00}},
          {"F0", types::BinaryVector{8, 0x00}},
          {"F5", types::BinaryVector{16, 0x00}},
          {"F6", types::BinaryVector{16, 0x00}}}},
        {"VSYS",
         {{"BR", types::BinaryVector{2, 0x20}},
          {"TM", types::BinaryVector{8, 0x20}},
          {"RG", types::BinaryVector{4, 0x20}},
          {"SE", types::BinaryVector{7, 0x20}},
          {"SU", types::BinaryVector{6, 0x20}},
          {"RB", types::BinaryVector{4, 0x20}},
          {"WN", types::BinaryVector{12, 0x20}},
          {"FV", types::BinaryVector{32, 0x20}}}},
        {"VCEN", {{"SE", types::BinaryVector{7, 0x20}}}}};

    /* TODO:
        search l_recordName in map
       search l_keywordName in map
       get default value of l_keywordName
       use readKeyword API to read hardware value from hardware
       if hardware value != default value,
        use writeKeyword API to update default value on hardware, backup and
       D-Bus*/

    (void)i_recordName;
    (void)i_keywordName;

    return l_rc;
}

} // namespace vpd

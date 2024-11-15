#include "vpd_tool.hpp"

#include "constants.hpp"
#include "types.hpp"
#include "utils.hpp"

#include <iostream>

namespace vpd
{
int VpdTool::readKeyword(const std::string& i_fruPath,
                         const std::string& i_recordName,
                         const std::string& i_keywordName,
                         const bool i_onHardware,
                         const std::string& i_fileToSave)
{
    int l_rc = -1;
    try
    {
        types::DbusVariantType l_keywordValue;
        if (i_onHardware)
        {
            // TODO: uncomment when implementation of the API goes in
            /*l_keywordValue = utils::readKeywordFromHardware(
                constants::vpdManagerService,
               constants::vpdManagerObjectPath,
                constants::vpdManagerIntfName, i_fruPath,
                make_tuple(i_recordName, i_keywordName));*/
        }
        else
        {
            std::string l_inventoryObjectPath(constants::baseInventoryPath +
                                              i_fruPath);

            l_keywordValue = utils::readDbusProperty(
                constants::inventoryManagerService, l_inventoryObjectPath,
                constants::ipzVpdInf + i_recordName, i_keywordName);
        }

        if (const auto l_value =
                std::get_if<types::BinaryVector>(&l_keywordValue))
        {
            const std::string& l_keywordStrValue =
                utils::getPrintableValue(*l_value);

            if (i_fileToSave.empty())
            {
                nlohmann::json l_resultInJson = nlohmann::json::object({});
                nlohmann::json l_keywordValInJson = nlohmann::json::object({});

                l_keywordValInJson.emplace(i_keywordName, l_keywordStrValue);
                l_resultInJson.emplace(i_fruPath, l_keywordValInJson);

                utils::printJson(l_resultInJson);
            }
            else
            {
                // TODO: Write result to a given file path.
            }
            l_rc = 0;
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
        /*std::cerr << "Read keyword's value for FRU path: " << i_fruPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

int VpdTool::dumpObject(const std::string& i_fruPath,
                        nlohmann::json::object_t& io_resultJson) const noexcept
{
    int l_rc{-1};
    try
    {
        const auto l_presentPropertyInJson = getPresentPropertyJson(i_fruPath);
        if (!l_presentPropertyInJson.empty())
        {
            io_resultJson.emplace(i_fruPath, l_presentPropertyInJson);
            l_rc = 0;
        }

        const auto l_prettyNameInJson = getPrettyNameJson(i_fruPath);
        if (!l_prettyNameInJson.empty())
        {
            if (io_resultJson.contains(i_fruPath))
            {
                io_resultJson[i_fruPath].insert(l_prettyNameInJson.begin(),
                                                l_prettyNameInJson.end());
            }
            else
            {
                io_resultJson.emplace(i_fruPath, l_prettyNameInJson);
            }
            l_rc = 0;
        }

        const auto l_viniPropertiesInJson = getVINIPropertiesJson(i_fruPath);
        if (!l_viniPropertiesInJson.empty())
        {
            if (io_resultJson.contains(i_fruPath))
            {
                io_resultJson[i_fruPath].insert(l_viniPropertiesInJson.begin(),
                                                l_viniPropertiesInJson.end());
            }
            else
            {
                io_resultJson.emplace(i_fruPath, l_viniPropertiesInJson);
            }
            l_rc = 0;
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Dump Object failed for FRU " << i_fruPath <<
        " due to error:" << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

nlohmann::json
    VpdTool::getPresentPropertyJson(const std::string& i_fruPath) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});
    try
    {
        types::DbusVariantType l_keyWordValue;
        const std::string l_inventoryObjectPath{constants::baseInventoryPath +
                                                i_fruPath};
        l_keyWordValue = utils::readDbusProperty(
            constants::inventoryManagerService, l_inventoryObjectPath,
            constants::inventoryItemInf, "Present");

        if (const auto l_value = std::get_if<bool>(&l_keyWordValue))
        {
            l_resultInJson.emplace("Present", *l_value ? "true" : "false");
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
        /*std::cerr << "Read Present property value for FRU path: " << i_fruPath
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_resultInJson;
}

nlohmann::json
    VpdTool::getPrettyNameJson(const std::string& i_fruPath) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});
    try
    {
        types::DbusVariantType l_keyWordValue;
        const std::string l_inventoryObjectPath{constants::baseInventoryPath +
                                                i_fruPath};
        l_keyWordValue = utils::readDbusProperty(
            constants::inventoryManagerService, l_inventoryObjectPath,
            constants::inventoryItemInf, "PrettyName");

        if (const auto l_value = std::get_if<std::string>(&l_keyWordValue))
        {
            l_resultInJson.emplace("PrettyName", *l_value);
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
        /*std::cerr << "Read Pretty Name value for FRU path: " << i_fruPath
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_resultInJson;
}

nlohmann::json
    VpdTool::getVINIPropertiesJson(const std::string& i_fruPath) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});

    try
    {
        const std::array<std::string, 5> l_viniKeyWords{"SN", "PN", "CC", "FN",
                                                        "DR"};

        auto l_readKeyWord = [i_fruPath,
                              &l_resultInJson](const std::string& i_keyWord) {
            const std::string l_inventoryObjectPath{
                constants::baseInventoryPath + i_fruPath};

            const types::DbusVariantType l_keyWordValue =
                utils::readDbusProperty(constants::inventoryManagerService,
                                        l_inventoryObjectPath,
                                        constants::kwdVpdInf, i_keyWord);

            if (const auto l_value =
                    std::get_if<types::BinaryVector>(&l_keyWordValue))
            {
                const std::string& l_keywordStrValue =
                    utils::getPrintableValue(*l_value);

                l_resultInJson.emplace(i_keyWord, l_keywordStrValue);
            }
            else
            {
                // TODO: Enable logging when verbose is enabled.
                // std::cout << "Invalid data type received." << std::endl;
            }
        };

        std::for_each(l_viniKeyWords.cbegin(), l_viniKeyWords.cend(),
                      l_readKeyWord);
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Read VINI property for FRU path: " << i_fruPath
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_resultInJson;
}

} // namespace vpd

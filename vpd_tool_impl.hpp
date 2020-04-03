#include "config.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sstream>
#include <variant>
#include <vector>

using namespace std;
using Binary = vector<uint8_t>;
using json = nlohmann::json;

class VpdTool
{
  private:
    const std::string fruPath;
    const std::string recordName;
    const std::string keyword;
    const std::string value;

  public:
    /**
     * @brief Dump the complete inventory in JSON format
     *
     * @param[in] jsObject - Inventory JSON specified in configure file.
     */
    void dumpInventory(nlohmann::basic_json<>& jsObject);

    /**
     * @brief Dump the given inventory object in JSON format
     *
     * @param[in] jsObject - Inventory JSON specified in configure file.
     */
    void dumpObject(nlohmann::basic_json<>& jsObject);

    /**
     * @brief Read keyword
     *
     * Read the given object path, record name and keyword
     * from the inventory and display the value of the keyword
     * in JSON format.
     */
    void readKeyword();

    /**
     * @brief Update Keyword
     *
     * Update the given keyword with the given value.
     *
     * @return return code (Success(0)/Failure(-1))
     */
    int updateKeyword();

    /**
     * @brief A Constructor
     *
     * Constructor is called during the
     * object instantiation for dumpInventory option.
     *
     * @return return code (success/failure)
     */
    VpdTool()
    {
    }

    /**
     * @brief Move Constructor
     *
     * Move constructor is called during the
     * object instantiation for dumpObject option.
     */
    VpdTool(const string&& fru) : fruPath(move(fru))
    {
    }

    /**
     * @brief Move Constructor
     *
     * Move constructor is called during the
     * object instantiation for readKeyword option.
     */
    VpdTool(const string&& fru, const string&& recName, const string&& kw) :
        fruPath(move(fru)), recordName(move(recName)), keyword(move(kw))
    {
    }

    /**
     * @brief Constructor
     *
     * Constructor is called during the
     * object instantiation for updateKeyword option.
     */

    VpdTool(const std::string&& fru, const std::string&& recName,
            const std::string&& kw, const std::string&& val) :
        fruPath(std::move(fru)),
        recordName(std::move(recName)), keyword(std::move(kw)),
        value(std::move(val))
    {
    }
};

#pragma once

#include <cstdint>

namespace vpd
{
namespace constants
{
static constexpr auto KEYWORD_SIZE = 2;
static constexpr auto RECORD_SIZE = 4;

constexpr auto vpdManagerService = "com.ibm.VPD.Manager";
constexpr auto vpdManagerObjectPath = "/com/ibm/VPD/Manager";
constexpr auto vpdManagerIntfName = "com.ibm.VPD.Manager";

constexpr auto inventoryManagerService =
    "xyz.openbmc_project.Inventory.Manager";
constexpr auto baseInventoryPath = "/xyz/openbmc_project/inventory";
constexpr auto ipzVpdInf = "com.ibm.ipzvpd.";
constexpr auto inventoryItemInf = "xyz.openbmc_project.Inventory.Item";
constexpr auto kwdVpdInf = "com.ibm.ipzvpd.VINI";
} // namespace constants
} // namespace vpd

#pragma once

#include <iostream>
namespace vpd
{
namespace constants
{
static constexpr auto FORMAT_11S_LEN = 3;
static constexpr auto MEMORY_VPD_DATA_START = 416;
static constexpr int IPZ_DATA_START = 11;
static constexpr uint8_t KW_VAL_PAIR_START_TAG = 0x84;
static constexpr int KW_VPD_DATA_START = 0;
static constexpr uint8_t KW_VPD_START_TAG = 0x82;
static constexpr auto MEMORY_VPD_START_TAG = "11S";
static constexpr auto SPD_BYTE_2 = 2;
static constexpr auto SPD_BYTE_3 = 3;
static constexpr auto SPD_BYTE_4 = 4;
static constexpr auto SPD_BYTE_6 = 6;
static constexpr auto SPD_BYTE_12 = 12;
static constexpr auto SPD_BYTE_13 = 13;
static constexpr auto SPD_BYTE_BIT_0_3_MASK = 0x0F;
static constexpr auto SPD_BYTE_MASK = 0xFF;
static constexpr auto SPD_MODULE_TYPE_DDIMM = 0x0A;
static constexpr auto SPD_DRAM_TYPE_DDR5 = 0x12;
static constexpr auto SPD_DRAM_TYPE_DDR4 = 0x0C;

} // namespace constants
} // namespace vpd
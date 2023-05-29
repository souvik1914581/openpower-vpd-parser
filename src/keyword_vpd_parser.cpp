#include "keyword_vpd_parser.hpp"

#include "constants.hpp"
#include "exceptions.hpp"
#include "logger.hpp"

#include <iostream>
#include <numeric>
#include <string>

namespace vpd
{

types::VPDMapVariant KeywordVpdParser::parse()
{
    try
    {
        m_vpdIterator = m_keywordVpdVector.begin();

        if (*m_vpdIterator != constants::KW_VPD_START_TAG)
        {
            throw(
                DataException("Invalid Large resource type Identifier String"));
        }

        checkNextBytesValidity(sizeof(constants::KW_VPD_START_TAG));
        std::advance(m_vpdIterator, sizeof(constants::KW_VPD_START_TAG));

        uint16_t dataSize = getKwDataSize();

        checkNextBytesValidity(constants::TWO_BYTES + dataSize);
        std::advance(m_vpdIterator, constants::TWO_BYTES + dataSize);

        // Check for invalid vendor defined large resource type
        if (*m_vpdIterator != constants::KW_VPD_PAIR_START_TAG)
        {
            if (*m_vpdIterator != constants::ALT_KW_VPD_PAIR_START_TAG)
            {
                throw(DataException("Invalid Keyword Vpd Start Tag"));
            }
        }
        auto kwValMap = populateVpdMap();

        // Do these validations before returning parsed data.
        // Check for small resource type end tag
        if (*m_vpdIterator != constants::KW_VAL_PAIR_END_TAG)
        {
            throw(DataException("Invalid Small resource type End"));
        }

        // Check vpd end Tag.
        if (*m_vpdIterator != constants::KW_VPD_END_TAG)
        {
            throw(DataException("Invalid Small resource type."));
        }
        return kwValMap;
    }
    catch (const DataException& ex)
    {
        logging::logMessage(ex.what());

        throw(DataException("VPD is corrupted, need to fix it."));
    }
}

types::KeywordVpdMap KeywordVpdParser::populateVpdMap()
{
    types::BinaryVector::const_iterator checkSumStart = m_vpdIterator;
    checkNextBytesValidity(constants::ONE_BYTE);
    std::advance(m_vpdIterator, constants::ONE_BYTE);

    auto totalSize = getKwDataSize();
    if (totalSize == 0)
    {
        throw(DataException("Data size is 0, badly formed keyword VPD"));
    }

    checkNextBytesValidity(constants::TWO_BYTES);
    std::advance(m_vpdIterator, constants::TWO_BYTES);

    types::KeywordVpdMap kwValMap;

    // Parse the keyword-value and store the pairs in map
    while (totalSize > 0)
    {
        checkNextBytesValidity(constants::TWO_BYTES);
        std::string keywordName(m_vpdIterator,
                                m_vpdIterator + constants::TWO_BYTES);
        std::advance(m_vpdIterator, constants::TWO_BYTES);

        size_t kwSize = *m_vpdIterator;
        checkNextBytesValidity(kwSize);
        m_vpdIterator++;
        std::vector<uint8_t> valueBytes(m_vpdIterator, m_vpdIterator + kwSize);
        std::advance(m_vpdIterator, kwSize);

        kwValMap.emplace(
            std::make_pair(std::move(keywordName), std::move(valueBytes)));

        totalSize -= constants::TWO_BYTES + constants::ONE_BYTE + kwSize;
    }

    types::BinaryVector::const_iterator checkSumEnd = m_vpdIterator;

    validateChecksum(checkSumStart, checkSumEnd);

    return kwValMap;
}

void KeywordVpdParser::validateChecksum(
    types::BinaryVector::const_iterator checkSumStart,
    types::BinaryVector::const_iterator checkSumEnd)
{
    uint8_t checkSumCalculated = 0;

    // Checksum calculation
    checkSumCalculated =
        std::accumulate(checkSumStart, checkSumEnd, checkSumCalculated);
    checkSumCalculated = ~checkSumCalculated + 1;
    uint8_t checksumVpdValue = *(m_vpdIterator + constants::ONE_BYTE);

    if (checkSumCalculated != checksumVpdValue)
    {
        throw(DataException("Invalid Checksum"));
    }

    checkNextBytesValidity(constants::TWO_BYTES);
    std::advance(m_vpdIterator, constants::TWO_BYTES);
}

void KeywordVpdParser::checkNextBytesValidity(uint8_t numberOfBytes)
{
    if ((std::distance(m_keywordVpdVector.begin(),
                       m_vpdIterator + numberOfBytes)) >
        std::distance(m_keywordVpdVector.begin(), m_keywordVpdVector.end()))
    {
        throw(DataException("Truncated VPD data"));
    }
}

} // namespace vpd
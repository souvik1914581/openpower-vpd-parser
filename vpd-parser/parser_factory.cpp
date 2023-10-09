#include "parser_factory.hpp"

#include "const.hpp"
#include "ibm_vpd_utils.hpp"
#include "ipz_parser.hpp"
#include "isdimm_vpd_parser.hpp"
#include "keyword_vpd_parser.hpp"
#include "memory_vpd_parser.hpp"
#include "vpd_exceptions.hpp"

using namespace vpd::keyword::parser;
using namespace openpower::vpd::memory::parser;
using namespace openpower::vpd::isdimm::parser;
using namespace openpower::vpd::parser::interface;
using namespace openpower::vpd::ipz::parser;
using namespace openpower::vpd::exceptions;
using namespace openpower::vpd::constants;

namespace openpower
{
namespace vpd
{
namespace parser
{
namespace factory
{
interface::ParserInterface* ParserFactory::getParser(
    const Binary& vpdVector, const std::string& inventoryPath,
    const std::string& vpdFilePath, uint32_t vpdStartOffset)
{
    std::cout<<" F 1"<<std::endl;
    vpdType type = vpdTypeCheck(vpdVector);

    switch (type)
    {
        case IPZ_VPD:
        {
            std::cout<<" F 2"<<std::endl;
            return new IpzVpdParser(vpdVector, inventoryPath, vpdFilePath,
                                    vpdStartOffset);
        }

        case KEYWORD_VPD:
        {
            std::cout<<" F 3"<<std::endl;
            return new KeywordVpdParser(vpdVector);
        }

        case DDR4_DDIMM_MEMORY_VPD:
        case DDR5_DDIMM_MEMORY_VPD:
        {
            std::cout<<" F 4"<<std::endl;
            return new memoryVpdParser(vpdVector);
        }

        case DDR4_ISDIMM_MEMORY_VPD:
        case DDR5_ISDIMM_MEMORY_VPD:
        {
            std::cout<<" F 5"<<std::endl;
            return new isdimmVpdParser(vpdVector);
        }

        default:
            throw VpdDataException("Unable to determine VPD format");
    }
    std::cout<<" F 6"<<std::endl;
}

void ParserFactory::freeParser(interface::ParserInterface* parser)
{
    if (parser)
    {
        delete parser;
        parser = nullptr;
    }
}

} // namespace factory
} // namespace parser
} // namespace vpd
} // namespace openpower

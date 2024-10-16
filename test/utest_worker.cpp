#include "exceptions.hpp"
#include "keyword_vpd_parser.hpp"
#include "types.hpp"
#include "worker.hpp"

#include <exception>
#include <fstream>

#include <gtest/gtest.h>

using namespace vpd;

TEST(WorkerTest, PostFailActionIfPreActionSuccessButVpdParsingFails)
{
    const std::string l_vpdFile{"vpd_files/keyword_corrupted_index_0.dat"};
    const std::string l_configJson{"test_configurations/50001001.json"};
    std::shared_ptr<Worker> l_objWorker = std::make_shared<Worker>();
    vpd::types::VPDMapVariant l_parsedVpdDataMap =
        l_objWorker->parseVpdFile(l_vpdFile);
}

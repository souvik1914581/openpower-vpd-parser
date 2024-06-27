#include "backup_restore.hpp"

namespace vpd
{
BackupRestore::BackupRestore(const nlohmann::json& i_parsedSystemJson)
{
    (void)i_parsedSystemJson;
}

std::tuple<types::VPDMapVariant, types::VPDMapVariant>
    BackupRestore::backupAndRestore()
{
    return std::make_tuple(std::monostate{}, std::monostate{});
}

void BackupRestore::backupAndRestoreIpzVpd(types::IPZVpdMap& io_srcVpdMap,
                                           types::IPZVpdMap& io_dstVpdMap,
                                           const std::string& i_srcPath,
                                           const std::string& i_dstPath)
{
    (void)io_srcVpdMap;
    (void)io_dstVpdMap;
    (void)i_srcPath;
    (void)i_dstPath;
}
} // namespace vpd

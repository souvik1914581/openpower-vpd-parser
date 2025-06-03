#!/usr/bin/python3
import json
import os
import shutil
import copy

SYSTEM_CONFIG_JSON_PATH = "/usr/share/vpd/50001000_v2.json"
NEW_SYSTEM_CONFIG_JSON_PATH = "/tmp/new_vpd_inventory.json"
#SYM_LINK = "/var/lib/vpd/vpd_inventory.json"
SYM_LINK="/tmp/vpd_inventory.json"
EEPROM_BASE_PATH = "/tmp/eeproms"
BASE_INVENTORY_PATH = "/xyz/openbmc_project/inventory/system/chassis/motherboard"
VALID_EEPROM = "/sys/bus/i2c/drivers/at24/0-0051/eeprom"
VALID_FILE_COUNT = 10

INV_DATA = {
    "inventoryPath": "/xyz/openbmc_project/inventory/system/chassis/motherboard/test_fru1",
    "serviceName": "xyz.openbmc_project.Inventory.Manager",
    "extraInterfaces": {
        "com.ibm.ipzvpd.Location": {
            "LocationCode": "Ufcs-P0-C5"
        },
        "xyz.openbmc_project.Inventory.Item": {
            "PrettyName": "Test FRU 1"
        }
    }
}
FRU_DATA = {"/sys/bus/i2c/drivers/at24/200-0055/eeprom": INV_DATA}

def add_frus_to_json(fru_details, file_path):
    with open(SYSTEM_CONFIG_JSON_PATH, "r") as file:
        file_data = json.load(file)

        file_data["frus"].update(fru_details)

        file.seek(0)
        with open(file_path, "w") as f:
            json.dump(file_data, f, indent=4)


def create_sym_link(src, dest):
    try:
        os.symlink(src, dest)
    except FileExistsError:
        print(f"link {dest} already exists, deleting and creating again !")
        os.remove(dest)
        os.symlink(src, dest)

def create_valid_eeproms():
    #os.makedirs(EEPROM_BASE_PATH, exist_ok=True)
    valid_frus = {}

    for i in range(0, VALID_FILE_COUNT):
        eeprom_path = f"{EEPROM_BASE_PATH}/200-{i:04x}/eeprom"
        os.makedirs(os.path.dirname(eeprom_path), exist_ok=True)
        shutil.copy(VALID_EEPROM, eeprom_path)

        inv_path = f"{BASE_INVENTORY_PATH}/test_fru{i}"

        sub_fru = INV_DATA.copy()
        sub_fru["inventoryPath"] = inv_path
        valid_frus[eeprom_path] = [sub_fru]
        
    print(json.dumps(valid_frus))
    add_frus_to_json(valid_frus, NEW_SYSTEM_CONFIG_JSON_PATH)

def main():
    create_sym_link(NEW_SYSTEM_CONFIG_JSON_PATH, SYM_LINK)
    create_valid_eeproms()

if __name__=="__main__":
    main()
#!/usr/bin/python3
import json
import os
import shutil
import copy

SYSTEM_CONFIG_JSON_PATH = "/usr/share/vpd/50001000_v2.json"
NEW_SYSTEM_CONFIG_JSON_PATH = "/tmp/vpd_inventory.json"
SYM_LINK = "/var/lib/vpd/vpd_inventory.json"
#SYM_LINK="/tmp/vpd_inventory_link.json"
EEPROM_BASE_PATH = "/tmp/valid_eeproms"
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

def get_fru_config(device_id):
    sub_fru = copy.deepcopy(INV_DATA)
    eeprom_path = f"{EEPROM_BASE_PATH}/200-{device_id:04x}/eeprom"
    sub_fru["inventoryPath"] = f"{BASE_INVENTORY_PATH}/valid_fru_{device_id}"
    sub_fru["extraInterfaces"]["xyz.openbmc_project.Inventory.Item"]["PrettyName"] = f"Test FRU {device_id}"

    return {eeprom_path: [sub_fru]}

def get_verify_info(eeprom, field_to_check):
    return (eeprom, "valid_eeprom", field_to_check, "verify_not_started")

def create_valid_eeproms():
    valid_frus = {}
    valid_eeprom_info = []

    for i in range(0, VALID_FILE_COUNT):
        eeprom_path = f"{EEPROM_BASE_PATH}/200-{i:04x}/eeprom"
        os.makedirs(os.path.dirname(eeprom_path), exist_ok=True)
        shutil.copy(VALID_EEPROM, eeprom_path)

        fru = get_fru_config(i)
        valid_frus.update(fru)
        valid_eeprom_info.append(get_verify_info(next(iter(fru)), "completed"))


    #print(json.dumps(valid_frus))
    print(json.dumps(valid_eeprom_info))

    return (valid_frus, valid_eeprom_info)

def main():
    create_sym_link(NEW_SYSTEM_CONFIG_JSON_PATH, SYM_LINK)
    create_valid_eeproms()

if __name__=="__main__":
    main()
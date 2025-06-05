#!/usr/bin/env python3

import copy
import os
import shutil

from generate_valid_eeproms import *

EEPROM_BASE_PATH = "/tmp/eeproms"
ipzSrcFilePath = "/sys/bus/i2c/drivers/at24/7-0051/eeprom"
dimmSrcFilePath = "/sys/bus/i2c/drivers/at24/110-0050/eeprom"
BASE_INVENTORY_PATH = "/xyz/openbmc_project/inventory/system/chassis/motherboard"

#(error_name, eeprom_file, offset, bytes_to_update, pel_type)
corrupt_format = [("missingVtoc", ipzSrcFilePath, 61, b'\x49\x56', "ERROR"),
                        ("missingHeader", ipzSrcFilePath, 17, b'\x49\x56', "ERROR"),
                        ("invalidRecordOffset", ipzSrcFilePath, 74, b'\x49\x56', "ERROR"),
                        ("invalidDdrType", dimmSrcFilePath, 2, b'\x49', "ERROR"),
                        ("invalidDensityPerDie", dimmSrcFilePath, 4, b'\x49\x56', "ERROR"),
                        ("invalidVpdType", dimmSrcFilePath, 2, b'\x49\x56', "ERROR"),
                        ("ZeroDdimmSize", dimmSrcFilePath, 235, b'\x00', "ERROR")]

def createCorruptedFiles(src_file_path, dest_name, offset, new_bytes):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
    shutil.copy(src_file_path, dest_file_path)

    fs = open(dest_file_path, "rb+")
    fs.seek(offset)
    fs.write(new_bytes)

    fs.close()


def truncateFile(src_file_path, dest_name, offset):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
    shutil.copy(src_file_path, dest_file_path)

    with open(dest_file_path, "rb+") as fs:
        fs.truncate(offset)


def emptyFile(dest_name):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_name, "eeprom")
    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)

    with open(dest_file_path, "wb+") as fs:
        pass


def get_fru_config(name):
    sub_fru = copy.deepcopy(INV_DATA)
    eeprom_path = f"{EEPROM_BASE_PATH}/{name}/eeprom"
    sub_fru["inventoryPath"] = f"{BASE_INVENTORY_PATH}/invalid_fru_{name}"
    sub_fru["extraInterfaces"]["xyz.openbmc_project.Inventory.Item"][
        "PrettyName"
    ] = f"Test FRU {name}"

    return {eeprom_path: [sub_fru]}


def get_verify_info(eeprom, field_to_check):
    return (eeprom, "invalid_eeprom", field_to_check, "verify_not_started")


def createInvalidEeproms():
    invalid_frus = {}
    invalid_eeprom_info = []

    truncateFile(ipzSrcFilePath, "truncated", 40)
    fru = get_fru_config("truncated")
    invalid_frus.update(fru)
    invalid_eeprom_info.append(get_verify_info(next(iter(fru)), "ERROR"))

    emptyFile("emptyFile")
    fru = get_fru_config("emptyFile")
    invalid_frus.update(fru)
    invalid_eeprom_info.append(get_verify_info(next(iter(fru)), "ERROR"))

    for file_name, eeprom_path, offset, bytes, error_type in corrupt_format:
        createCorruptedFiles(eeprom_path, file_name, offset, bytes)
        fru = get_fru_config(file_name)
        invalid_frus.update(fru)
        invalid_eeprom_info.append(
            get_verify_info(next(iter(fru)), error_type)
        )

    # print(json.dumps(invalid_frus))
    print(json.dumps(invalid_eeprom_info))

    return (invalid_frus, invalid_eeprom_info)


if __name__ == "__main__":
    createInvalidEeproms()

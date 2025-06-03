#!/usr/bin/env python3

import shutil
import os

EEPROM_BASE_PATH = "/tmp/eeproms"
ipzSrcFilePath = "/sys/bus/i2c/drivers/at24/7-0051/eeprom"
dimmSrcFilePath = "/sys/bus/i2c/drivers/at24/110-0050/eeprom"

def createCorruptedFiles(src_file_path, dest_file_name, offset, new_bytes):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_file_name)
    shutil.copy(src_file_path, dest_file_path)

    fs= open(dest_file_path, "rb+")
    fs.seek(offset)
    fs.write(new_bytes)

    fs.close()

def truncateFile(src_file_path, dest_file_name, offset):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_file_name)
    shutil.copy(src_file_path, dest_file_path)

    with open(dest_file_path, "rb+") as fs:
        fs.truncate(offset)

def emptyFile(dest_file_name):
    dest_file_path = os.path.join(EEPROM_BASE_PATH, dest_file_name)

    with open(dest_file_path, "wb+") as fs:
        pass

def createInvalidFiles():
    truncateFile(ipzSrcFilePath, "truncated", 100)
    createCorruptedFiles(ipzSrcFilePath, "missingVtoc", 61, b'\x49\x56')
    emptyFile("emptyFile")
    createCorruptedFiles(ipzSrcFilePath, "missingHeader", 17, b'\x49\x56')
    truncateFile(ipzSrcFilePath, "malformedVpd", 43)
    createCorruptedFiles(ipzSrcFilePath, "invalidRecordOffset", 74, b'\x49\x56')
    createCorruptedFiles(dimmSrcFilePath, "invalidDdrType", 2, b'\x49')
    createCorruptedFiles(dimmSrcFilePath, "invalidDensityPerDie", 4, b'\x49\x56')
    createCorruptedFiles(dimmSrcFilePath, "invalidVpdType", 2, b'\x49\x56')
    createCorruptedFiles(dimmSrcFilePath, "ZeroDdimmSize", 235, b'\x00')
    
    
if __name__=="__main__":
    if not os.path.isdir(EEPROM_BASE_PATH):
        os.mkdir(EEPROM_BASE_PATH)
    
    createInvalidFiles()
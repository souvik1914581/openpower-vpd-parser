#!/usr/bin/env python3

import shutil

from generate_corrupted_eeproms import *
from generate_valid_eeproms import *

VERIFY_FILE_PATH = "/tmp/verify_report.txt"


def upadate_verify_info(verify_data):
    with open(VERIFY_FILE_PATH, "w") as f:
        json.dump(verify_data, f, indent=4)


def main():
    fru_data = {}

    shutil.copy(SYSTEM_CONFIG_JSON_PATH, SYSTEM_CONFIG_JSON_PATH + "_bkp")
    # create_sym_link(SYSTEM_CONFIG_JSON_PATH, SYM_LINK)

    valid_frus_map, valid_verify_data = create_valid_eeproms()
    invalid_frus_map, invalid_verify_data = createInvalidEeproms()

    fru_data.update(valid_frus_map)
    fru_data.update(invalid_frus_map)

    add_frus_to_json(fru_data, SYSTEM_CONFIG_JSON_PATH)
    upadate_verify_info(valid_verify_data + invalid_verify_data)


if __name__ == "__main__":
    main()

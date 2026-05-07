# VPD Configuration Generator Tool

A Python-based GUI tool for generating IBM OpenBMC VPD (Vital Product Data) system configuration JSON files.

## Features

- **Intuitive GUI**: Built with tkinter, runs natively on macOS
- **Hierarchical Menu System**: Navigate through configuration sections easily
- **Field Validation**: Real-time validation of user inputs
  - Hex address validation (e.g., 0xE0)
  - I2C bus number validation
  - File path validation
  - D-Bus interface name validation
  - Boolean value validation
- **JSON Structure Validation**: Validates the complete configuration structure
- **Template Loading**: Load existing configurations as templates
- **Visual Configuration**: View and edit all configuration sections
- **Export Functionality**: Export validated configurations to JSON files

## Requirements

- Python 3.7 or higher
- tkinter (usually included with Python)
- No additional dependencies required!

## Installation

1. Ensure Python 3.7+ is installed:
   ```bash
   python3 --version
   ```

2. Make the script executable:
   ```bash
   chmod +x vpd_config_generator.py
   ```

## Usage

### Starting the Tool

```bash
python3 vpd_config_generator.py
```

Or if made executable:
```bash
./vpd_config_generator.py
```

### Workflow

1. **Create New Configuration or Load Template**
   - Start with a blank configuration
   - Or load an existing JSON file as a template

2. **Configure Basic Settings**
   - Device Tree: Specify the device tree blob file
   - BIOS Handler Path: Path to BIOS mapping JSON
   - Backup Restore Path: Path to backup/restore configuration

3. **Configure Common Interfaces**
   - Define standard D-Bus interfaces for asset information
   - Set record names and keyword names for each field

4. **Add Muxes**
   - Configure I2C multiplexers
   - Specify I2C bus, device address, and hold idle path

5. **Add FRUs (Field Replaceable Units)**
   - Define EEPROM paths
   - Set inventory paths and service names
   - Configure extra interfaces and properties

6. **Validate Configuration**
   - Use the Validate menu to check your configuration
   - View the generated JSON before exporting

7. **Export**
   - Export your validated configuration to a JSON file

## Configuration Structure

The tool generates JSON files with the following structure:

```json
{
    "devTree": "conf-aspeed-bmc-ibm-rainier.dtb",
    "biosHandlerJsonPath": "/usr/share/vpd/bios_map_50001001.json",
    "backupRestoreConfigPath": "/usr/share/vpd/backup_restore_50001000.json",
    "commonInterfaces": {
        "xyz.openbmc_project.Inventory.Decorator.Asset": {
            "PartNumber": {
                "recordName": "VINI",
                "keywordName": "PN"
            },
            ...
        }
    },
    "muxes": [
        {
            "i2bus": "4",
            "deviceaddress": "0xE0",
            "holdidlepath": "/sys/bus/i2c/drivers/pca954x/4-0070/hold_idle"
        }
    ],
    "frus": {
        "/sys/bus/i2c/drivers/at24/8-0050/eeprom": [
            {
                "inventoryPath": "/xyz/openbmc_project/inventory/system/chassis/motherboard",
                "serviceName": "xyz.openbmc_project.Inventory.Manager",
                "extraInterfaces": { ... }
            }
        ]
    }
}
```

## Validation Rules

### Field Validations

- **Hex Addresses**: Must match pattern `0x[0-9A-Fa-f]+`
- **I2C Bus**: Must be a numeric value
- **Paths**: Must start with `/` or `./`
- **D-Bus Interfaces**: Must follow D-Bus naming conventions
- **Required Fields**: Cannot be empty

### Structure Validations

- All top-level required fields must be present:
  - `devTree`
  - `biosHandlerJsonPath`
  - `backupRestoreConfigPath`
  - `commonInterfaces`
  - `frus`

- Optional top-level fields:
  - `muxes` (array of I2C multiplexer configurations)

- `commonInterfaces` must contain `xyz.openbmc_project.Inventory.Decorator.Asset`
- `muxes` (if present) must be an array
- `frus` must be an object/dictionary

## Menu Structure

### File Menu
- **New Configuration**: Start a fresh configuration
- **Load Template**: Load an existing JSON file
- **Export JSON**: Save configuration to file
- **Exit**: Close the application

### Edit Menu
- **Basic Configuration**: Edit device tree and paths
- **Common Interfaces**: Configure asset decorator interfaces
- **Muxes**: Add/edit/delete I2C multiplexers
- **FRUs**: Add/edit/delete Field Replaceable Units

### Validate Menu
- **Validate Configuration**: Check configuration validity
- **View JSON**: Preview the generated JSON

### Help Menu
- **About**: Application information
- **Documentation**: Built-in help documentation

## FRU Optional Fields

The tool now supports all optional FRU fields found in IBM OpenBMC VPD configurations:

### Boolean Fields
- `isSystemVpd`: Marks this FRU as system VPD
- `inherit`: Whether to inherit common interfaces (default: true)
- `replaceableAtRuntime`: FRU can be replaced while system is running
- `replaceableAtStandby`: FRU can be replaced in standby mode
- `essentialFru`: Marks FRU as essential for system operation
- `powerOffOnly`: FRU requires power off for replacement
- `concurrentlyMaintainable`: FRU supports concurrent maintenance
- `embedded`: Marks FRU as embedded component

### Action Fields (JSON Objects)
- `preAction`: Actions to perform before VPD collection
- `postAction`: Actions to perform after successful VPD collection
- `postFailAction`: Actions to perform if VPD collection fails
- `pollingRequired`: Configuration for hot-plug polling

### Additional Fields
- Additional D-Bus interfaces can be specified (e.g., `xyz.openbmc_project.Inventory.Decorator.Slot`)

## Examples

### Example 1: Creating a Simple Configuration

1. Launch the tool
2. Click "Creating a New Configuration"
3. Fill in Basic Configuration:
   - Device Tree: `conf-aspeed-bmc-ibm-rainier.dtb`
   - BIOS Handler: `/usr/share/vpd/bios_map_50001001.json`
   - Backup Restore: `/usr/share/vpd/backup_restore_50001000.json`
4. Add a Mux:
   - I2C Bus: `4`
   - Device Address: `0xE0`
   - Hold Idle Path: `/sys/bus/i2c/drivers/pca954x/4-0070/hold_idle`
5. Add a FRU:
   - EEPROM Path: `/sys/bus/i2c/drivers/at24/8-0050/eeprom`
   - Inventory Path: `/xyz/openbmc_project/inventory/system/chassis/motherboard`
   - Service Name: `xyz.openbmc_project.Inventory.Manager`
   - Pretty Name: `System backplane`
6. Validate and Export

### Example 2: Using a Template

1. Launch the tool
2. Click "Loading an Existing Template"
3. Navigate to `configuration/ibm/`
4. Select an existing JSON file (e.g., `60001001_v2.json`)
5. Modify as needed
6. Export with a new name

## Troubleshooting

### Issue: Tool won't start
- **Solution**: Ensure Python 3.7+ is installed and tkinter is available
- **Check**: Run `python3 -m tkinter` to verify tkinter installation

### Issue: Validation errors
- **Solution**: Check the error message for specific field issues
- **Tip**: Use "View JSON" to see the current configuration state

### Issue: Can't load template
- **Solution**: Ensure the JSON file is valid and properly formatted
- **Tip**: Check file permissions

## Tips and Best Practices

1. **Start with a Template**: Load an existing configuration similar to your target system
2. **Validate Frequently**: Use the validation feature often to catch errors early
3. **Save Incrementally**: Export your work regularly to avoid data loss
4. **Use Descriptive Names**: Give FRUs clear, descriptive pretty names
5. **Check Paths**: Ensure all file paths are correct for your target system
6. **Review JSON**: Use "View JSON" before exporting to review the complete configuration

## Technical Details

### Supported Field Types

- **String**: Text fields with optional validation
- **Hex Address**: Hexadecimal values (e.g., 0xE0, 0x50)
- **Numeric**: Integer values (e.g., I2C bus numbers)
- **Path**: File system paths (absolute or relative)
- **Boolean**: True/false values
- **Object**: Nested JSON objects
- **Array**: Lists of items

### Validation Classes

- `FieldValidator`: Validates individual field values
- `ConfigGenerator`: Manages configuration data and validation
- `VPDConfigGUI`: Handles the user interface

## Contributing

To extend the tool:

1. Add new validation methods to `FieldValidator` class
2. Extend `ConfigGenerator` for new configuration sections
3. Add new UI sections in `VPDConfigGUI` class

## License

This tool is provided as-is for use with IBM OpenBMC VPD systems.

## Support

For issues or questions:
1. Check the built-in documentation (Help → Documentation)
2. Review existing configuration files in `configuration/ibm/`
3. Consult the OpenBMC VPD documentation in `docs/`

## Version History

### Version 2.0 (Current)
- Made `muxes` field optional
- Added support for all optional FRU fields:
  - Boolean fields: isSystemVpd, inherit, replaceableAtRuntime, replaceableAtStandby, essentialFru, powerOffOnly, concurrentlyMaintainable, embedded
  - Action fields: preAction, postAction, postFailAction, pollingRequired
  - Additional interface support
- Enhanced FRU dialog with scrollable interface
- Improved validation for optional fields

### Version 1.0
- Initial release
- Basic configuration support
- Mux and FRU management
- Field and structure validation
- Template loading
- JSON export

## Related Documentation

- `docs/system-config-json.md`: System configuration JSON format
- `docs/system-json_README.md`: System JSON documentation
- `docs/backup-restore-config-json.md`: Backup/restore configuration

---

**Note**: This tool generates configuration files for IBM OpenBMC VPD systems. Ensure you understand the system requirements before deploying generated configurations.
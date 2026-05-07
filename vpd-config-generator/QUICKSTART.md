# VPD Configuration Generator - Quick Start Guide

## Installation & Launch

### Step 1: Verify Python Installation
```bash
python3 --version
# Should show Python 3.7 or higher
```

### Step 2: Make Script Executable (Optional)
```bash
chmod +x vpd_config_generator.py
```

### Step 3: Launch the Tool
```bash
python3 vpd_config_generator.py
```

## Quick Tutorial: Create Your First Configuration

### Option A: Start from Scratch (5 minutes)

1. **Launch the tool** and click "Creating a New Configuration"

2. **Basic Configuration** (Edit → Basic Configuration)
   ```
   Device Tree: conf-aspeed-bmc-ibm-rainier.dtb
   BIOS Handler: /usr/share/vpd/bios_map_50001001.json
   Backup Restore: /usr/share/vpd/backup_restore_50001000.json
   ```
   Click "Save Basic Configuration"

3. **Add a Mux** (Edit → Muxes → Add Mux)
   ```
   I2C Bus: 4
   Device Address: 0xE0
   Hold Idle Path: /sys/bus/i2c/drivers/pca954x/4-0070/hold_idle
   ```
   Click "Add"

4. **Add a FRU** (Edit → FRUs → Add FRU)
   ```
   EEPROM Path: /sys/bus/i2c/drivers/at24/8-0050/eeprom
   Inventory Path: /xyz/openbmc_project/inventory/system/chassis/motherboard
   Service Name: xyz.openbmc_project.Inventory.Manager
   Pretty Name: System backplane
   ☑ Is System VPD
   ☑ Inherit
   ```
   Click "Add"

5. **Validate** (Validate → Validate Configuration)
   - Should show "Configuration is valid!"

6. **Export** (File → Export JSON)
   - Choose location and filename
   - Click "Save"

### Option B: Start from Template (2 minutes)

1. **Launch the tool** and click "Loading an Existing Template"

2. **Navigate to** `configuration/ibm/`

3. **Select a template** (e.g., `60001001_v2.json`)

4. **Modify as needed**:
   - Change device tree name
   - Add/remove muxes
   - Add/remove FRUs

5. **Validate and Export**

## Common Tasks

### View Current Configuration
- **Validate → View JSON**
- Shows complete JSON structure
- Copy to clipboard option available

### Add Multiple Muxes
1. Edit → Muxes
2. Click "Add Mux" for each one
3. Fill in details and click "Add"

### Modify Common Interfaces
1. Edit → Common Interfaces
2. Update record names and keywords
3. Click "Save Common Interfaces"

### Delete Items
- **Muxes**: Select in list → Click "Delete Selected"
- **FRUs**: Select in list → Click "Delete Selected"

## Keyboard Shortcuts

- **⌘+N** (macOS) / **Ctrl+N**: New Configuration
- **⌘+O** (macOS) / **Ctrl+O**: Load Template
- **⌘+S** (macOS) / **Ctrl+S**: Export JSON
- **⌘+Q** (macOS) / **Ctrl+Q**: Quit

## Validation Tips

### Common Validation Errors

1. **"Device Tree is required"**
   - Fill in the Device Tree field in Basic Configuration

2. **"Device Address must be a valid hex address"**
   - Use format: 0xE0, 0x50, etc.
   - Must start with "0x"

3. **"I2C Bus must be a numeric value"**
   - Use numbers only: 4, 5, 11, etc.

4. **"Path must be an absolute or relative path"**
   - Start with `/` for absolute paths
   - Start with `./` for relative paths

### Pre-Export Checklist

- ✓ Basic configuration filled
- ✓ At least one mux configured
- ✓ At least one FRU configured
- ✓ Validation passes
- ✓ JSON preview looks correct

## Example Configurations

### Minimal Configuration
```json
{
    "devTree": "conf-aspeed-bmc-ibm-rainier.dtb",
    "biosHandlerJsonPath": "/usr/share/vpd/bios_map.json",
    "backupRestoreConfigPath": "/usr/share/vpd/backup_restore.json",
    "commonInterfaces": { ... },
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
                "inventoryPath": "/xyz/openbmc_project/inventory/system",
                "serviceName": "xyz.openbmc_project.Inventory.Manager",
                "extraInterfaces": { ... }
            }
        ]
    }
}
```

## Troubleshooting

### Tool Won't Start
```bash
# Check Python version
python3 --version

# Test tkinter
python3 -m tkinter
# Should open a small test window
```

### Can't Load Template
- Check file exists
- Verify JSON is valid
- Check file permissions

### Validation Fails
- Use "View JSON" to see current state
- Check error message for specific field
- Compare with working template

## Next Steps

1. **Read Full Documentation**: See `README_VPD_GENERATOR.md`
2. **Explore Templates**: Check `configuration/ibm/` directory
3. **Review System Docs**: See `docs/system-config-json.md`

## Tips for Success

1. 🎯 **Start Simple**: Begin with minimal configuration
2. 📋 **Use Templates**: Load similar configurations as starting point
3. ✅ **Validate Often**: Check configuration frequently
4. 💾 **Save Regularly**: Export work incrementally
5. 📖 **Read Examples**: Study existing configurations

## Getting Help

- **Built-in Help**: Help → Documentation
- **About Dialog**: Help → About
- **Status Bar**: Watch for messages at bottom of window

---

**Ready to start?** Launch the tool and follow Option A or B above!
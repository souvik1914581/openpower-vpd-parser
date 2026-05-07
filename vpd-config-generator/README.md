# VPD Configuration Generator

A Python-based GUI tool for generating IBM OpenBMC VPD (Vital Product Data) system configuration JSON files.

## 🚀 Quick Start

### Option 1: Web-Based Interface (No tkinter required)

**Recommended for macOS users with Homebrew Python**

```bash
cd vpd-config-generator
python3 vpd_config_generator_web.py
```

This will:
- Start a local web server on port 8080
- Automatically open your browser
- Provide the same functionality without requiring tkinter

### Option 2: Native GUI (Requires tkinter)

If you have tkinter installed:

```bash
cd vpd-config-generator
python3 vpd_config_generator.py
```

## 📋 Installation

### For Homebrew Python Users (macOS)

If you get `ModuleNotFoundError: No module named '_tkinter'`, you have two options:

**A. Install tkinter support (for native GUI):**
```bash
brew install python-tk@3.13
```

**B. Use the web-based version (no installation needed):**
```bash
python3 vpd_config_generator_web.py
```

See [`INSTALL_TKINTER.md`](INSTALL_TKINTER.md) for detailed installation instructions.

## 📚 Documentation

- **[README_VPD_GENERATOR.md](README_VPD_GENERATOR.md)** - Complete user guide and features
- **[QUICKSTART.md](QUICKSTART.md)** - 5-minute tutorial
- **[INSTALL_TKINTER.md](INSTALL_TKINTER.md)** - tkinter installation guide
- **[PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)** - Technical overview

## ✨ Features

- ✅ **Two Interface Options**: Native GUI (tkinter) or Web-based
- ✅ **Hierarchical Menu System**: Navigate configuration sections easily
- ✅ **Field Validation**: Real-time validation of all inputs
- ✅ **JSON Validation**: Complete structure validation
- ✅ **Template Support**: Load existing configurations
- ✅ **Self-Contained**: No external dependencies (except tkinter for native GUI)

## 🧪 Testing

Run the test suite to verify functionality:

```bash
cd vpd-config-generator
python3 test_vpd_generator.py
```

Expected output:
```
============================================================
VPD Configuration Generator - Test Suite
============================================================
Testing Field Validator...
  ✓ Hex address validation
  ✓ Invalid hex rejected correctly
  ✓ I2C bus validation
  ✓ Path validation
  ✓ Boolean validation

Testing Config Generator...
  ✓ Basic configuration
  ✓ Add mux
  ✓ Add FRU
  ✓ Configuration validation

Testing Template Loading...
  ✓ Template loaded successfully

Testing JSON Export...
  ✓ JSON export successful

============================================================
Test Results: 4 passed, 0 failed
============================================================

✓ All tests passed! The tool is ready to use.
```

## 📖 Usage Examples

### Web Interface

1. Start the server:
   ```bash
   python3 vpd_config_generator_web.py
   ```

2. Browser opens automatically at `http://localhost:8080`

3. Use the tabs to configure:
   - **Basic Config**: Device tree and paths
   - **Muxes**: I2C multiplexers
   - **FRUs**: Field Replaceable Units
   - **View JSON**: Preview and validate
   - **Templates**: Load existing configurations

### Native GUI

1. Launch the application:
   ```bash
   python3 vpd_config_generator.py
   ```

2. Use the menu system:
   - **File** → New/Load/Export
   - **Edit** → Configure sections
   - **Validate** → Check configuration
   - **Help** → Documentation

## 🔧 Configuration Structure

The tool generates JSON files with this structure:

```json
{
    "devTree": "conf-aspeed-bmc-ibm-rainier.dtb",
    "biosHandlerJsonPath": "/usr/share/vpd/bios_map.json",
    "backupRestoreConfigPath": "/usr/share/vpd/backup_restore.json",
    "commonInterfaces": { ... },
    "muxes": [ ... ],
    "frus": { ... }
}
```

## 🎯 Validation Rules

- **Hex Addresses**: Must match `0x[0-9A-Fa-f]+` (e.g., `0xE0`)
- **I2C Bus**: Numeric values only (e.g., `4`, `5`)
- **Paths**: Must start with `/` or `./`
- **D-Bus Interfaces**: Must follow D-Bus naming conventions
- **Required Fields**: Cannot be empty

## 🐛 Troubleshooting

### tkinter Not Available

**Error**: `ModuleNotFoundError: No module named '_tkinter'`

**Solution**: Use the web-based version instead:
```bash
python3 vpd_config_generator_web.py
```

Or install tkinter support:
```bash
brew install python-tk@3.13
```

### Port Already in Use (Web Version)

**Error**: `OSError: [Errno 48] Address already in use`

**Solution**: Stop the existing server or use a different port:
```bash
# Find and kill the process
lsof -ti:8080 | xargs kill -9

# Or edit vpd_config_generator_web.py and change the port number
```

### Template Not Found

**Error**: Template files not loading

**Solution**: Ensure you're running from the correct directory:
```bash
cd /path/to/openpower-vpd-parser
cd vpd-config-generator
python3 vpd_config_generator_web.py
```

## 📁 File Structure

```
vpd-config-generator/
├── vpd_config_generator.py      # Native GUI (tkinter)
├── vpd_config_generator_web.py  # Web-based interface
├── test_vpd_generator.py        # Test suite
├── README.md                    # This file
├── README_VPD_GENERATOR.md      # Detailed documentation
├── QUICKSTART.md                # Quick start guide
├── INSTALL_TKINTER.md           # tkinter installation
└── PROJECT_SUMMARY.md           # Technical overview
```

## 🤝 Contributing

To extend the tool:

1. Add validation methods to `FieldValidator` class
2. Extend `ConfigGenerator` for new sections
3. Add UI components in `VPDConfigGUI` or web handler

## 📄 License

This tool is provided for use with IBM OpenBMC VPD systems.

## 🆘 Support

- Check built-in documentation: Help → Documentation (native GUI)
- Review existing configurations in `../configuration/ibm/`
- Consult OpenBMC VPD documentation in `../docs/`

---

**Version**: 1.0  
**Status**: ✅ Production Ready  
**Tested**: macOS with Python 3.13
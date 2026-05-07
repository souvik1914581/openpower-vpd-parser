#!/usr/bin/env python3
"""
Test script for VPD Configuration Generator
Validates core functionality without launching GUI
"""

import json
import sys
import os

# Add current directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from vpd_config_generator import ConfigGenerator, FieldValidator, ValidationError


def test_field_validator():
    """Test field validation functions"""
    print("Testing Field Validator...")
    validator = FieldValidator()
    
    # Test hex address validation
    try:
        result = validator.validate_hex_address("0xE0", "test")
        assert result == "0xE0", "Hex validation failed"
        print("  ✓ Hex address validation")
    except Exception as e:
        print(f"  ✗ Hex address validation failed: {e}")
        return False
    
    # Test invalid hex
    try:
        validator.validate_hex_address("E0", "test")
        print("  ✗ Invalid hex should have failed")
        return False
    except ValidationError:
        print("  ✓ Invalid hex rejected correctly")
    
    # Test I2C bus validation
    try:
        result = validator.validate_i2c_bus("4", "test")
        assert result == "4", "I2C bus validation failed"
        print("  ✓ I2C bus validation")
    except Exception as e:
        print(f"  ✗ I2C bus validation failed: {e}")
        return False
    
    # Test path validation
    try:
        result = validator.validate_path("/sys/bus/i2c/test", "test")
        assert result == "/sys/bus/i2c/test", "Path validation failed"
        print("  ✓ Path validation")
    except Exception as e:
        print(f"  ✗ Path validation failed: {e}")
        return False
    
    # Test boolean validation
    try:
        assert validator.validate_boolean(True, "test") == True
        assert validator.validate_boolean("true", "test") == True
        assert validator.validate_boolean("false", "test") == False
        print("  ✓ Boolean validation")
    except Exception as e:
        print(f"  ✗ Boolean validation failed: {e}")
        return False
    
    return True


def test_config_generator():
    """Test configuration generator"""
    print("\nTesting Config Generator...")
    generator = ConfigGenerator()
    
    # Test basic config
    try:
        generator.set_basic_config(
            "conf-aspeed-bmc-ibm-rainier.dtb",
            "/usr/share/vpd/bios_map.json",
            "/usr/share/vpd/backup_restore.json"
        )
        assert generator.config_data["devTree"] == "conf-aspeed-bmc-ibm-rainier.dtb"
        print("  ✓ Basic configuration")
    except Exception as e:
        print(f"  ✗ Basic configuration failed: {e}")
        return False
    
    # Test add mux
    try:
        generator.add_mux("4", "0xE0", "/sys/bus/i2c/drivers/pca954x/4-0070/hold_idle")
        assert len(generator.config_data["muxes"]) == 1
        print("  ✓ Add mux")
    except Exception as e:
        print(f"  ✗ Add mux failed: {e}")
        return False
    
    # Test add FRU
    try:
        fru_config = {
            "inventoryPath": "/xyz/openbmc_project/inventory/system",
            "serviceName": "xyz.openbmc_project.Inventory.Manager",
            "extraInterfaces": {}
        }
        generator.add_fru("/sys/bus/i2c/drivers/at24/8-0050/eeprom", fru_config)
        assert len(generator.config_data["frus"]) == 1
        print("  ✓ Add FRU")
    except Exception as e:
        print(f"  ✗ Add FRU failed: {e}")
        return False
    
    # Test validation
    try:
        generator.validate()
        print("  ✓ Configuration validation")
    except Exception as e:
        print(f"  ✗ Configuration validation failed: {e}")
        return False
    
    return True


def test_template_loading():
    """Test loading existing templates"""
    print("\nTesting Template Loading...")
    generator = ConfigGenerator()
    
    template_path = "configuration/ibm/60001001_v2.json"
    if not os.path.exists(template_path):
        print(f"  ⚠ Template not found: {template_path}")
        return True  # Not a failure, just skip
    
    try:
        generator.load_template(template_path)
        assert "devTree" in generator.config_data
        assert "muxes" in generator.config_data
        assert "frus" in generator.config_data
        print(f"  ✓ Template loaded successfully")
        print(f"    - Device Tree: {generator.config_data['devTree']}")
        print(f"    - Muxes: {len(generator.config_data['muxes'])}")
        print(f"    - FRUs: {len(generator.config_data['frus'])}")
    except Exception as e:
        print(f"  ✗ Template loading failed: {e}")
        return False
    
    return True


def test_json_export():
    """Test JSON export functionality"""
    print("\nTesting JSON Export...")
    generator = ConfigGenerator()
    
    # Setup minimal config
    generator.set_basic_config(
        "test.dtb",
        "/test/bios.json",
        "/test/backup.json"
    )
    generator.add_mux("4", "0xE0", "/test/path")
    
    fru_config = {
        "inventoryPath": "/test/inventory",
        "serviceName": "test.service",
        "extraInterfaces": {}
    }
    generator.add_fru("/test/eeprom", fru_config)
    
    # Export to temp file
    test_file = "/tmp/test_vpd_config.json"
    try:
        generator.export_json(test_file)
        
        # Verify file exists and is valid JSON
        with open(test_file, 'r') as f:
            data = json.load(f)
        
        assert data["devTree"] == "test.dtb"
        assert len(data["muxes"]) == 1
        assert len(data["frus"]) == 1
        
        print("  ✓ JSON export successful")
        
        # Cleanup
        os.remove(test_file)
        
    except Exception as e:
        print(f"  ✗ JSON export failed: {e}")
        return False
    
    return True


def main():
    """Run all tests"""
    print("=" * 60)
    print("VPD Configuration Generator - Test Suite")
    print("=" * 60)
    
    tests = [
        ("Field Validator", test_field_validator),
        ("Config Generator", test_config_generator),
        ("Template Loading", test_template_loading),
        ("JSON Export", test_json_export),
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        try:
            if test_func():
                passed += 1
            else:
                failed += 1
        except Exception as e:
            print(f"\n✗ {test_name} crashed: {e}")
            failed += 1
    
    print("\n" + "=" * 60)
    print(f"Test Results: {passed} passed, {failed} failed")
    print("=" * 60)
    
    if failed == 0:
        print("\n✓ All tests passed! The tool is ready to use.")
        return 0
    else:
        print(f"\n✗ {failed} test(s) failed. Please review the errors above.")
        return 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob

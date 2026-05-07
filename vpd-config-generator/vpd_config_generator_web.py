#!/usr/bin/env python3
"""
VPD Configuration Generator - Web Interface
A standalone web-based tool that doesn't require tkinter
"""

import json
import os
import re
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs, urlparse
import webbrowser
from pathlib import Path
from typing import Dict, Any


class ValidationError(Exception):
    """Custom exception for validation errors"""
    pass


class FieldValidator:
    """Validates individual field values based on their type and constraints"""
    
    @staticmethod
    def validate_string(value: str, field_name: str, required: bool = True) -> str:
        """Validate string fields"""
        if required and not value.strip():
            raise ValidationError(f"{field_name} is required and cannot be empty")
        return value.strip()
    
    @staticmethod
    def validate_hex_address(value: str, field_name: str) -> str:
        """Validate hexadecimal address (e.g., 0xE0, 0x50)"""
        value = value.strip()
        if not re.match(r'^0x[0-9A-Fa-f]+$', value):
            raise ValidationError(f"{field_name} must be a valid hex address (e.g., 0xE0)")
        return value
    
    @staticmethod
    def validate_i2c_bus(value: str, field_name: str) -> str:
        """Validate I2C bus number"""
        value = value.strip()
        if not value.isdigit():
            raise ValidationError(f"{field_name} must be a numeric value")
        return value
    
    @staticmethod
    def validate_path(value: str, field_name: str, required: bool = True) -> str:
        """Validate file system paths"""
        value = value.strip()
        if required and not value:
            raise ValidationError(f"{field_name} is required")
        if value and not (value.startswith('/') or value.startswith('./')):
            raise ValidationError(f"{field_name} must be an absolute or relative path")
        return value
    
    @staticmethod
    def validate_json_structure(data: Dict[str, Any]) -> None:
        """Validate the complete JSON structure"""
        required_fields = ['devTree', 'biosHandlerJsonPath', 'backupRestoreConfigPath',
                          'commonInterfaces', 'frus']
        
        for field in required_fields:
            if field not in data:
                raise ValidationError(f"Required top-level field '{field}' is missing")
        
        # Validate commonInterfaces structure
        if 'xyz.openbmc_project.Inventory.Decorator.Asset' not in data['commonInterfaces']:
            raise ValidationError("commonInterfaces must contain 'xyz.openbmc_project.Inventory.Decorator.Asset'")
        
        # Validate muxes is a list (if present)
        if 'muxes' in data and not isinstance(data['muxes'], list):
            raise ValidationError("'muxes' must be a list")
        
        # Validate frus is a dict
        if not isinstance(data['frus'], dict):
            raise ValidationError("'frus' must be an object/dictionary")


class ConfigGenerator:
    """Main configuration generator class"""
    
    def __init__(self):
        self.config_data = {
            "devTree": "",
            "biosHandlerJsonPath": "",
            "backupRestoreConfigPath": "",
            "commonInterfaces": {
                "xyz.openbmc_project.Inventory.Decorator.Asset": {
                    "PartNumber": {"recordName": "VINI", "keywordName": "PN"},
                    "SerialNumber": {"recordName": "VINI", "keywordName": "SN"},
                    "SparePartNumber": {"recordName": "VINI", "keywordName": "FN"},
                    "Model": {"recordName": "VINI", "keywordName": "CC"},
                    "BuildDate": {"recordName": "VR10", "keywordName": "DC", "encoding": "DATE"}
                }
            },
            "muxes": [],
            "frus": {}
        }
        self.validator = FieldValidator()
    
    def add_mux(self, i2bus: str, deviceaddress: str, holdidlepath: str) -> None:
        """Add a mux configuration"""
        mux = {
            "i2bus": self.validator.validate_i2c_bus(i2bus, "I2C Bus"),
            "deviceaddress": self.validator.validate_hex_address(deviceaddress, "Device Address"),
            "holdidlepath": self.validator.validate_path(holdidlepath, "Hold Idle Path")
        }
        self.config_data["muxes"].append(mux)
    
    def add_fru(self, eeprom_path: str, fru_config: Dict[str, Any]) -> None:
        """Add a FRU configuration"""
        if eeprom_path not in self.config_data["frus"]:
            self.config_data["frus"][eeprom_path] = []
        self.config_data["frus"][eeprom_path].append(fru_config)
    
    def set_basic_config(self, dev_tree: str, bios_handler: str, backup_restore: str) -> None:
        """Set basic configuration parameters"""
        self.config_data["devTree"] = self.validator.validate_string(dev_tree, "Device Tree")
        self.config_data["biosHandlerJsonPath"] = self.validator.validate_path(bios_handler, "BIOS Handler Path")
        self.config_data["backupRestoreConfigPath"] = self.validator.validate_path(backup_restore, "Backup Restore Path")
    
    def validate(self) -> bool:
        """Validate the complete configuration"""
        try:
            self.validator.validate_json_structure(self.config_data)
            return True
        except ValidationError as e:
            raise ValidationError(f"Configuration validation failed: {str(e)}")
    
    def export_json(self, filepath: str) -> None:
        """Export configuration to JSON file"""
        self.validate()
        with open(filepath, 'w') as f:
            json.dump(self.config_data, f, indent=4)
    
    def load_template(self, filepath: str) -> None:
        """Load an existing configuration as template"""
        with open(filepath, 'r') as f:
            self.config_data = json.load(f)


class VPDWebHandler(BaseHTTPRequestHandler):
    """HTTP request handler for the web interface"""
    
    generator = ConfigGenerator()
    
    def log_message(self, format, *args):
        """Suppress default logging"""
        pass
    
    def do_GET(self):
        """Handle GET requests"""
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/' or parsed_path.path == '/index.html':
            self.serve_html()
        elif parsed_path.path == '/api/config':
            self.serve_config()
        elif parsed_path.path == '/api/templates':
            self.serve_templates()
        else:
            self.send_error(404)
    
    def do_POST(self):
        """Handle POST requests"""
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/api/save':
            self.handle_save(post_data)
        elif parsed_path.path == '/api/validate':
            self.handle_validate(post_data)
        elif parsed_path.path == '/api/export':
            self.handle_export(post_data)
        elif parsed_path.path == '/api/load':
            self.handle_load(post_data)
        else:
            self.send_error(404)
    
    def serve_html(self):
        """Serve the main HTML interface"""
        html = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>VPD Configuration Generator</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #f5f5f7;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 { font-size: 2em; margin-bottom: 10px; }
        .header p { opacity: 0.9; }
        .tabs {
            display: flex;
            background: #f8f9fa;
            border-bottom: 1px solid #dee2e6;
        }
        .tab {
            padding: 15px 30px;
            cursor: pointer;
            border: none;
            background: none;
            font-size: 14px;
            font-weight: 500;
            color: #6c757d;
            transition: all 0.3s;
        }
        .tab:hover { background: #e9ecef; }
        .tab.active {
            background: white;
            color: #667eea;
            border-bottom: 3px solid #667eea;
        }
        .content {
            padding: 30px;
            min-height: 500px;
        }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            color: #333;
        }
        input[type="text"], textarea, select {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        input[type="text"]:focus, textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        textarea {
            min-height: 400px;
            font-family: 'Monaco', 'Courier New', monospace;
            font-size: 12px;
        }
        .btn {
            padding: 10px 20px;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s;
            margin-right: 10px;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover { background: #5568d3; }
        .btn-success {
            background: #28a745;
            color: white;
        }
        .btn-success:hover { background: #218838; }
        .btn-secondary {
            background: #6c757d;
            color: white;
        }
        .btn-secondary:hover { background: #5a6268; }
        .alert {
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
        }
        .alert-success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .alert-error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .alert-info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }
        .hidden { display: none; }
        .list-item {
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 6px;
            margin-bottom: 10px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .list-item:hover { background: #f8f9fa; }
        .hint {
            font-size: 12px;
            color: #6c757d;
            margin-top: 5px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔧 VPD Configuration Generator</h1>
            <p>Web-based tool for IBM OpenBMC VPD system configurations</p>
        </div>
        
        <div class="tabs">
            <button class="tab active" onclick="showTab('basic')">Basic Config</button>
            <button class="tab" onclick="showTab('muxes')">Muxes</button>
            <button class="tab" onclick="showTab('frus')">FRUs</button>
            <button class="tab" onclick="showTab('json')">View JSON</button>
            <button class="tab" onclick="showTab('templates')">Templates</button>
        </div>
        
        <div class="content">
            <div id="alert-container"></div>
            
            <!-- Basic Configuration Tab -->
            <div id="basic-tab" class="tab-content active">
                <h2>Basic Configuration</h2>
                <div class="form-group">
                    <label>Device Tree:</label>
                    <input type="text" id="devTree" placeholder="conf-aspeed-bmc-ibm-rainier.dtb">
                    <div class="hint">e.g., conf-aspeed-bmc-ibm-rainier.dtb</div>
                </div>
                <div class="form-group">
                    <label>BIOS Handler JSON Path:</label>
                    <input type="text" id="biosHandler" placeholder="/usr/share/vpd/bios_map.json">
                    <div class="hint">e.g., /usr/share/vpd/bios_map_50001001.json</div>
                </div>
                <div class="form-group">
                    <label>Backup Restore Config Path:</label>
                    <input type="text" id="backupRestore" placeholder="/usr/share/vpd/backup_restore.json">
                    <div class="hint">e.g., /usr/share/vpd/backup_restore_50001000.json</div>
                </div>
                <button class="btn btn-primary" onclick="saveBasicConfig()">Save Basic Config</button>
            </div>
            
            <!-- Muxes Tab -->
            <div id="muxes-tab" class="tab-content">
                <h2>I2C Multiplexers (Muxes)</h2>
                <div id="muxes-list"></div>
                <hr style="margin: 20px 0;">
                <h3>Add New Mux</h3>
                <div class="form-group">
                    <label>I2C Bus:</label>
                    <input type="text" id="mux-i2bus" placeholder="4">
                </div>
                <div class="form-group">
                    <label>Device Address:</label>
                    <input type="text" id="mux-address" placeholder="0xE0">
                </div>
                <div class="form-group">
                    <label>Hold Idle Path:</label>
                    <input type="text" id="mux-path" placeholder="/sys/bus/i2c/drivers/pca954x/4-0070/hold_idle">
                </div>
                <button class="btn btn-primary" onclick="addMux()">Add Mux</button>
            </div>
            
            <!-- FRUs Tab -->
            <div id="frus-tab" class="tab-content">
                <h2>Field Replaceable Units (FRUs)</h2>
                <div id="frus-list"></div>
                <hr style="margin: 20px 0;">
                <h3>Add New FRU</h3>
                <div style="max-height: 400px; overflow-y: auto; padding: 10px; border: 1px solid #ddd; border-radius: 6px;">
                    <div class="form-group">
                        <label>EEPROM Path:</label>
                        <input type="text" id="fru-eeprom" placeholder="/sys/bus/i2c/drivers/at24/8-0050/eeprom">
                    </div>
                    <div class="form-group">
                        <label>Inventory Path:</label>
                        <input type="text" id="fru-inventory" placeholder="/xyz/openbmc_project/inventory/system">
                    </div>
                    <div class="form-group">
                        <label>Service Name:</label>
                        <input type="text" id="fru-service" value="xyz.openbmc_project.Inventory.Manager">
                    </div>
                    <div class="form-group">
                        <label>Pretty Name:</label>
                        <input type="text" id="fru-pretty" placeholder="System backplane">
                    </div>
                    
                    <hr style="margin: 15px 0;">
                    <h4>Optional Boolean Fields</h4>
                    <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px;">
                        <label><input type="checkbox" id="fru-system-vpd"> isSystemVpd</label>
                        <label><input type="checkbox" id="fru-inherit" checked> inherit</label>
                        <label><input type="checkbox" id="fru-replaceable-runtime"> replaceableAtRuntime</label>
                        <label><input type="checkbox" id="fru-replaceable-standby"> replaceableAtStandby</label>
                        <label><input type="checkbox" id="fru-essential"> essentialFru</label>
                        <label><input type="checkbox" id="fru-power-off"> powerOffOnly</label>
                        <label><input type="checkbox" id="fru-concurrent"> concurrentlyMaintainable</label>
                        <label><input type="checkbox" id="fru-embedded"> embedded</label>
                    </div>
                    
                    <hr style="margin: 15px 0;">
                    <h4>Action Fields (JSON) <button class="btn btn-secondary" onclick="showActionHelp()" style="padding: 5px 10px; font-size: 12px;">Help</button></h4>
                    <div class="form-group">
                        <label>preAction:</label>
                        <input type="text" id="fru-pre-action" placeholder='{"collection": {"gpioPresence": {"pin": "GPIO_PIN", "value": 0}}}'>
                        <div class="hint">Actions to perform before VPD collection</div>
                    </div>
                    <div class="form-group">
                        <label>postAction:</label>
                        <input type="text" id="fru-post-action" placeholder='{"deletion": {"systemCmd": {"cmd": "echo unbind"}}}'>
                        <div class="hint">Actions to perform after successful VPD collection</div>
                    </div>
                    <div class="form-group">
                        <label>postFailAction:</label>
                        <input type="text" id="fru-post-fail-action" placeholder='{"collection": {"setGpio": {"pin": "GPIO_PIN", "value": 1}}}'>
                        <div class="hint">Actions to perform if VPD collection fails</div>
                    </div>
                    <div class="form-group">
                        <label>pollingRequired:</label>
                        <input type="text" id="fru-polling" placeholder='{"hotPlugging": {"gpioPresence": {"pin": "GPIO_PIN", "value": 0}}}'>
                        <div class="hint">Configuration for hot-plug polling</div>
                    </div>
                    
                    <hr style="margin: 15px 0;">
                    <h4>Additional Interface</h4>
                    <div class="form-group">
                        <label>Additional D-Bus Interface:</label>
                        <input type="text" id="fru-additional-interface" placeholder="xyz.openbmc_project.Inventory.Decorator.Slot">
                    </div>
                </div>
                <button class="btn btn-primary" onclick="addFRU()" style="margin-top: 15px;">Add FRU</button>
            </div>
            
            <!-- JSON View Tab -->
            <div id="json-tab" class="tab-content">
                <h2>Configuration JSON</h2>
                <button class="btn btn-success" onclick="validateConfig()">Validate</button>
                <button class="btn btn-primary" onclick="exportConfig()">Export JSON</button>
                <button class="btn btn-secondary" onclick="refreshJSON()">Refresh</button>
                <div class="form-group" style="margin-top: 20px;">
                    <textarea id="json-view" readonly></textarea>
                </div>
            </div>
            
            <!-- Templates Tab -->
            <div id="templates-tab" class="tab-content">
                <h2>Load Template</h2>
                <p>Select an existing configuration to use as a template:</p>
                <div id="templates-list"></div>
            </div>
        </div>
    </div>
    
    <script>
        let config = null;
        
        // Initialize
        window.onload = function() {
            loadConfig();
            loadTemplates();
        };
        
        function showTab(tabName) {
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            event.target.classList.add('active');
            document.getElementById(tabName + '-tab').classList.add('active');
            
            if (tabName === 'json') {
                refreshJSON();
            } else if (tabName === 'muxes') {
                refreshMuxesList();
            } else if (tabName === 'frus') {
                refreshFRUsList();
            }
        }
        
        function showAlert(message, type = 'info') {
            const container = document.getElementById('alert-container');
            container.innerHTML = `<div class="alert alert-${type}">${message}</div>`;
            setTimeout(() => container.innerHTML = '', 5000);
        }
        
        async function loadConfig() {
            const response = await fetch('/api/config');
            config = await response.json();
            updateBasicFields();
        }
        
        function updateBasicFields() {
            if (config) {
                document.getElementById('devTree').value = config.devTree || '';
                document.getElementById('biosHandler').value = config.biosHandlerJsonPath || '';
                document.getElementById('backupRestore').value = config.backupRestoreConfigPath || '';
            }
        }
        
        async function saveBasicConfig() {
            const data = {
                action: 'basic',
                devTree: document.getElementById('devTree').value,
                biosHandler: document.getElementById('biosHandler').value,
                backupRestore: document.getElementById('backupRestore').value
            };
            
            const response = await fetch('/api/save', {
                method: 'POST',
                body: JSON.stringify(data)
            });
            
            const result = await response.json();
            if (result.success) {
                showAlert('Basic configuration saved!', 'success');
                await loadConfig();
            } else {
                showAlert('Error: ' + result.error, 'error');
            }
        }
        
        async function addMux() {
            const data = {
                action: 'mux',
                i2bus: document.getElementById('mux-i2bus').value,
                address: document.getElementById('mux-address').value,
                path: document.getElementById('mux-path').value
            };
            
            const response = await fetch('/api/save', {
                method: 'POST',
                body: JSON.stringify(data)
            });
            
            const result = await response.json();
            if (result.success) {
                showAlert('Mux added successfully!', 'success');
                document.getElementById('mux-i2bus').value = '';
                document.getElementById('mux-address').value = '';
                document.getElementById('mux-path').value = '';
                await loadConfig();
                refreshMuxesList();
            } else {
                showAlert('Error: ' + result.error, 'error');
            }
        }
        
        async function addFRU() {
            const data = {
                action: 'fru',
                eeprom: document.getElementById('fru-eeprom').value,
                inventory: document.getElementById('fru-inventory').value,
                service: document.getElementById('fru-service').value,
                pretty: document.getElementById('fru-pretty').value,
                // Boolean fields
                isSystemVpd: document.getElementById('fru-system-vpd').checked,
                inherit: document.getElementById('fru-inherit').checked,
                replaceableAtRuntime: document.getElementById('fru-replaceable-runtime').checked,
                replaceableAtStandby: document.getElementById('fru-replaceable-standby').checked,
                essentialFru: document.getElementById('fru-essential').checked,
                powerOffOnly: document.getElementById('fru-power-off').checked,
                concurrentlyMaintainable: document.getElementById('fru-concurrent').checked,
                embedded: document.getElementById('fru-embedded').checked,
                // Action fields
                preAction: document.getElementById('fru-pre-action').value,
                postAction: document.getElementById('fru-post-action').value,
                postFailAction: document.getElementById('fru-post-fail-action').value,
                pollingRequired: document.getElementById('fru-polling').value,
                // Additional interface
                additionalInterface: document.getElementById('fru-additional-interface').value
            };
            
            const response = await fetch('/api/save', {
                method: 'POST',
                body: JSON.stringify(data)
            });
            
            const result = await response.json();
            if (result.success) {
                showAlert('FRU added successfully!', 'success');
                // Clear form
                document.getElementById('fru-eeprom').value = '';
                document.getElementById('fru-inventory').value = '';
                document.getElementById('fru-pretty').value = '';
                document.getElementById('fru-system-vpd').checked = false;
                document.getElementById('fru-inherit').checked = true;
                document.getElementById('fru-replaceable-runtime').checked = false;
                document.getElementById('fru-replaceable-standby').checked = false;
                document.getElementById('fru-essential').checked = false;
                document.getElementById('fru-power-off').checked = false;
                document.getElementById('fru-concurrent').checked = false;
                document.getElementById('fru-embedded').checked = false;
                document.getElementById('fru-pre-action').value = '';
                document.getElementById('fru-post-action').value = '';
                document.getElementById('fru-post-fail-action').value = '';
                document.getElementById('fru-polling').value = '';
                document.getElementById('fru-additional-interface').value = '';
                await loadConfig();
                refreshFRUsList();
            } else {
                showAlert('Error: ' + result.error, 'error');
            }
        }
        
        function refreshMuxesList() {
            const list = document.getElementById('muxes-list');
            if (!config || !config.muxes || config.muxes.length === 0) {
                list.innerHTML = '<p class="hint">No muxes configured yet.</p>';
                return;
            }
            
            list.innerHTML = config.muxes.map((mux, i) => `
                <div class="list-item">
                    <div>
                        <strong>Bus ${mux.i2bus}</strong> - ${mux.deviceaddress}<br>
                        <small>${mux.holdidlepath}</small>
                    </div>
                </div>
            `).join('');
        }
        
        function refreshFRUsList() {
            const list = document.getElementById('frus-list');
            if (!config || !config.frus || Object.keys(config.frus).length === 0) {
                list.innerHTML = '<p class="hint">No FRUs configured yet.</p>';
                return;
            }
            
            list.innerHTML = Object.entries(config.frus).map(([path, configs]) => `
                <div class="list-item">
                    <div>
                        <strong>${path}</strong><br>
                        <small>${configs.length} configuration(s)</small>
                    </div>
                </div>
            `).join('');
        }
        
        async function refreshJSON() {
            await loadConfig();
            document.getElementById('json-view').value = JSON.stringify(config, null, 4);
        }
        
        async function validateConfig() {
            const response = await fetch('/api/validate', {
                method: 'POST',
                body: JSON.stringify(config)
            });
            
            const result = await response.json();
            if (result.valid) {
                showAlert('✓ Configuration is valid!', 'success');
            } else {
                showAlert('✗ Validation failed: ' + result.error, 'error');
            }
        }
        
        async function exportConfig() {
            const filename = prompt('Enter filename:', 'new_config.json');
            if (!filename) return;
            
            const response = await fetch('/api/export', {
                method: 'POST',
                body: JSON.stringify({ filename: filename })
            });
            
            const result = await response.json();
            if (result.success) {
                showAlert('Configuration exported to: ' + result.path, 'success');
            } else {
                showAlert('Export failed: ' + result.error, 'error');
            }
        }
        
        async function loadTemplates() {
            const response = await fetch('/api/templates');
            const templates = await response.json();
            
            const list = document.getElementById('templates-list');
            if (templates.length === 0) {
                list.innerHTML = '<p class="hint">No templates found in ../configuration/ibm/</p>';
                return;
            }
            
            list.innerHTML = templates.map(t => `
                <div class="list-item">
                    <div><strong>${t}</strong></div>
                    <button class="btn btn-primary" onclick="loadTemplate('${t}')">Load</button>
                </div>
            `).join('');
        }
        
        async function loadTemplate(filename) {
            const response = await fetch('/api/load', {
                method: 'POST',
                body: JSON.stringify({ filename: filename })
            });
            
            const result = await response.json();
            if (result.success) {
                showAlert('Template loaded: ' + filename, 'success');
                await loadConfig();
                updateBasicFields();
                showTab('basic');
            } else {
                showAlert('Failed to load template: ' + result.error, 'error');
            }
        }
        
        function showActionHelp() {
            const helpMessage = `Action Field Structure:

preAction/postAction/postFailAction can contain:
  • collection: Actions during VPD collection
  • deletion: Actions during VPD deletion

Each can have:
  • gpioPresence: {"pin": "GPIO_NAME", "value": 0/1}
  • setGpio: {"pin": "GPIO_NAME", "value": 0/1}
  • systemCmd: {"cmd": "shell command"}

pollingRequired structure:
  • hotPlugging: {"gpioPresence": {"pin": "GPIO_NAME", "value": 0/1}}

Examples:

1. GPIO presence check:
{"collection": {"gpioPresence": {"pin": "CARD_PRESENT_N", "value": 0}}}

2. Multiple actions:
{"collection": {"gpioPresence": {"pin": "PIN1", "value": 0}, "setGpio": {"pin": "PIN2", "value": 1}}}

3. System command:
{"deletion": {"systemCmd": {"cmd": "echo 7-0051 > /sys/bus/i2c/drivers/at24/unbind"}}}

4. Complex preAction:
{"collection": {"gpioPresence": {"pin": "OPPANEL_PRESENT_N", "value": 0}, "setGpio": {"pin": "FW_I2C_ENABLE_N", "value": 0}, "systemCmd": {"cmd": "echo 7-0051 > /sys/bus/i2c/drivers/at24/bind"}}}`;
            
            alert(helpMessage);
        }
    </script>
</body>
</html>
"""
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(html.encode())
    
    def serve_config(self):
        """Serve current configuration"""
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(self.generator.config_data).encode())
    
    def serve_templates(self):
        """Serve list of available templates"""
        templates_dir = Path('../configuration/ibm')
        templates = []
        if templates_dir.exists():
            templates = [f.name for f in templates_dir.glob('*.json')]
        
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(templates).encode())
    
    def handle_save(self, post_data):
        """Handle save operations"""
        try:
            data = json.loads(post_data.decode())
            action = data.get('action')
            
            if action == 'basic':
                self.generator.set_basic_config(
                    data['devTree'],
                    data['biosHandler'],
                    data['backupRestore']
                )
                result = {'success': True}
            
            elif action == 'mux':
                self.generator.add_mux(
                    data['i2bus'],
                    data['address'],
                    data['path']
                )
                result = {'success': True}
            
            elif action == 'fru':
                fru_config = {
                    'inventoryPath': data['inventory'],
                    'serviceName': data['service'],
                    'extraInterfaces': {
                        'xyz.openbmc_project.Inventory.Item': {
                            'PrettyName': data['pretty']
                        }
                    }
                }
                
                # Add optional boolean fields
                if data.get('isSystemVpd'):
                    fru_config['isSystemVpd'] = True
                
                if not data.get('inherit', True):
                    fru_config['inherit'] = False
                
                if data.get('replaceableAtRuntime'):
                    fru_config['replaceableAtRuntime'] = True
                
                if data.get('replaceableAtStandby'):
                    fru_config['replaceableAtStandby'] = True
                
                if data.get('essentialFru'):
                    fru_config['essentialFru'] = True
                
                if data.get('powerOffOnly'):
                    fru_config['powerOffOnly'] = True
                
                if data.get('concurrentlyMaintainable'):
                    fru_config['concurrentlyMaintainable'] = True
                
                if data.get('embedded'):
                    fru_config['embedded'] = True
                
                # Add action fields (parse JSON)
                for action_field in ['preAction', 'postAction', 'postFailAction', 'pollingRequired']:
                    action_value = data.get(action_field, '').strip()
                    if action_value:
                        try:
                            fru_config[action_field] = json.loads(action_value)
                        except json.JSONDecodeError:
                            raise ValidationError(f"{action_field} must be valid JSON")
                
                # Add additional interface
                additional_interface = data.get('additionalInterface', '').strip()
                if additional_interface:
                    fru_config['extraInterfaces'][additional_interface] = None
                
                self.generator.add_fru(data['eeprom'], fru_config)
                result = {'success': True}
            
            else:
                result = {'success': False, 'error': 'Unknown action'}
        
        except ValidationError as e:
            result = {'success': False, 'error': str(e)}
        except Exception as e:
            result = {'success': False, 'error': str(e)}
        
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(result).encode())
    
    def handle_validate(self, post_data):
        """Handle validation"""
        try:
            self.generator.validate()
            result = {'valid': True}
        except ValidationError as e:
            result = {'valid': False, 'error': str(e)}
        
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(result).encode())
    
    def handle_export(self, post_data):
        """Handle export"""
        try:
            data = json.loads(post_data.decode())
            filename = data.get('filename', 'config.json')
            filepath = os.path.join('../configuration/ibm', filename)
            
            self.generator.export_json(filepath)
            result = {'success': True, 'path': filepath}
        except Exception as e:
            result = {'success': False, 'error': str(e)}
        
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(result).encode())
    
    def handle_load(self, post_data):
        """Handle template loading"""
        try:
            data = json.loads(post_data.decode())
            filename = data.get('filename')
            filepath = os.path.join('../configuration/ibm', filename)
            
            self.generator.load_template(filepath)
            result = {'success': True}
        except Exception as e:
            result = {'success': False, 'error': str(e)}
        
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(result).encode())


def main():
    """Start the web server"""
    port = 8080
    server = HTTPServer(('localhost', port), VPDWebHandler)
    
    print("=" * 60)
    print("VPD Configuration Generator - Web Interface")
    print("=" * 60)
    print(f"\nServer starting on http://localhost:{port}")
    print("\nOpening browser...")
    print("\nPress Ctrl+C to stop the server")
    print("=" * 60)
    
    # Open browser
    webbrowser.open(f'http://localhost:{port}')
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n\nServer stopped.")
        server.shutdown()


if __name__ == "__main__":
    main()

# Made with Bob

#!/usr/bin/env python3
"""
VPD Configuration Generator Tool
A GUI-based tool for generating IBM OpenBMC VPD system configuration JSON files.
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import json
import os
import re
from typing import Dict, Any, List, Optional
from pathlib import Path


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
    def validate_dbus_interface(value: str, field_name: str) -> str:
        """Validate D-Bus interface names"""
        value = value.strip()
        if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*(\.[a-zA-Z_][a-zA-Z0-9_]*)+$', value):
            raise ValidationError(f"{field_name} must be a valid D-Bus interface name")
        return value
    
    @staticmethod
    def validate_boolean(value: Any, field_name: str) -> bool:
        """Validate boolean values"""
        if isinstance(value, bool):
            return value
        if isinstance(value, str):
            if value.lower() in ('true', '1', 'yes'):
                return True
            if value.lower() in ('false', '0', 'no'):
                return False
        raise ValidationError(f"{field_name} must be a boolean value (true/false)")
    
    @staticmethod
    def validate_json_structure(data: Dict[str, Any]) -> None:
        """Validate the complete JSON structure"""
        required_fields = ['devTree', 'biosHandlerJsonPath', 'backupRestoreConfigPath', 
                          'commonInterfaces', 'muxes', 'frus']
        
        for field in required_fields:
            if field not in data:
                raise ValidationError(f"Required top-level field '{field}' is missing")
        
        # Validate commonInterfaces structure
        if 'xyz.openbmc_project.Inventory.Decorator.Asset' not in data['commonInterfaces']:
            raise ValidationError("commonInterfaces must contain 'xyz.openbmc_project.Inventory.Decorator.Asset'")
        
        # Validate muxes is a list
        if not isinstance(data['muxes'], list):
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


class VPDConfigGUI:
    """Main GUI application"""
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("VPD Configuration Generator")
        self.root.geometry("1000x700")
        
        self.generator = ConfigGenerator()
        self.current_section = None
        
        # Configure style
        style = ttk.Style()
        style.theme_use('aqua' if os.name == 'posix' else 'clam')
        
        self.setup_ui()
    
    def setup_ui(self):
        """Setup the user interface"""
        # Menu bar
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="New Configuration", command=self.new_config)
        file_menu.add_command(label="Load Template", command=self.load_template)
        file_menu.add_separator()
        file_menu.add_command(label="Export JSON", command=self.export_json)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        # Edit menu
        edit_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Edit", menu=edit_menu)
        edit_menu.add_command(label="Basic Configuration", command=self.show_basic_config)
        edit_menu.add_command(label="Common Interfaces", command=self.show_common_interfaces)
        edit_menu.add_command(label="Muxes", command=self.show_muxes)
        edit_menu.add_command(label="FRUs", command=self.show_frus)
        
        # Validate menu
        validate_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Validate", menu=validate_menu)
        validate_menu.add_command(label="Validate Configuration", command=self.validate_config)
        validate_menu.add_command(label="View JSON", command=self.view_json)
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self.show_about)
        help_menu.add_command(label="Documentation", command=self.show_documentation)
        
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)
        
        # Title
        title_label = ttk.Label(main_frame, text="VPD Configuration Generator", 
                               font=('Helvetica', 18, 'bold'))
        title_label.grid(row=0, column=0, pady=10)
        
        # Content frame (will be replaced based on selection)
        self.content_frame = ttk.Frame(main_frame)
        self.content_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.content_frame.columnconfigure(0, weight=1)
        self.content_frame.rowconfigure(0, weight=1)
        
        # Status bar
        self.status_var = tk.StringVar(value="Ready")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, 
                              relief=tk.SUNKEN, anchor=tk.W)
        status_bar.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=(5, 0))
        
        # Show welcome screen
        self.show_welcome()
    
    def clear_content_frame(self):
        """Clear the content frame"""
        for widget in self.content_frame.winfo_children():
            widget.destroy()
    
    def show_welcome(self):
        """Show welcome screen"""
        self.clear_content_frame()
        
        welcome_frame = ttk.Frame(self.content_frame)
        welcome_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        welcome_frame.columnconfigure(0, weight=1)
        
        ttk.Label(welcome_frame, text="Welcome to VPD Configuration Generator", 
                 font=('Helvetica', 16)).grid(row=0, column=0, pady=20)
        
        ttk.Label(welcome_frame, text="This tool helps you create system configuration JSON files\n"
                 "for IBM OpenBMC VPD (Vital Product Data) systems.",
                 justify=tk.CENTER).grid(row=1, column=0, pady=10)
        
        ttk.Label(welcome_frame, text="Get started by:", 
                 font=('Helvetica', 12, 'bold')).grid(row=2, column=0, pady=(20, 10))
        
        button_frame = ttk.Frame(welcome_frame)
        button_frame.grid(row=3, column=0, pady=10)
        
        ttk.Button(button_frame, text="Creating a New Configuration", 
                  command=self.new_config, width=30).grid(row=0, column=0, pady=5)
        ttk.Button(button_frame, text="Loading an Existing Template", 
                  command=self.load_template, width=30).grid(row=1, column=0, pady=5)
        
        self.status_var.set("Welcome! Start by creating a new configuration or loading a template.")
    
    def new_config(self):
        """Create a new configuration"""
        if messagebox.askyesno("New Configuration", 
                              "This will clear the current configuration. Continue?"):
            self.generator = ConfigGenerator()
            self.show_basic_config()
            self.status_var.set("New configuration created")
    
    def load_template(self):
        """Load an existing configuration template"""
        filepath = filedialog.askopenfilename(
            title="Load Configuration Template",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
            initialdir="./configuration/ibm"
        )
        
        if filepath:
            try:
                self.generator.load_template(filepath)
                self.show_basic_config()
                self.status_var.set(f"Template loaded: {os.path.basename(filepath)}")
                messagebox.showinfo("Success", "Template loaded successfully!")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load template:\n{str(e)}")
    
    def show_basic_config(self):
        """Show basic configuration editor"""
        self.clear_content_frame()
        
        config_frame = ttk.Frame(self.content_frame, padding="10")
        config_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        ttk.Label(config_frame, text="Basic Configuration", 
                 font=('Helvetica', 14, 'bold')).grid(row=0, column=0, columnspan=2, pady=10)
        
        # Device Tree
        ttk.Label(config_frame, text="Device Tree:").grid(row=1, column=0, sticky=tk.W, pady=5)
        dev_tree_var = tk.StringVar(value=self.generator.config_data.get("devTree", ""))
        dev_tree_entry = ttk.Entry(config_frame, textvariable=dev_tree_var, width=50)
        dev_tree_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), pady=5, padx=5)
        ttk.Label(config_frame, text="e.g., conf-aspeed-bmc-ibm-rainier.dtb", 
                 font=('Helvetica', 9), foreground='gray').grid(row=2, column=1, sticky=tk.W, padx=5)
        
        # BIOS Handler Path
        ttk.Label(config_frame, text="BIOS Handler JSON Path:").grid(row=3, column=0, sticky=tk.W, pady=5)
        bios_var = tk.StringVar(value=self.generator.config_data.get("biosHandlerJsonPath", ""))
        bios_entry = ttk.Entry(config_frame, textvariable=bios_var, width=50)
        bios_entry.grid(row=3, column=1, sticky=(tk.W, tk.E), pady=5, padx=5)
        ttk.Label(config_frame, text="e.g., /usr/share/vpd/bios_map_50001001.json", 
                 font=('Helvetica', 9), foreground='gray').grid(row=4, column=1, sticky=tk.W, padx=5)
        
        # Backup Restore Path
        ttk.Label(config_frame, text="Backup Restore Config Path:").grid(row=5, column=0, sticky=tk.W, pady=5)
        backup_var = tk.StringVar(value=self.generator.config_data.get("backupRestoreConfigPath", ""))
        backup_entry = ttk.Entry(config_frame, textvariable=backup_var, width=50)
        backup_entry.grid(row=5, column=1, sticky=(tk.W, tk.E), pady=5, padx=5)
        ttk.Label(config_frame, text="e.g., /usr/share/vpd/backup_restore_50001000.json", 
                 font=('Helvetica', 9), foreground='gray').grid(row=6, column=1, sticky=tk.W, padx=5)
        
        # Save button
        def save_basic_config():
            try:
                self.generator.set_basic_config(
                    dev_tree_var.get(),
                    bios_var.get(),
                    backup_var.get()
                )
                self.status_var.set("Basic configuration saved")
                messagebox.showinfo("Success", "Basic configuration saved successfully!")
            except ValidationError as e:
                messagebox.showerror("Validation Error", str(e))
        
        ttk.Button(config_frame, text="Save Basic Configuration", 
                  command=save_basic_config).grid(row=7, column=0, columnspan=2, pady=20)
        
        config_frame.columnconfigure(1, weight=1)
    
    def show_common_interfaces(self):
        """Show common interfaces editor"""
        self.clear_content_frame()
        
        interface_frame = ttk.Frame(self.content_frame, padding="10")
        interface_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        ttk.Label(interface_frame, text="Common Interfaces Configuration", 
                 font=('Helvetica', 14, 'bold')).grid(row=0, column=0, columnspan=3, pady=10)
        
        ttk.Label(interface_frame, text="Asset Decorator Interface", 
                 font=('Helvetica', 12, 'bold')).grid(row=1, column=0, columnspan=3, pady=10)
        
        asset_data = self.generator.config_data["commonInterfaces"]["xyz.openbmc_project.Inventory.Decorator.Asset"]
        
        row = 2
        entries = {}
        
        for field_name, field_data in asset_data.items():
            ttk.Label(interface_frame, text=f"{field_name}:").grid(row=row, column=0, sticky=tk.W, pady=5)
            
            # Record Name
            ttk.Label(interface_frame, text="Record:").grid(row=row, column=1, sticky=tk.E, padx=(10, 5))
            record_var = tk.StringVar(value=field_data.get("recordName", ""))
            record_entry = ttk.Entry(interface_frame, textvariable=record_var, width=15)
            record_entry.grid(row=row, column=2, sticky=tk.W, pady=5)
            
            # Keyword Name
            ttk.Label(interface_frame, text="Keyword:").grid(row=row+1, column=1, sticky=tk.E, padx=(10, 5))
            keyword_var = tk.StringVar(value=field_data.get("keywordName", ""))
            keyword_entry = ttk.Entry(interface_frame, textvariable=keyword_var, width=15)
            keyword_entry.grid(row=row+1, column=2, sticky=tk.W, pady=5)
            
            # Encoding (optional)
            if "encoding" in field_data:
                ttk.Label(interface_frame, text="Encoding:").grid(row=row+2, column=1, sticky=tk.E, padx=(10, 5))
                encoding_var = tk.StringVar(value=field_data.get("encoding", ""))
                encoding_entry = ttk.Entry(interface_frame, textvariable=encoding_var, width=15)
                encoding_entry.grid(row=row+2, column=2, sticky=tk.W, pady=5)
                entries[field_name] = (record_var, keyword_var, encoding_var)
                row += 3
            else:
                entries[field_name] = (record_var, keyword_var, None)
                row += 2
            
            ttk.Separator(interface_frame, orient='horizontal').grid(row=row, column=0, columnspan=3, 
                                                                     sticky=(tk.W, tk.E), pady=10)
            row += 1
        
        def save_interfaces():
            try:
                for field_name, vars_tuple in entries.items():
                    record_var, keyword_var, encoding_var = vars_tuple
                    self.generator.config_data["commonInterfaces"]["xyz.openbmc_project.Inventory.Decorator.Asset"][field_name] = {
                        "recordName": record_var.get(),
                        "keywordName": keyword_var.get()
                    }
                    if encoding_var:
                        self.generator.config_data["commonInterfaces"]["xyz.openbmc_project.Inventory.Decorator.Asset"][field_name]["encoding"] = encoding_var.get()
                
                self.status_var.set("Common interfaces saved")
                messagebox.showinfo("Success", "Common interfaces saved successfully!")
            except Exception as e:
                messagebox.showerror("Error", str(e))
        
        ttk.Button(interface_frame, text="Save Common Interfaces", 
                  command=save_interfaces).grid(row=row, column=0, columnspan=3, pady=20)
    
    def show_muxes(self):
        """Show muxes configuration editor"""
        self.clear_content_frame()
        
        mux_frame = ttk.Frame(self.content_frame, padding="10")
        mux_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        mux_frame.columnconfigure(0, weight=1)
        mux_frame.rowconfigure(1, weight=1)
        
        ttk.Label(mux_frame, text="Muxes Configuration", 
                 font=('Helvetica', 14, 'bold')).grid(row=0, column=0, pady=10)
        
        # Muxes list
        list_frame = ttk.Frame(mux_frame)
        list_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        list_frame.columnconfigure(0, weight=1)
        list_frame.rowconfigure(0, weight=1)
        
        # Treeview for muxes
        columns = ('I2C Bus', 'Device Address', 'Hold Idle Path')
        tree = ttk.Treeview(list_frame, columns=columns, show='headings', height=10)
        
        for col in columns:
            tree.heading(col, text=col)
            tree.column(col, width=200)
        
        # Populate tree
        for mux in self.generator.config_data["muxes"]:
            tree.insert('', tk.END, values=(mux["i2bus"], mux["deviceaddress"], mux["holdidlepath"]))
        
        tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=tree.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        button_frame = ttk.Frame(mux_frame)
        button_frame.grid(row=2, column=0, pady=10)
        
        def add_mux():
            self.show_add_mux_dialog(tree)
        
        def delete_mux():
            selected = tree.selection()
            if selected:
                if messagebox.askyesno("Confirm Delete", "Delete selected mux?"):
                    index = tree.index(selected[0])
                    self.generator.config_data["muxes"].pop(index)
                    tree.delete(selected[0])
                    self.status_var.set("Mux deleted")
            else:
                messagebox.showwarning("No Selection", "Please select a mux to delete")
        
        ttk.Button(button_frame, text="Add Mux", command=add_mux).grid(row=0, column=0, padx=5)
        ttk.Button(button_frame, text="Delete Selected", command=delete_mux).grid(row=0, column=1, padx=5)
    
    def show_add_mux_dialog(self, tree):
        """Show dialog to add a new mux"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Add Mux")
        dialog.geometry("400x250")
        dialog.transient(self.root)
        dialog.grab_set()
        
        frame = ttk.Frame(dialog, padding="20")
        frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        ttk.Label(frame, text="I2C Bus:").grid(row=0, column=0, sticky=tk.W, pady=5)
        i2bus_var = tk.StringVar()
        ttk.Entry(frame, textvariable=i2bus_var, width=30).grid(row=0, column=1, pady=5)
        ttk.Label(frame, text="e.g., 4", font=('Helvetica', 9), 
                 foreground='gray').grid(row=1, column=1, sticky=tk.W)
        
        ttk.Label(frame, text="Device Address:").grid(row=2, column=0, sticky=tk.W, pady=5)
        addr_var = tk.StringVar()
        ttk.Entry(frame, textvariable=addr_var, width=30).grid(row=2, column=1, pady=5)
        ttk.Label(frame, text="e.g., 0xE0", font=('Helvetica', 9), 
                 foreground='gray').grid(row=3, column=1, sticky=tk.W)
        
        ttk.Label(frame, text="Hold Idle Path:").grid(row=4, column=0, sticky=tk.W, pady=5)
        path_var = tk.StringVar()
        ttk.Entry(frame, textvariable=path_var, width=30).grid(row=4, column=1, pady=5)
        ttk.Label(frame, text="e.g., /sys/bus/i2c/drivers/pca954x/4-0070/hold_idle", 
                 font=('Helvetica', 9), foreground='gray').grid(row=5, column=1, sticky=tk.W)
        
        def save_mux():
            try:
                self.generator.add_mux(i2bus_var.get(), addr_var.get(), path_var.get())
                tree.insert('', tk.END, values=(i2bus_var.get(), addr_var.get(), path_var.get()))
                self.status_var.set("Mux added successfully")
                dialog.destroy()
            except ValidationError as e:
                messagebox.showerror("Validation Error", str(e), parent=dialog)
        
        button_frame = ttk.Frame(frame)
        button_frame.grid(row=6, column=0, columnspan=2, pady=20)
        ttk.Button(button_frame, text="Add", command=save_mux).grid(row=0, column=0, padx=5)
        ttk.Button(button_frame, text="Cancel", command=dialog.destroy).grid(row=0, column=1, padx=5)
    
    def show_frus(self):
        """Show FRUs configuration editor"""
        self.clear_content_frame()
        
        fru_frame = ttk.Frame(self.content_frame, padding="10")
        fru_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        fru_frame.columnconfigure(0, weight=1)
        fru_frame.rowconfigure(1, weight=1)
        
        ttk.Label(fru_frame, text="FRUs Configuration", 
                 font=('Helvetica', 14, 'bold')).grid(row=0, column=0, pady=10)
        
        # FRU list
        list_frame = ttk.Frame(fru_frame)
        list_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        list_frame.columnconfigure(0, weight=1)
        list_frame.rowconfigure(0, weight=1)
        
        # Treeview for FRUs
        tree = ttk.Treeview(list_frame, columns=('EEPROM Path', 'Count'), show='headings', height=15)
        tree.heading('EEPROM Path', text='EEPROM Path')
        tree.heading('Count', text='# of Configs')
        tree.column('EEPROM Path', width=500)
        tree.column('Count', width=100)
        
        # Populate tree
        for eeprom_path, configs in self.generator.config_data["frus"].items():
            tree.insert('', tk.END, values=(eeprom_path, len(configs)))
        
        tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=tree.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        button_frame = ttk.Frame(fru_frame)
        button_frame.grid(row=2, column=0, pady=10)
        
        def add_fru():
            self.show_add_fru_dialog(tree)
        
        def view_fru():
            selected = tree.selection()
            if selected:
                eeprom_path = tree.item(selected[0])['values'][0]
                self.show_fru_details(eeprom_path)
            else:
                messagebox.showwarning("No Selection", "Please select a FRU to view")
        
        def delete_fru():
            selected = tree.selection()
            if selected:
                if messagebox.askyesno("Confirm Delete", "Delete selected FRU?"):
                    eeprom_path = tree.item(selected[0])['values'][0]
                    del self.generator.config_data["frus"][eeprom_path]
                    tree.delete(selected[0])
                    self.status_var.set("FRU deleted")
            else:
                messagebox.showwarning("No Selection", "Please select a FRU to delete")
        
        ttk.Button(button_frame, text="Add FRU", command=add_fru).grid(row=0, column=0, padx=5)
        ttk.Button(button_frame, text="View Details", command=view_fru).grid(row=0, column=1, padx=5)
        ttk.Button(button_frame, text="Delete Selected", command=delete_fru).grid(row=0, column=2, padx=5)
    
    def show_add_fru_dialog(self, tree):
        """Show dialog to add a new FRU"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Add FRU")
        dialog.geometry("500x400")
        dialog.transient(self.root)
        dialog.grab_set()
        
        frame = ttk.Frame(dialog, padding="20")
        frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        frame.columnconfigure(1, weight=1)
        
        ttk.Label(frame, text="EEPROM Path:").grid(row=0, column=0, sticky=tk.W, pady=5)
        eeprom_var = tk.StringVar()
        ttk.Entry(frame, textvariable=eeprom_var, width=40).grid(row=0, column=1, pady=5, sticky=(tk.W, tk.E))
        ttk.Label(frame, text="e.g., /sys/bus/i2c/drivers/at24/8-0050/eeprom", 
                 font=('Helvetica', 9), foreground='gray').grid(row=1, column=1, sticky=tk.W)
        
        ttk.Label(frame, text="Inventory Path:").grid(row=2, column=0, sticky=tk.W, pady=5)
        inv_var = tk.StringVar()
        ttk.Entry(frame, textvariable=inv_var, width=40).grid(row=2, column=1, pady=5, sticky=(tk.W, tk.E))
        
        ttk.Label(frame, text="Service Name:").grid(row=3, column=0, sticky=tk.W, pady=5)
        service_var = tk.StringVar(value="xyz.openbmc_project.Inventory.Manager")
        ttk.Entry(frame, textvariable=service_var, width=40).grid(row=3, column=1, pady=5, sticky=(tk.W, tk.E))
        
        ttk.Label(frame, text="Pretty Name:").grid(row=4, column=0, sticky=tk.W, pady=5)
        pretty_var = tk.StringVar()
        ttk.Entry(frame, textvariable=pretty_var, width=40).grid(row=4, column=1, pady=5, sticky=(tk.W, tk.E))
        
        # Checkboxes
        system_vpd_var = tk.BooleanVar()
        ttk.Checkbutton(frame, text="Is System VPD", variable=system_vpd_var).grid(row=5, column=1, sticky=tk.W, pady=5)
        
        inherit_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(frame, text="Inherit", variable=inherit_var).grid(row=6, column=1, sticky=tk.W, pady=5)
        
        def save_fru():
            try:
                eeprom_path = self.generator.validator.validate_path(eeprom_var.get(), "EEPROM Path")
                
                fru_config = {
                    "inventoryPath": inv_var.get(),
                    "serviceName": service_var.get(),
                    "extraInterfaces": {
                        "xyz.openbmc_project.Inventory.Item": {
                            "PrettyName": pretty_var.get()
                        }
                    }
                }
                
                if system_vpd_var.get():
                    fru_config["isSystemVpd"] = True
                
                if not inherit_var.get():
                    fru_config["inherit"] = False
                
                self.generator.add_fru(eeprom_path, fru_config)
                
                # Update tree
                existing = None
                for item in tree.get_children():
                    if tree.item(item)['values'][0] == eeprom_path:
                        existing = item
                        break
                
                if existing:
                    count = len(self.generator.config_data["frus"][eeprom_path])
                    tree.item(existing, values=(eeprom_path, count))
                else:
                    tree.insert('', tk.END, values=(eeprom_path, 1))
                
                self.status_var.set("FRU added successfully")
                dialog.destroy()
            except ValidationError as e:
                messagebox.showerror("Validation Error", str(e), parent=dialog)
        
        button_frame = ttk.Frame(frame)
        button_frame.grid(row=7, column=0, columnspan=2, pady=20)
        ttk.Button(button_frame, text="Add", command=save_fru).grid(row=0, column=0, padx=5)
        ttk.Button(button_frame, text="Cancel", command=dialog.destroy).grid(row=0, column=1, padx=5)
    
    def show_fru_details(self, eeprom_path: str):
        """Show FRU configuration details"""
        dialog = tk.Toplevel(self.root)
        dialog.title(f"FRU Details: {eeprom_path}")
        dialog.geometry("700x500")
        dialog.transient(self.root)
        
        frame = ttk.Frame(dialog, padding="10")
        frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(0, weight=1)
        
        # Text widget with scrollbar
        text_widget = scrolledtext.ScrolledText(frame, wrap=tk.WORD, width=80, height=25)
        text_widget.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Display FRU configurations
        configs = self.generator.config_data["frus"][eeprom_path]
        text_widget.insert(tk.END, json.dumps(configs, indent=4))
        text_widget.config(state=tk.DISABLED)
        
        ttk.Button(frame, text="Close", command=dialog.destroy).grid(row=1, column=0, pady=10)
    
    def validate_config(self):
        """Validate the current configuration"""
        try:
            self.generator.validate()
            messagebox.showinfo("Validation Success", 
                              "Configuration is valid!\n\nAll required fields are present and properly formatted.")
            self.status_var.set("Configuration validated successfully")
        except ValidationError as e:
            messagebox.showerror("Validation Error", f"Configuration validation failed:\n\n{str(e)}")
            self.status_var.set("Validation failed")
    
    def view_json(self):
        """View the current configuration as JSON"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Configuration JSON")
        dialog.geometry("800x600")
        dialog.transient(self.root)
        
        frame = ttk.Frame(dialog, padding="10")
        frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(0, weight=1)
        
        dialog.columnconfigure(0, weight=1)
        dialog.rowconfigure(0, weight=1)
        
        # Text widget with scrollbar
        text_widget = scrolledtext.ScrolledText(frame, wrap=tk.WORD, width=90, height=30)
        text_widget.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Display JSON
        json_str = json.dumps(self.generator.config_data, indent=4)
        text_widget.insert(tk.END, json_str)
        text_widget.config(state=tk.DISABLED)
        
        button_frame = ttk.Frame(frame)
        button_frame.grid(row=1, column=0, pady=10)
        
        def copy_to_clipboard():
            self.root.clipboard_clear()
            self.root.clipboard_append(json_str)
            self.status_var.set("JSON copied to clipboard")
            messagebox.showinfo("Copied", "JSON copied to clipboard!", parent=dialog)
        
        ttk.Button(button_frame, text="Copy to Clipboard", 
                  command=copy_to_clipboard).grid(row=0, column=0, padx=5)
        ttk.Button(button_frame, text="Close", command=dialog.destroy).grid(row=0, column=1, padx=5)
    
    def export_json(self):
        """Export configuration to JSON file"""
        try:
            self.generator.validate()
            
            filepath = filedialog.asksaveasfilename(
                title="Export Configuration",
                defaultextension=".json",
                filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
                initialdir="./configuration/ibm"
            )
            
            if filepath:
                self.generator.export_json(filepath)
                self.status_var.set(f"Configuration exported: {os.path.basename(filepath)}")
                messagebox.showinfo("Success", f"Configuration exported successfully to:\n{filepath}")
        except ValidationError as e:
            messagebox.showerror("Validation Error", 
                               f"Cannot export invalid configuration:\n\n{str(e)}")
    
    def show_about(self):
        """Show about dialog"""
        messagebox.showinfo("About", 
                          "VPD Configuration Generator\n\n"
                          "Version 1.0\n\n"
                          "A tool for generating IBM OpenBMC VPD system configuration JSON files.\n\n"
                          "Features:\n"
                          "• Hierarchical menu-based configuration\n"
                          "• Field validation\n"
                          "• JSON structure validation\n"
                          "• Template loading\n"
                          "• Export to JSON")
    
    def show_documentation(self):
        """Show documentation"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Documentation")
        dialog.geometry("700x500")
        dialog.transient(self.root)
        
        frame = ttk.Frame(dialog, padding="10")
        frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(0, weight=1)
        
        dialog.columnconfigure(0, weight=1)
        dialog.rowconfigure(0, weight=1)
        
        text_widget = scrolledtext.ScrolledText(frame, wrap=tk.WORD, width=80, height=25)
        text_widget.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        doc_text = """
VPD Configuration Generator - Documentation

OVERVIEW
--------
This tool helps you create system configuration JSON files for IBM OpenBMC VPD 
(Vital Product Data) systems. The configuration defines hardware components, 
their locations, and how they should be managed.

GETTING STARTED
---------------
1. Start by creating a new configuration or loading an existing template
2. Fill in the basic configuration (device tree, paths)
3. Configure common interfaces (asset information)
4. Add muxes (I2C multiplexers)
5. Add FRUs (Field Replaceable Units)
6. Validate and export your configuration

CONFIGURATION SECTIONS
----------------------

Basic Configuration:
- Device Tree: The device tree blob file name
- BIOS Handler Path: Path to BIOS mapping JSON
- Backup Restore Path: Path to backup/restore configuration

Common Interfaces:
- Defines standard D-Bus interfaces for asset information
- Includes part numbers, serial numbers, model info, etc.

Muxes:
- I2C multiplexer configurations
- Each mux has: I2C bus number, device address, hold idle path

FRUs:
- Field Replaceable Unit configurations
- Each FRU has: EEPROM path, inventory path, service name, interfaces

VALIDATION
----------
The tool validates:
- Required fields are present
- Field formats (hex addresses, paths, etc.)
- Overall JSON structure
- D-Bus interface names

TIPS
----
- Load an existing configuration as a template to get started quickly
- Use the "View JSON" option to see your configuration at any time
- Validate frequently to catch errors early
- Save your work by exporting to JSON regularly

For more information, see the documentation in the docs/ directory.
"""
        
        text_widget.insert(tk.END, doc_text)
        text_widget.config(state=tk.DISABLED)
        
        ttk.Button(frame, text="Close", command=dialog.destroy).grid(row=1, column=0, pady=10)


def main():
    """Main entry point"""
    root = tk.Tk()
    app = VPDConfigGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()

# Made with Bob

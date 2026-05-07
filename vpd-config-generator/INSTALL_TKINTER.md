# Installing tkinter on macOS

## The Issue

Your Python installation from Homebrew doesn't include tkinter support. This is a common issue with Homebrew Python.

## Solutions

### Option 1: Install python-tk via Homebrew (Recommended)

```bash
# Install python-tk package
brew install python-tk@3.13

# Verify installation
python3 -m tkinter
```

If this opens a small test window, tkinter is working!

### Option 2: Use System Python

macOS comes with Python that includes tkinter:

```bash
# Check if system Python has tkinter
/usr/bin/python3 -m tkinter

# If it works, use system Python to run the tool
/usr/bin/python3 vpd_config_generator.py
```

### Option 3: Install Python from python.org

Download and install Python from [python.org](https://www.python.org/downloads/macos/), which includes tkinter by default.

### Option 4: Use pyenv with tkinter support

```bash
# Install pyenv if not already installed
brew install pyenv

# Install Python with tkinter support
env PYTHON_CONFIGURE_OPTS="--with-tcltk-includes='-I$(brew --prefix tcl-tk)/include' --with-tcltk-libs='-L$(brew --prefix tcl-tk)/lib -ltcl8.6 -ltk8.6'" pyenv install 3.13.0

# Set as global or local version
pyenv global 3.13.0
```

## Quick Fix for Your Current Setup

The easiest solution for your Homebrew Python:

```bash
# Install python-tk
brew install python-tk@3.13

# Run the tool
python3 vpd_config_generator.py
```

## Alternative: Use the Web-Based Version

If you prefer not to install tkinter, use the web-based version instead:

```bash
python3 vpd_config_generator_web.py
```

This will start a local web server and open the tool in your browser - no tkinter required!

## Verify Installation

After installing tkinter, verify it works:

```bash
python3 -c "import tkinter; print('tkinter is available')"
```

If you see "tkinter is available", you're ready to go!

## Troubleshooting

### Still getting ModuleNotFoundError?

1. Check which Python you're using:
   ```bash
   which python3
   python3 --version
   ```

2. Make sure you installed python-tk for the correct version:
   ```bash
   brew list | grep python-tk
   ```

3. Try reinstalling:
   ```bash
   brew reinstall python-tk@3.13
   ```

### Multiple Python Installations?

If you have multiple Python installations, make sure you're using the one with tkinter:

```bash
# List all Python installations
ls -la /usr/local/bin/python*
ls -la /opt/homebrew/bin/python*

# Test each one
/usr/local/bin/python3 -m tkinter
/opt/homebrew/bin/python3 -m tkinter
/usr/bin/python3 -m tkinter
```

Use the one that works!
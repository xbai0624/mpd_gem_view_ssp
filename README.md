
## Table of Contents

- [Prerequisites](#prerequisites)
- [Installation](#installation)
  - [Linux/Ubuntu](#linuxubuntu)
  - [Windows (WSL)](#windows-wsl)
  - [macOS](#macos)
- [Building](#building)
- [Running](#running)
- [Troubleshooting](#troubleshooting)
- [TLDR](#quick-start-summary)

---

## Prerequisites

### Required Software

1. **ROOT** (6.20 or later recommended)
   - CERN's data analysis framework
   - Download from: https://root.cern.ch/

2. **Qt5** (5.15 or later)
   - Qt5 development tools and libraries

3. **C++17 Compiler**
   - GCC 9+ or Clang 10+

4. **qmake** (Qt build tool)

5. **X11 libraries** (for GUI on Linux/WSL)

---

## Installation

### Linux/Ubuntu

#### 1. Install Build Tools and Dependencies

```bash
# Update package list
sudo apt update

# Install Qt5 and qmake
sudo apt install qt5-qmake qtbase5-dev qtbase5-dev-tools \
                 libqt5widgets5 libqt5gui5 libqt5core5a libqt5x11extras5

# Install X11 libraries
sudo apt install libxcb-xinerama0 libxcb-icccm4 libxcb-image0 \
                 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 \
                 libxcb-xkb1 libxkbcommon-x11-0 libxft2 libxft-dev

# Install build essentials
sudo apt install build-essential g++ cmake dos2unix

# Verify installations
qmake --version
g++ --version
```

#### 2. Install ROOT

**Install binary**

```bash
# Create software directory
mkdir -p ~/software
cd ~/software

# Download ROOT (adjust version for your Ubuntu version)
# For Ubuntu 22.04:
wget https://root.cern/download/root_v6.28.10.Linux-ubuntu22-x86_64-gcc11.4.tar.gz

# For Ubuntu 24.04, check: https://root.cern/install/all_releases/

# Extract
tar -xzf root_v6.28.10.Linux-ubuntu22-x86_64-gcc11.4.tar.gz

# Add to shell configuration
echo "" >> ~/.bashrc
echo "# ROOT Setup" >> ~/.bashrc
echo "source ~/software/root/bin/thisroot.sh" >> ~/.bashrc
source ~/.bashrc

# Verify
root-config --version
```

### Windows (WSL)

**Prerequisites:**
- Windows 10 (Version 2004+) or Windows 11
- WSL 2 installed

#### 1. Enable WSLg (GUI Support)

**For Windows 11:**

```powershell
# In PowerShell (Admin)
wsl --update
wsl --shutdown
```

**For Windows 10:**

Install VcXsrv X Server:

```powershell
winget install VcXsrv
```

Then launch XLaunch with "Disable access control" enabled.

#### 2. Follow Linux Installation Steps

After enabling GUI support, follow the [Linux/Ubuntu](#linuxubuntu) installation steps above.

#### 3. Configure Display (WSL)

```bash
# For Windows 11 (WSLg)
echo 'export DISPLAY=:0' >> ~/.bashrc

# For Windows 10 with VcXsrv
echo 'export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '"'"'{print $2}'"'"'):0' >> ~/.bashrc

source ~/.bashrc
```

#### 4. Test GUI Support

```bash
sudo apt install x11-apps
xclock  # Should display a clock window
```

---

### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install qt@5 root

# Add Qt to PATH
echo 'export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# Verify
qmake --version
root-config --version
```

---

## Building

### 1. Clone/Download the Repository

```bash
cd ~/your/workspace
# If using git:
git clone <repository-url>
cd mpd_gem_view_ssp
```

### 2. Fix Line Endings (Important for Linux/WSL)

If cloning from Windows or transferring files:

```bash
# Install dos2unix if not already installed
sudo apt install dos2unix  # Linux
brew install dos2unix      # macOS

# Convert all shell scripts and config files
find . -name "*.sh" -exec dos2unix {} \;
dos2unix config/*.conf*
```

### 3. Fix Linux Compatibility Issues

**Add missing headers** (if not already added):

In `decoder/include/MPDDataStruct.h`, add after existing includes:

```cpp
#include <cstdint>
```

In `epics/include/EPICSystem.h`, add after existing includes:

```cpp
#include <cstdint>
```

### 4. Set Up Environment

**For Linux:**

```bash
cd /path/to/mpd_gem_view_ssp

# Edit setup_env.sh to use LD_LIBRARY_PATH
# Uncomment the Linux line, comment the macOS line
nano setup_env.sh

# Source ROOT
source ~/software/root/bin/thisroot.sh  # or wherever ROOT is installed

# Source project environment
source setup_env.sh
```

**For macOS:**

```bash
cd /path/to/mpd_gem_view_ssp
source setup_env.sh
```

### 5. Build with qmake

```bash
# Generate Makefiles
qmake mpd_gem_view.pro

# Compile (use -j4 for parallel compilation with 4 cores)
make -j4
```

### 6. Verify Build

```bash
# Check if executables were created
ls -lh bin/

# Should see:
# - data_viewer (GUI application)
# - replay (command-line analyzer)

# Check if libraries were created
ls -lh decoder/lib/ gem/lib/ epics/lib/

# Should see .so files (Linux) or .dylib files (macOS)
```

---

## Running

### GUI Application (data_viewer)

```bash
cd /path/to/mpd_gem_view_ssp

# Set up environment
source ~/software/root/bin/thisroot.sh
source setup_env.sh

# Run the GUI
./bin/data_viewer
```

### Command-Line Replay Tool

```bash
./bin/replay --help

# Example usage:
./bin/replay -c 0 -t 0 -z 1 -n -1 \
  --tracking off \
  --pedestal database/gem_ped_55.dat \
  --common_mode database/CommonModeRange_55.txt \
  /path/to/data.evio.0
```

### Loading Data Files

**Option 1: Via GUI**
1. Launch `data_viewer`
2. File → Open
3. Navigate to your EVIO file
4. Select and open

**Option 2: Copy to Project Directory**

```bash
# Copy your EVIO file to project root
cp /path/to/your/data.evio.0 /path/to/mpd_gem_view_ssp/

# Run viewer - file will be in current directory
./bin/data_viewer
```

**For WSL Users:**

Your Windows files are accessible at `/mnt/c/`, `/mnt/d/`, etc.

```bash
# Example: Copy from Windows Downloads
cp /mnt/c/Users/YourUsername/Downloads/data.evio.0 ./
```

---

## Troubleshooting

### Build Issues

#### Error: `Command 'qmake' not found`

**Cause:** Qt5 not installed or not in PATH  
**Solution:**

```bash
sudo apt install qt5-qmake qtbase5-dev
```

#### Error: `cannot find -ldecoder` / `cannot find -lgem`

**Cause:** Libraries didn't build or environment not set  
**Solution:**

```bash
# Rebuild from scratch
make clean
qmake mpd_gem_view.pro
make -j4

# Make sure environment is set
source setup_env.sh
```

#### Error: ROOT headers not found (`TH1I.h: No such file or directory`)

**Cause:** ROOT not sourced or Makefiles generated before ROOT was available  
**Solution:**

```bash
# Source ROOT first
source ~/software/root/bin/thisroot.sh
root-config --version  # Verify it works

# Clean and regenerate Makefiles
make clean
qmake mpd_gem_view.pro
make -j4
```

---

### Runtime Issues

#### Error: `cannot open shared object file: libdecoder.so.1`

**Cause:** Library path not set  
**Solution:**

```bash
# For Linux
export LD_LIBRARY_PATH=${PWD}/decoder/lib:${PWD}/gem/lib:${PWD}/epics/lib:${LD_LIBRARY_PATH}

# Or source the setup script
source setup_env.sh
```

#### Error: `Can't open display`

**Cause:** X server not running or DISPLAY not set (WSL/Linux)  
**Solution:**

**For Windows 11 (WSLg):**

```bash
export DISPLAY=:0
xclock  # Test if GUI works
```

**For Windows 10 with VcXsrv:**
1. Launch XLaunch (check "Disable access control")
2. In WSL:

```bash
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
```

**For Linux:**

Verify X server is running and DISPLAY is set:

```bash
echo $DISPLAY  # Should show something like :0 or :1
```

#### Application Crashes with `std::out_of_range` or `stof` error

**Cause:** Config files have Windows line endings (CRLF)  
**Solution:**

```bash
# Convert all config files to Unix format
dos2unix config/*.conf*

# Try running again
./bin/data_viewer
```

#### Error: `failed loading config file`

**Cause:** Running from wrong directory or config file missing  
**Solution:**

```bash
# Must run from project root
cd /path/to/mpd_gem_view_ssp

# Verify config exists
ls -la config/gem.conf

# Run application
./bin/data_viewer
```

#### Qt Platform Plugin Error (`libqxcb.so` not found)

**Cause:** Missing Qt XCB platform plugin  
**Solution:**

```bash
sudo apt install libqt5gui5 libqt5widgets5 qt5-qmake \
                 libxcb-xinerama0 libxcb-icccm4 libxcb-image0 \
                 libxcb-keysyms1 libxkbcommon-x11-0
```

---

## Configuration Files

### Main Configuration: `config/gem.conf`

Contains detector setup parameters, APV mappings, and analysis settings.

### Tracking Configuration: `config/gem_tracking.conf`

Contains tracking algorithm parameters and detector geometry.

**Multiple versions available:**
- `gem_tracking.conf_backup` - Backup configuration
- `gem_tracking.conf_setup1_run_period2` - Setup 1 configuration
- `gem_tracking.conf_setup2_run_period*` - Setup 2 configurations

Copy the appropriate config for your setup:

```bash
cp config/gem_tracking.conf_setup1_run_period2 config/gem_tracking.conf
```

### Pedestal Files: `database/gem_ped_*.dat`

Pre-calculated pedestal (baseline noise) data for each detector configuration. 
Select the appropriate file based on your run number.

---

## Testing

### Verify Build

```bash
# All libraries should exist
ls -lh decoder/lib/libdecoder.so*
ls -lh gem/lib/libgem.so*
ls -lh epics/lib/libepics.so*

# Executables should exist
ls -lh bin/data_viewer
ls -lh bin/replay
```

### Test GUI

```bash
# Should launch without errors
./bin/data_viewer

# Should display Qt window with menu bar and canvas areas
```

### Test with Sample Data

If sample data is available:

```bash
./bin/data_viewer
# File → Open → select sample .evio file
# Should display detector data
```

---


## Quick Start Summary

```bash
# 1. Install dependencies
sudo apt install qt5-qmake qtbase5-dev root-system libroot-dev

sudo apt install dos2unix # Only if on Linux/WSL

# 2. Clone repository
cd ~/workspace
git clone <repo-url>
cd mpd_gem_view_ssp

# 3. Fix compatibility if on linux or wsl
dos2unix setup_env.sh config/*.conf*

# 4. Build
source ~/software/root/bin/thisroot.sh
source setup_env.sh
qmake mpd_gem_view.pro
make -j4

# 5. Run
./bin/data_viewer
```


TLDR a fork thats complete overhaul of a victus controll that works for victus 16-R and doesent break the keyboard lighting  (which original did, i culdnt even turn on the backlight at all due to broken file)   the keyboard BG can be controlled via KDE build in options in the same place as the screen brightness. the fan profiles are much improved and allowing for user changes to them. The fan speed is based on the cpu temps and it auto updates. there is an option to make your own progfile as well. Also it allows for 0rpm mode. Also allows manual fan speed input.  


Values in betwen temp points are interpolated. 

The fan speed 2 needs a moment to catch up after fan 1 changes (not sure why but thats not imporatnt isue i guess)

# Victus-Control - Complete Documentation

A comprehensive fan control solution for HP Victus/Omen laptops on Linux, featuring dynamic temperature monitoring, customizable fan profiles, and low-resource background operation.

## Table of Contents
- [Features](#features)
- [System Requirements](#system-requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [User Guide](#user-guide)
- [Configuration](#configuration)
- [Architecture](#architecture)
- [Troubleshooting](#troubleshooting)
- [Performance & Optimization](#performance--optimization)

---

## Features

### ✅ Core Capabilities
- **Better Auto Mode**: Intelligent fan curve that adapts to CPU temperature and load
- **Manual Control**: Direct RPM input (0 or 1500-6100 RPM per fan) 
- **Custom Profiles**: Save up to 10 temperature/RPM points for different scenarios
- **Real-Time Temperatures**: CPU package temp + individual core temps + NVMe temps
- **Persistent Settings**: Automatically save and load user preferences
- **Low Resource Usage**: Optimized for background operation (minimal CPU/memory impact)
- **Configurable Update Interval**: Adjust monitoring frequency from 1-60 seconds

### ✅ Advanced Features
- **Request Queue System**: Limits backend communication to 3 concurrent requests
- **Background Service**: Runs 24/7 to maintain fan settings
- **Hardware Watchdog**: Reapplies settings every 90 seconds to counter firmware quirks
- **Module Auto-Loading**: DKMS module auto-builds for kernel updates
- **Permission Management**: Automatic user/group setup via sudoers

---

## System Requirements

### Minimum
- 64-bit Linux with systemd (Arch-based distros tested)
- HP Victus or Omen laptop with WMI fan control support
- ~50MB disk space

### Software Dependencies
- `meson` - Build system
- `ninja` - Build tool
- `gtk4` - UI framework
- `git` - Source control
- `dkms` - Kernel module management
- `lm_sensors` - **CRITICAL** for temperature reading // build in to CachyOS
- `linux-headers` - For building kernel modules

### First-Time Setup
```bash
# Install dependencies (automatic with install.sh)
sudo pacman -S meson ninja gtk4 git dkms linux-headers lm_sensors

# Configure lm-sensors for temperature monitoring
sudo sensors-detect  # Follow prompts, answer yes to save settings
sudo systemctl restart victus-backend.service
```

---

## Installation

### Standard Installation

```bash
git clone https://github.com/pieotr/victus16-control-FanSpeed-in-Arch.git
cd victus-control
sudo ./install.sh
```

**What the installer does:**
1. Installs all system dependencies
2. Creates `victus` user group
3. Registers hp-wmi DKMS module
4. Builds and installs frontend/backend
5. Sets up systemd services and sudoers rules
6. Creates `/run/victus-control` socket directory

**Post-installation:**
```bash
# Log out and back in for group membership to take effect
groups  # Should show 'victus' in the list
```

### Uninstallation
```bash
sudo systemctl disable --now victus-backend.service
sudo dkms remove hp-wmi-fan-and-backlight-control/0.0.2 --all
sudo userdel victus
sudo rm -rf /etc/victus-control /run/victus-control ~/.config/victus-control
```

---

## Quick Start

### Launch the App
```bash
victus-control
# App minimizes to system tray; click to restore
```

### Verify Backend Status
```bash
systemctl status victus-backend.service
journalctl -u victus-backend -f  # Live logs
```

### Check Temperature Reading
```bash
sensors  # Should show CPU Package, Core, and NVMe temps
```

---

## Important: Automatic Operation Without Frontend

**The backend service (`victus-backend`) automatically enables Better Auto mode on startup** and maintains fan control 24/7, even if the frontend GUI is not running.

### What happens when the service starts:
1. Backend loads and automatically enters **Better Auto mode**
2. Fans are controlled based on CPU temperature following the profile in `fan_profile_config.hpp`
3. Settings persist and reapply every 90 seconds (hardware watchdog)
4. Frontend (GUI) is **optional** — it's only for manual mode selection and viewing temperatures

### Running without the frontend:
You don't need to launch the GUI. The service works completely standalone:
```bash
# Just check that the service is running
systemctl status victus-backend.service

# View live logs to see RPM adjustments
journalctl -u victus-backend -f

# Service automatically enabled on boot
systemctl is-enabled victus-backend.service  # Should show "enabled"
```

The frontend GUI only provides a user-friendly interface for selecting modes and viewing status. For pure background operation, the backend does everything automatically.

---

## User Guide

### Main UI Sections

#### 1. **Mode Selector** (Top dropdown)
| Mode | Purpose |
|------|---------|
| **AUTO** | Stock firmware default (not recommended) |
| **Better Auto** | Intelligent curve (recommended for daily use) |
| **MANUAL** | Direct RPM input for precise control (0 or 1500-6100 RPM) |
| **PROFILE** | Custom temperature/RPM curves |
| **MAX** | Maximum speed (max cooling, max noise) |

#### 2. **System Temperatures** (Real-time display)
Shows:
- **CPU**: Package temperature in orange (primary thermal target)
- **NVMe1/NVMe2**: SSD temperatures for storage stability

Updates every 2-60 seconds (configurable).

#### 3. **CPU Cores** (Bottom status)
Displays all individual CPU core temperatures:
```
CPU Cores: C0:42°C, C1:40°C, C2:41°C, C3:39°C, ...
```

#### 4. **Fan Speed Status**
Shows real-time RPM for both fans:
```
Fan 1 Speed: 3200 RPM
Fan 2 Speed: 3100 RPM
```

### Manual Mode

1. Select **MANUAL** from mode dropdown
2. Enter RPM value (0 for stop, 1500-6100 for running)
3. Click **Apply RPM** — wait a little for both fans to be set
4. Fan maintains setting even after system sleep/resume

**Note on Minimum RPM**: The minimum non-zero RPM is **1500** due to fan motor specifications. These fans require a minimum voltage to overcome static friction and start spinning. Below 1500 RPM, the fans may not respond or may stall. This limit ensures reliable fan operation. Use **0 RPM mode** if you want to stop the fans completely.

### Profile Mode

#### Creating a Custom Profile

1. Select **PROFILE** mode
2. Set temperature (30-100°C) and desired RPM
3. Click **Add Point**
4. Repeat for up to 10 points
5. Click **Apply Profile**

**Example Gaming Profile:**
```
30°C  → 2600 RPM (idle)
50°C  → 3500 RPM (light work)
70°C  → 4500 RPM (heavy load)
85°C  → 5800 RPM (thermal limit)
```

**Example Silent Profile:**
```
30°C  → 1000 RPM
60°C  → 2800 RPM
80°C  → 3500 RPM
```

#### Removing Points
Click **Remove Last** to delete the most recently added point.

#### Applying Profiles
Once configured, click **Apply Profile**. The profile persists until changed.

### Settings

#### Update Interval
- **Range**: 1-60 seconds
- **Default**: 2 seconds
- **Effect**: How often temps/RPM are refreshed

**Optimization Tips:**
- 1-2 sec: Most responsive (default)
- 5-10 sec: Good balance
- 30+ sec: Minimal overhead

**Auto-saved to** `~/.config/victus-control/settings.conf`

---

## Configuration

### Settings File Location
```
~/.config/victus-control/settings.conf
```

### Settings File Format
```ini
# Victus Control Settings
# Update interval in seconds (1-60, default: 2)
update_interval_sec=2
# Start minimized (true/false, default: false)
start_minimized=false
```

### Edit Manually
```bash
nano ~/.config/victus-control/settings.conf
# Make changes
# App reloads on restart
```

### Profile Configuration (Advanced)  -- you propably want to do it when you find your prefered curve
Edit hardcoded "Better Auto" profiles:
```bash
nano backend/src/fan_profile_config.hpp
# Modify fan curves
meson compile -C build
sudo meson install -C build
```

---

## Architecture

### System Overview
```

dont look at it to much, its ai generated but looks nice so i added it 

┌─────────────────────────────────────────────────────────┐
│ User Interface (GTK4 Frontend)                          │
│  - Temperature display                                  │
│  - Mode selector                                        │
│  - Manual/Profile controls                              │
│  - Settings (update interval, minimize on start)        │
└──────────────────────┬──────────────────────────────────┘
                       │
                   Unix Socket
                  (/run/victus-control/
                   victus-backend.sock)
                       │
┌──────────────────────┼──────────────────────────────────┐
│ Backend Service (runs as root)                          │
│  - Fan control logic                                    │
│  - Temperature reading (lm-sensors)                     │
│  - Request queue (3 concurrent max)                     │
│  - Hardware watchdog (90s reapply)                      │
│  - Profile interpolation                                │
└──────────────────────┬──────────────────────────────────┘
                       │
         ┌─────────────┼─────────────┐
         │             │             │
    /sys/kernel/   lm-sensors    hp-wmi.ko
    debug/        (sensors cmd)   (DKMS module)
    (thermal)
         │             │            │
    ┌────┴─────┬───────┴─┬──────────┴──────┐
    │ CPU Temp │ NVMe   │ Fan PWM Control  │
    │ Zones    │ Sensors│ (pwm1_enable)    │
    └──────────┴────────┴─────────────────┘
```

### Components

#### Frontend (`frontend/src/`)
- **fan.cpp/hpp**: Main UI logic, settings loading/saving
- **socket.cpp/hpp**: Unix socket client
- **main.cpp**: GTK window setup
- **settings.hpp**: Configuration management

#### Backend (`backend/src/`)
- **fan.cpp/hpp**: Fan control, temperature reading
- **main.cpp**: Socket server, command dispatcher
- **fan_profile_config.hpp**: Built-in temperature curves
- **set-fan-speed.sh/set-fan-mode.sh**: Hardware interface

#### System Integration
- **victus-backend.service**: Runs backend 24/7
- **victus-healthcheck.service**: Module initialization
- **victus-control.rules**: udev permissions
- **victus-control.tmpfiles**: `/run` directory setup

### Data Flow

#### Temperature Reading (every 2-60 sec)
```
Frontend  → "GET_ALL_TEMPS"  → Backend
         ← "PKG:48|CORES:40,41,39,42...|NVME:36"  ← 
Frontend parses and displays on UI
```

#### Fan Mode Change
```
User selects mode  →  Frontend
            ↓
    "SET_FAN_MODE MANUAL"  →  Backend
            ↓
    Backend writes to /sys/.../pwm1_enable
            ↓
    Backend returns "OK"  →  Frontend
            ↓
    Frontend updates UI
```

#### Request Queue
```
Frontend sends command 1  ┐
Frontend sends command 2  ├→ Queue (max 3 concurrent)
Frontend sends command 3  │
Frontend sends command 4  ┘  Waits until #1 completes
```


its there to prevent lag in the app when to many comands are send at once by the user 
---

### Optimization Strategies

#### For Laptops (default)
```ini
update_interval_sec=2   # Good balance
```
**I**: Minimal impact on battery life, responsive updates

#### For less cpu useage - slower responce
```ini
update_interval_sec=30  # Very low CPU
```
**I**: less CPU usage, updates every 30 seconds

#### For idk Gaming or stuff ?  not advortised but its there 
```ini
update_interval_sec=1   # Most responsive
```
**I**: immediate fan response to load spikes

### Reducing System Impact

1. **Increase update interval** if monitoring overhead is concerns
2. **Disable UI** after launch (runs in background)
3. **Use systemd.service only** if GUI not needed:
   ```bash
   systemctl mask victus-control.desktop
   # Disable app launch, backend still runs
   ```

---

## Troubleshooting

### Common Issues you shuld never encounter... but in case -

#### 1. **"CPU: N/A | NVMe: N/A"** (no temperatures)
**Cause**: lm-sensors not initialized
**Fix**:
```bash
sudo sensors-detect  # Run detection
sudo systemctl restart victus-backend.service
sensors  # Verify command works
```

#### 2. **Fans ignore commands**
**Cause**: DKMS module not loaded
**Fix**:
```bash
sudo modprobe hp_wmi
dkms status | grep hp-wmi-fan
# Should show "installed"
```

#### 3. **"Permission denied" errors**
**Cause**: Not in `victus` group
**Fix**:
```bash
groups  # Check membership
sudo usermod -aG victus $USER
# Log out and back in
```

#### 4. **Socket missing error**
**Cause**: `/run/victus-control` directory removed
**Fix**:
```bash
sudo systemd-tmpfiles --create
sudo systemctl restart victus-backend.service
```

#### 5. **Settings not saving**
**Cause**: Permission issue or corrupted file
**Fix**:
```bash
rm ~/.config/victus-control/settings.conf
# App recreates on next launch
```

#### 6. **High CPU usage**
**Cause**: Update interval too short or request queue backlog
**Fix**:
```ini
# Increase interval in settings UI
update_interval_sec=5  # Default is 2 sec
```

### Debug Logs

```bash
# Backend logs
journalctl -u victus-backend -f

# Last 50 lines
journalctl -u victus-backend -n 50

# Specific error
journalctl -u victus-backend | grep -i "error"

# Since boot
journalctl -u victus-backend -b
```

### Manual Hardware Testing

```bash
# Read current fan mode (2=AUTO, 1=MANUAL, 0=MAX)
cat /sys/devices/platform/hp-wmi/hwmon/hwmon*/pwm1_enable

# Set to MANUAL
echo 1 | sudo tee /sys/devices/platform/hp-wmi/hwmon/hwmon*/pwm1_enable

# Set fan speed (0-255 PWM value, ~3000RPM = 128)
echo 128 | sudo tee /sys/devices/platform/hp-wmi/hwmon/hwmon*/pwm1
```

---

## Advanced Usage

### Autostart on Boot

Already enabled by default:
```bash
systemctl is-enabled victus-backend.service  # Should print: enabled
```

If disabled:
```bash
sudo systemctl enable victus-backend.service
```

### CLI Backend Testing

```bash
# Connect to socket and send commands
echo "GET_FAN_MODE" | nc -U /run/victus-control/victus-backend.sock

# Expected responses:
# - "AUTO" / "MANUAL" / "BETTER_AUTO" / "MAX" / "PROFILE"
# - "ERROR: ..." (on failure)
```

### Profile Advanced Tips

**Non-linear curves** (steep increase at high temps):
```
30°C → 1200 RPM
50°C → 3000 RPM
60°C → 3500 RPM
70°C → 4500 RPM
80°C → 5500 RPM
90°C → 5800 RPM
```

**Gaming profile** (aggressive cooling):
```
40°C → 3200 RPM
55°C → 4000 RPM
70°C → 5000 RPM
85°C → 5800 RPM
```

**Silent profile** (minimal noise):
```
40°C → 1500 RPM
60°C → 2500 RPM
80°C → 3500 RPM
```

---

## Modifying Code and Rebuilding

If you want to customize the code (fan profiles, RPM ranges, colors, etc.), here's how to rebuild and install:

### Prerequisites
Make sure you have the build dependencies installed:
```bash
sudo pacman -S meson ninja gtk4 git dkms linux-headers lm_sensors
```

### Build Steps

1. **Navigate to the project directory:**
   ```bash
   cd ~/Downloads/Victus-FanControl/victus-control
   ```

2. **Clean previous builds (optional but recommended):**
   ```bash
   rm -rf build
   ```

3. **Configure and build:**
   ```bash
   meson setup build --prefix=/usr
   meson compile -C build
   ```

4. **Install the updated binaries:**
   ```bash
   sudo meson install -C build
   ```

5. **Restart the backend service:**
   ```bash
   sudo systemctl restart victus-backend.service
   ```

### Common Customizations

**To modify fan profiles:**
- Edit `backend/src/fan_profile_config.hpp`
- Profiles define temperature points and corresponding RPM targets
- Follow the existing format and rebuild using the steps above

**To change minimum RPM:**
- Frontend: `frontend/src/fan.cpp` — `const int MIN_RPM_NONZERO = 1500;`
- Backend: `backend/src/fan.cpp` — `static constexpr int kBetterAutoMinRpm = 1500;` and validation `if (rpm != 0 && rpm < 1500)`
- Update this value consistently in all three places
- Rebuild after changes

**To modify temperature ranges:**
- Edit profile points in `backend/src/fan_profile_config.hpp`
- Temperature range: 0–100°C (validated in backend)
- Rebuild and reinstall

### Troubleshooting Rebuild

**Compilation errors:**
- Ensure all dependencies are installed: `pacman -S meson ninja gtk4 dkms linux-headers`
- Check file permissions: `ls -la frontend/src/`
- Look for missing includes or typos

**Build cache issues:**
- Remove build directory and reconfigure: `rm -rf build && meson setup build --prefix=/usr`

**Permission denied during install:**
- Use `sudo meson install` if non-root installation fails
- Verify sudoers rules: `sudo cat /etc/sudoers.d/victus-control`

---

**Last Updated**: January 31, 2026
**Version**: 1.1.3 (Min RPM Documentation + Rebuild Guide)

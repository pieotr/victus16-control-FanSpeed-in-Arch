#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Check for root privileges ---
if [ "$EUID" -ne 0 ]; then
  echo "This script requires root privileges. Re-running with sudo..."
  sudo "$0" "$@"
  exit
fi

echo "--- Starting Victus Control Installation ---"

# --- 1. Install Dependencies ---
echo "--> Installing required packages..."

packages=(meson ninja gtk4 git dkms lm_sensors)
declare -A header_packages=()

# Detect kernel headers package
for module_dir in /usr/lib/modules/*; do
    [[ -d "${module_dir}" ]] || continue

    kernel_release=$(basename "${module_dir}")
    pkgbase_path="${module_dir}/pkgbase"

    if [[ -r "${pkgbase_path}" ]]; then
        kernel_pkg=$(<"${pkgbase_path}")
        kernel_pkg=${kernel_pkg//[[:space:]]/}
        header_pkg="${kernel_pkg}-headers"

        if pacman -Si "${header_pkg}" > /dev/null 2>&1; then
            header_packages["${header_pkg}"]=1
            echo "Detected kernel '${kernel_pkg}' (${kernel_release}); queued '${header_pkg}'."
            continue
        fi
    fi

    header_packages[linux-headers]=1
done

[[ ${#header_packages[@]} -gt 0 ]] || header_packages[linux-headers]=1

for header_pkg in "${!header_packages[@]}"; do
    packages+=("${header_pkg}")
done

pacman -S --needed --noconfirm "${packages[@]}"

# --- 2. Create Users and Groups ---
echo "--> Creating secure users and groups..."

# Ensure the victus group exists
if ! getent group victus > /dev/null; then
    groupadd --system victus
    echo "Group 'victus' created."
else
    echo "Group 'victus' already exists."
fi

# Create the victus-backend group
if ! getent group victus-backend > /dev/null; then
    groupadd --system victus-backend
    echo "Group 'victus-backend' created."
else
    echo "Group 'victus-backend' already exists."
fi

# Create the victus-backend user
if ! id -u victus-backend > /dev/null 2>&1; then
    useradd --system -g victus-backend -s /usr/bin/nologin victus-backend
    echo "User 'victus-backend' created."
else
    echo "User 'victus-backend' already exists."
fi

# Add victus-backend to the victus group
if ! groups victus-backend | grep -q '\bvictus\b'; then
    usermod -aG victus victus-backend
    echo "User 'victus-backend' added to the 'victus' group."
fi

# Add the original user to the 'victus' group
if [ -n "$SUDO_USER" ]; then
    if ! groups "$SUDO_USER" | grep -q '\bvictus\b'; then
        usermod -aG victus "$SUDO_USER"
        echo "User '$SUDO_USER' added to the 'victus' group."
    fi
else
    echo "Warning: Could not determine the original user. Add your user to the 'victus' group manually with: sudo usermod -aG victus \$USER"
fi

# --- 2.5. Configure Sudoers and Scripts ---
echo "--> Installing helper script and configuring sudoers..."
install -m 0755 backend/src/set-fan-speed.sh /usr/bin/set-fan-speed.sh
install -m 0755 backend/src/set-fan-mode.sh /usr/bin/set-fan-mode.sh
rm -f /etc/sudoers.d/victus-fan-sudoers
install -m 0440 victus-control-sudoers /etc/sudoers.d/victus-control-sudoers
echo "Helper script and sudoers file installed."

# --- 3. Install Patched HP-WMI Kernel Module ---
echo "--> Installing patched hp-wmi kernel module..."
wmi_repo="wmi-project/hp-wmi-fan-and-backlight-control"
mkdir -p "$(dirname "$wmi_repo")"

if [ -d "${wmi_repo}/.git" ]; then
    echo "Kernel module source already exists. Updating..."
    git -C "${wmi_repo}" fetch origin master && git -C "${wmi_repo}" reset --hard origin/master || \
        echo "Warning: Failed to update hp-wmi repository."
else
    git clone https://github.com/Batuhan4/hp-wmi-fan-and-backlight-control.git "${wmi_repo}"
fi

pushd "${wmi_repo}" >/dev/null
module_name="hp-wmi-fan-and-backlight-control"
module_version="0.0.2"

# Remove existing DKMS registration if present
dkms status -m "${module_name}" -v "${module_version}" >/dev/null 2>&1 && \
    dkms remove "${module_name}/${module_version}" --all 2>/dev/null || true

# Register and install DKMS module
dkms add .

# Install for any missing kernels
for module_dir in /usr/lib/modules/*; do
    [[ -d "${module_dir}" ]] || continue
    kernel_release=$(basename "${module_dir}")
    
    if ! dkms status -m "${module_name}" -v "${module_version}" -k "${kernel_release}" 2>/dev/null | grep -q "installed"; then
        dkms install "${module_name}/${module_version}" -k "${kernel_release}"
    fi
done

# Reload module
lsmod | grep -q hp_wmi && rmmod hp_wmi
modprobe hp_wmi
popd >/dev/null
echo "Kernel module installed and loaded."

# --- 4. Build and Install victus-control ---
echo "--> Building and installing the application..."
meson setup build --prefix=/usr
ninja -C build
ninja -C build install
echo "Application built and installed."

# --- 5. Configure and Start Backend Service ---
echo "--> Configuring and starting backend service..."
systemd-tmpfiles --create 2>/dev/null || echo "Warning: Failed to create tmpfiles."
systemctl daemon-reload
udevadm control --reload-rules && udevadm trigger 2>/dev/null || true

# Enable health check service
systemctl list-unit-files | grep -q '^victus-healthcheck.service' && \
    systemctl enable --now victus-healthcheck.service 2>/dev/null || true

# Enable and start backend service
if systemctl enable --now victus-backend.service; then
    echo "Backend service enabled and started."
else
    echo "Error: Failed to enable/start victus-backend service"
    exit 1
fi

echo ""
echo "--- Installation Complete! ---"
echo ""
echo "IMPORTANT: For the group changes to take full effect, please log out and log back in."
echo "After logging back in, you can launch the application from your desktop menu or by running 'victus-control' in the terminal."
echo ""

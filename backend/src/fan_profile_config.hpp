#ifndef FAN_PROFILE_CONFIG_HPP
#define FAN_PROFILE_CONFIG_HPP

#include <array>

// BETTER AUTO Fan Profile Configuration
// Each profile point: {temperature_celsius, rpm}
// Temperatures should be in ascending order

// Fan 1 Profile (typically CPU fan)
// Adjust these values before compilation to customize Better Auto behavior
static constexpr std::array<std::pair<int, int>, 8> FAN1_BETTER_AUTO_PROFILE = {{
    {50, 1500},    // 50°C → 1500 RPM
    {55, 1800},    // 55°C → 1800 RPM
    {58, 2000},    // 58°C → 2000 RPM
    {60, 2500},    // 60°C → 2500 RPM
    {70, 4500},    // 70°C → 4500 RPM
    {75, 5200},    // 75°C → 5200 RPM
    {80, 5600},    // 80°C → 5600 RPM
    {90, 5800}     // 90°C → 5800 RPM (max)
}};

// Fan 2 Profile (typically GPU fan)
// Adjust these values before compilation to customize Better Auto behavior
static constexpr std::array<std::pair<int, int>, 8> FAN2_BETTER_AUTO_PROFILE = {{
    {50, 1500},    // 50°C → 1500 RPM
    {55, 1800},    // 55°C → 1800 RPM
    {58, 2000},    // 58°C → 2000 RPM
    {60, 2500},    // 60°C → 2500 RPM
    {70, 4600},    // 70°C → 4600 RPM
    {75, 5300},    // 75°C → 5300 RPM
    {80, 5700},    // 80°C → 5700 RPM
    {90, 6100}     // 90°C → 6100 RPM (max)
}};

#endif // FAN_PROFILE_CONFIG_HPP

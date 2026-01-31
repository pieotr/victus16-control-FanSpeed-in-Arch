#ifndef FAN_PROFILE_CONFIG_HPP
#define FAN_PROFILE_CONFIG_HPP

#include <array>

// BETTER AUTO Fan Profile Configuration
// Each profile point: {temperature_celsius, rpm}
// Temperatures should be in ascending order

// Fan 1 Profile (typically CPU fan)
// Adjust these values before compilation to customize Better Auto behavior
static constexpr std::array<std::pair<int, int>, 8> FAN1_BETTER_AUTO_PROFILE = {{
    {30, 2600},    // 30°C → 2600 RPM
    {40, 2800},    // 40°C → 2800 RPM
    {50, 3200},    // 50°C → 3200 RPM
    {60, 3800},    // 60°C → 3800 RPM
    {70, 4500},    // 70°C → 4500 RPM
    {80, 5200},    // 80°C → 5200 RPM
    {90, 5600},    // 90°C → 5600 RPM
    {100, 5800}    // 100°C → 5800 RPM (max)
}};

// Fan 2 Profile (typically GPU fan)
// Adjust these values before compilation to customize Better Auto behavior
static constexpr std::array<std::pair<int, int>, 8> FAN2_BETTER_AUTO_PROFILE = {{
    {30, 2600},    // 30°C → 2600 RPM
    {40, 2800},    // 40°C → 2800 RPM
    {50, 3300},    // 50°C → 3300 RPM
    {60, 3900},    // 60°C → 3900 RPM
    {70, 4600},    // 70°C → 4600 RPM
    {80, 5300},    // 80°C → 5300 RPM
    {90, 5700},    // 90°C → 5700 RPM
    {100, 6100}    // 100°C → 6100 RPM (max)
}};

#endif // FAN_PROFILE_CONFIG_HPP

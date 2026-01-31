#include <string>

void fan_mode_trigger(const std::string mode);
std::string set_fan_mode(const std::string &value);
std::string get_fan_mode();
std::string get_cpu_temp();
std::string get_all_temps();

std::string get_fan_speed(const std::string &fan_num);
std::string set_fan_speed(const std::string &fan_num, const std::string &speed, bool trigger_mode = true, bool update_cache = true);
std::string set_fan_profile(const std::string &profile_data);
std::string ensure_better_auto_mode();

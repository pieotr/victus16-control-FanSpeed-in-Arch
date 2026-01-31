#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

class AppSettings {
public:
    int update_interval_sec;  // Temperature and fan speed update interval (default: 2)
    bool start_minimized;     // Start app minimized (default: false)
    
    AppSettings() : update_interval_sec(2), start_minimized(false) {}
    
    static std::string get_config_path() {
        const char* home_env = std::getenv("HOME");
        std::string home = home_env ? home_env : "/root";
        return home + "/.config/victus-control/settings.conf";
    }
    
    bool load() {
        try {
            std::string config_path = get_config_path();
            if (!std::filesystem::exists(config_path)) {
                return false;  // File doesn't exist, use defaults
            }
            
            std::ifstream file(config_path);
            std::string line;
            
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;
                
                size_t eq_pos = line.find('=');
                if (eq_pos == std::string::npos) continue;
                
                std::string key = line.substr(0, eq_pos);
                std::string value = line.substr(eq_pos + 1);
                
                if (key == "update_interval_sec") {
                    try {
                        int val = std::stoi(value);
                        if (val >= 1 && val <= 60) {
                            update_interval_sec = val;
                        }
                    } catch (...) {}
                } else if (key == "start_minimized") {
                    start_minimized = (value == "true" || value == "1");
                }
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool save() {
        try {
            std::string config_path = get_config_path();
            std::string config_dir = std::filesystem::path(config_path).parent_path();
            
            // Create directory if it doesn't exist
            std::filesystem::create_directories(config_dir);
            
            std::ofstream file(config_path);
            file << "# Victus Control Settings\n";
            file << "# Update interval in seconds (1-60, default: 2)\n";
            file << "update_interval_sec=" << update_interval_sec << "\n";
            file << "# Start minimized (true/false, default: false)\n";
            file << "start_minimized=" << (start_minimized ? "true" : "false") << "\n";
            
            return true;
        } catch (...) {
            return false;
        }
    }
};

#endif // SETTINGS_HPP

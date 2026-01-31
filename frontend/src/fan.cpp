#include "fan.hpp"
#include "socket.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>

// Constants for manual fan control
const int MIN_RPM_NONZERO = 1500;  // Minimum non-zero RPM
const int FAN1_MAX_RPM = 5800;
const int FAN2_MAX_RPM = 6100;
const int RPM_STEPS = 8;
const int MIN_TEMP = 30;
const int MAX_TEMP = 100;
const int MIN_RPM_INPUT = 0;    // Allow 0 RPM mode
const int MAX_RPM_INPUT = 6100; // Use fan2 max as overall max

VictusFanControl::VictusFanControl(std::shared_ptr<VictusSocketClient> client) : socket_client(client), temp_timer_id(0), fan_timer_id(0)
{
    // Load settings
    settings.load();
    
    fan_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(fan_page, 20);
    gtk_widget_set_margin_bottom(fan_page, 20);
    gtk_widget_set_margin_start(fan_page, 20);
    gtk_widget_set_margin_end(fan_page, 20);

    // --- Mode Selector ---
    mode_selector = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_selector), "AUTO", "AUTO");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_selector), "BETTER_AUTO", "Better Auto");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_selector), "MANUAL", "MANUAL");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_selector), "MAX", "MAX");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_selector), "PROFILE", "PROFILE");
    g_signal_connect(mode_selector, "changed", G_CALLBACK(on_mode_changed), this);
    gtk_box_append(GTK_BOX(fan_page), mode_selector);

    // --- Manual Mode Section ---
    GtkWidget *manual_label = gtk_label_new("Manual Mode - Set RPM");
    gtk_widget_set_halign(manual_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(manual_label, 15);
    gtk_box_append(GTK_BOX(fan_page), manual_label);

    manual_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *rpm_label = gtk_label_new("RPM (0 or 1500-6100):");
    gtk_box_append(GTK_BOX(manual_box), rpm_label);
    
    rpm_input = gtk_spin_button_new_with_range(MIN_RPM_INPUT, MAX_RPM_INPUT, 50);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(rpm_input), 3000);
    gtk_box_append(GTK_BOX(manual_box), rpm_input);

    apply_rpm_button = gtk_button_new_with_label("Apply RPM");
    g_signal_connect(apply_rpm_button, "clicked", G_CALLBACK(on_apply_rpm_clicked), this);
    gtk_box_append(GTK_BOX(manual_box), apply_rpm_button);

    gtk_box_append(GTK_BOX(fan_page), manual_box);
    gtk_widget_set_sensitive(manual_box, FALSE);

    // --- CPU/NVMe Temperature Display Section ---
    GtkWidget *temp_display_label = gtk_label_new("System Temperatures");
    gtk_widget_set_halign(temp_display_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(temp_display_label, 15);
    gtk_box_append(GTK_BOX(fan_page), temp_display_label);

    all_temps_label = gtk_label_new("CPU: N/A | NVMe: N/A");
    gtk_widget_set_halign(all_temps_label, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(all_temps_label), TRUE);
    gtk_box_append(GTK_BOX(fan_page), all_temps_label);

    // --- Profile Section ---
    GtkWidget *profile_label = gtk_label_new("Create Temperature/Speed Profile (Max 10 Points)");
    gtk_widget_set_halign(profile_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(profile_label, 20);
    gtk_box_append(GTK_BOX(fan_page), profile_label);

    profile_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(fan_page), profile_box);
    gtk_widget_set_sensitive(profile_box, TRUE);  // Enable by default, disable for non-PROFILE modes

    // Profile points list with scrollable area
    profile_list_scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(profile_list_scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(profile_list_scrolled, -1, 150);

    profile_points_list = gtk_label_new("No profile points (0/10)");
    gtk_widget_set_halign(profile_points_list, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(profile_points_list), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(profile_list_scrolled), profile_points_list);
    gtk_box_append(GTK_BOX(profile_box), profile_list_scrolled);

    point_count_label = gtk_label_new("Points: 0/10");
    gtk_widget_set_halign(point_count_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(profile_box), point_count_label);

    // Input controls
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *temp_label = gtk_label_new("Temp (°C):");
    gtk_box_append(GTK_BOX(input_box), temp_label);
    
    temp_input = gtk_spin_button_new_with_range(MIN_TEMP, MAX_TEMP, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(temp_input), 50);
    gtk_box_append(GTK_BOX(input_box), temp_input);

    GtkWidget *speed_label = gtk_label_new("RPM:");
    gtk_box_append(GTK_BOX(input_box), speed_label);
    
    speed_input = gtk_spin_button_new_with_range(MIN_RPM_INPUT, MAX_RPM_INPUT, 50);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(speed_input), 3000);
    gtk_box_append(GTK_BOX(input_box), speed_input);

    add_point_button = gtk_button_new_with_label("Add Point");
    g_signal_connect(add_point_button, "clicked", G_CALLBACK(on_add_point_clicked), this);
    gtk_box_append(GTK_BOX(input_box), add_point_button);

    remove_point_button = gtk_button_new_with_label("Remove Last");
    g_signal_connect(remove_point_button, "clicked", G_CALLBACK(on_remove_point_clicked), this);
    gtk_box_append(GTK_BOX(input_box), remove_point_button);

    gtk_box_append(GTK_BOX(profile_box), input_box);

    apply_profile_button = gtk_button_new_with_label("Apply Profile");
    g_signal_connect(apply_profile_button, "clicked", G_CALLBACK(on_apply_profile_clicked), this);
    gtk_box_append(GTK_BOX(profile_box), apply_profile_button);

    gtk_widget_set_sensitive(profile_box, FALSE);

    // --- Settings Section ---
    GtkWidget *settings_label = gtk_label_new("Settings");
    gtk_widget_set_halign(settings_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(settings_label, 20);
    gtk_box_append(GTK_BOX(fan_page), settings_label);

    settings_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *interval_label = gtk_label_new("Update Interval (sec, 1-60):");
    gtk_box_append(GTK_BOX(settings_box), interval_label);
    
    interval_spin = gtk_spin_button_new_with_range(1, 60, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(interval_spin), settings.update_interval_sec);
    g_signal_connect(interval_spin, "value-changed", G_CALLBACK(on_interval_changed_static), this);
    gtk_box_append(GTK_BOX(settings_box), interval_spin);
    
    gtk_box_append(GTK_BOX(fan_page), settings_box);

    // --- Status Labels ---
    state_label = gtk_label_new("Current State: N/A");
    gtk_widget_set_halign(state_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(state_label, 20);
    gtk_box_append(GTK_BOX(fan_page), state_label);

    cpu_temp_label = gtk_label_new("CPU Temperature: N/A °C");
    gtk_widget_set_halign(cpu_temp_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(fan_page), cpu_temp_label);

    fan1_speed_label = gtk_label_new("Fan 1 Speed: N/A RPM");
    gtk_widget_set_halign(fan1_speed_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(fan_page), fan1_speed_label);

    fan2_speed_label = gtk_label_new("Fan 2 Speed: N/A RPM");
    gtk_widget_set_halign(fan2_speed_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(fan_page), fan2_speed_label);

    // Initial UI state update
    update_ui_from_system_state();
    update_fan_speeds();
    update_all_temperatures();

    // Set up timers with configurable interval
    temp_timer_id = g_timeout_add_seconds(settings.update_interval_sec, [](gpointer data) -> gboolean {
        static_cast<VictusFanControl*>(data)->update_all_temperatures();
        return G_SOURCE_CONTINUE;
    }, this);
    
    fan_timer_id = g_timeout_add_seconds(settings.update_interval_sec, [](gpointer data) -> gboolean {
        static_cast<VictusFanControl*>(data)->update_fan_speeds();
        return G_SOURCE_CONTINUE;
    }, this);
}

GtkWidget* VictusFanControl::get_page()
{
    return fan_page;
}

VictusFanControl::~VictusFanControl()
{
    // Clean up timers
    if (temp_timer_id) g_source_remove(temp_timer_id);
    if (fan_timer_id) g_source_remove(fan_timer_id);
    settings.save();
}

void VictusFanControl::update_ui_from_system_state()
{
    auto response = socket_client->send_command_async(GET_FAN_MODE);
    std::string fan_mode = response.get();

    if (fan_mode.find("ERROR") != std::string::npos) {
        fan_mode = "AUTO"; // Default to AUTO on error
        std::cerr << "Failed to get fan mode, defaulting to AUTO." << std::endl;
    }

    gtk_label_set_text(GTK_LABEL(state_label), ("Current State: " + fan_mode).c_str());

    if (fan_mode == "MANUAL") {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(mode_selector), "MANUAL");
        gtk_widget_set_sensitive(manual_box, TRUE);
        gtk_widget_set_sensitive(profile_box, FALSE);
    } else if (fan_mode == "PROFILE") {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(mode_selector), "PROFILE");
        gtk_widget_set_sensitive(manual_box, FALSE);
        gtk_widget_set_sensitive(profile_box, TRUE);
    } else if (fan_mode == "BETTER_AUTO") {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(mode_selector), "BETTER_AUTO");
        gtk_widget_set_sensitive(manual_box, FALSE);
        gtk_widget_set_sensitive(profile_box, FALSE);
    } else if (fan_mode == "MAX") {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(mode_selector), "MAX");
        gtk_widget_set_sensitive(manual_box, FALSE);
        gtk_widget_set_sensitive(profile_box, FALSE);
    } else { // AUTO
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(mode_selector), "AUTO");
        gtk_widget_set_sensitive(manual_box, FALSE);
        gtk_widget_set_sensitive(profile_box, FALSE);
    }
}

void VictusFanControl::update_fan_speeds()
{
    auto response2 = socket_client->send_command_async(GET_FAN_SPEED, "1");
    std::string fan1_speed = response2.get();
    if (fan1_speed.find("ERROR") != std::string::npos) fan1_speed = "N/A";

    auto response3 = socket_client->send_command_async(GET_FAN_SPEED, "2");
    std::string fan2_speed = response3.get();
    if (fan2_speed.find("ERROR") != std::string::npos) fan2_speed = "N/A";

    gtk_label_set_text(GTK_LABEL(fan1_speed_label), ("Fan 1 Speed: " + fan1_speed + " RPM").c_str());
    gtk_label_set_text(GTK_LABEL(fan2_speed_label), ("Fan 2 Speed: " + fan2_speed + " RPM").c_str());
}

void VictusFanControl::set_fan_rpm(int rpm)
{
    rpm = validate_rpm(rpm);
    
    std::string fan1_rpm_str = std::to_string(rpm);
    std::string fan2_rpm_str = std::to_string(rpm);

    // Launch a detached thread to send commands without freezing the UI
    std::thread([this, fan1_rpm_str, fan2_rpm_str]() {
        // Send command for Fan 1
        socket_client->send_command_async(SET_FAN_SPEED, "1 " + fan1_rpm_str).get();
        
        // Wait for 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        // Send command for Fan 2
        socket_client->send_command_async(SET_FAN_SPEED, "2 " + fan2_rpm_str).get();
    }).detach();
}

int VictusFanControl::validate_rpm(int rpm)
{
    // Allow 0 RPM (0 RPM mode) OR minimum 600 RPM
    if (rpm == 0) {
        return 0;
    }
    if (rpm < MIN_RPM_NONZERO) {
        return MIN_RPM_NONZERO;
    }
    return std::min(rpm, MAX_RPM_INPUT);
}

void VictusFanControl::update_profile_display()
{
    std::string display_text;
    
    if (profile_points.empty()) {
        display_text = "No profile points (0/10)";
    } else {
        for (size_t i = 0; i < profile_points.size(); ++i) {
            if (i > 0) display_text += "\n";
            display_text += std::to_string(i + 1) + ". " + 
                           std::to_string(profile_points[i].temperature) + "°C → " +
                           std::to_string(profile_points[i].rpm) + " RPM";
        }
    }
    
    gtk_label_set_text(GTK_LABEL(profile_points_list), display_text.c_str());
    
    std::string count_text = "Points: " + std::to_string(profile_points.size()) + "/" + 
                            std::to_string(MAX_PROFILE_POINTS);
    gtk_label_set_text(GTK_LABEL(point_count_label), count_text.c_str());
}

void VictusFanControl::add_profile_point()
{
    if (profile_points.size() >= MAX_PROFILE_POINTS) {
        std::cerr << "Maximum profile points (" << MAX_PROFILE_POINTS << ") reached" << std::endl;
        return;
    }

    int temp = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(temp_input)));
    int rpm = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(speed_input)));
    
    rpm = validate_rpm(rpm);

    // Check if temperature already exists in profile
    for (auto &point : profile_points) {
        if (point.temperature == temp) {
            point.rpm = rpm;
            update_profile_display();
            return;
        }
    }

    // Add new point and sort by temperature
    profile_points.push_back({temp, rpm});
    std::sort(profile_points.begin(), profile_points.end(),
              [](const FanProfilePoint &a, const FanProfilePoint &b) {
                  return a.temperature < b.temperature;
              });
    
    update_profile_display();
}

void VictusFanControl::remove_profile_point_at(int index)
{
    if (index >= 0 && index < static_cast<int>(profile_points.size())) {
        profile_points.erase(profile_points.begin() + index);
        update_profile_display();
    }
}

void VictusFanControl::apply_profile()
{
    if (profile_points.empty()) {
        std::cerr << "No profile points to apply" << std::endl;
        return;
    }

    // Build profile string: "temp1 rpm1 temp2 rpm2 ..."
    std::string profile_str;
    for (size_t i = 0; i < profile_points.size(); ++i) {
        if (i > 0) profile_str += " ";
        profile_str += std::to_string(profile_points[i].temperature) + " " +
                      std::to_string(profile_points[i].rpm);
    }

    auto result = socket_client->send_command_async(SET_FAN_PROFILE, profile_str).get();
    if (result != "OK") {
        std::cerr << "Failed to apply profile: " << result << std::endl;
    }
}

void VictusFanControl::on_mode_changed(GtkComboBox *widget, gpointer data)
{
    VictusFanControl *self = static_cast<VictusFanControl*>(data);
    const char *mode_id = gtk_combo_box_get_active_id(widget);

    if (mode_id) {
        std::string mode_str(mode_id);
        
        // Send the mode command and wait for it to complete.
        auto result = self->socket_client->send_command_async(SET_FAN_MODE, mode_str).get();

        if (result == "OK") {
            // Handle different modes
            if (mode_str == "MANUAL") {
                gtk_widget_set_sensitive(self->manual_box, TRUE);
                gtk_widget_set_sensitive(self->profile_box, FALSE);
            } else if (mode_str == "PROFILE") {
                gtk_widget_set_sensitive(self->manual_box, FALSE);
                gtk_widget_set_sensitive(self->profile_box, TRUE);
                self->apply_profile();
            } else {
                gtk_widget_set_sensitive(self->manual_box, FALSE);
                gtk_widget_set_sensitive(self->profile_box, FALSE);
            }
        } else {
            std::cerr << "Failed to set fan mode: " << result << std::endl;
        }
        
        // After all commands are sent, update the UI to reflect the final state.
        self->update_ui_from_system_state();
    }
}

void VictusFanControl::on_apply_rpm_clicked(GtkButton *button, gpointer data)
{
    VictusFanControl *self = static_cast<VictusFanControl*>(data);
    int rpm = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(self->rpm_input)));
    self->set_fan_rpm(rpm);
}

void VictusFanControl::on_add_point_clicked(GtkButton *button, gpointer data)
{
    VictusFanControl *self = static_cast<VictusFanControl*>(data);
    self->add_profile_point();
}

void VictusFanControl::on_remove_point_clicked(GtkButton *button, gpointer data)
{
    VictusFanControl *self = static_cast<VictusFanControl*>(data);
    if (!self->profile_points.empty()) {
        self->remove_profile_point_at(self->profile_points.size() - 1);
    }
}

void VictusFanControl::update_all_temperatures()
{
    try {
        auto result = socket_client->send_command_async(ServerCommands::GET_ALL_TEMPS, "").get();
        
        if (result == "N/A" || result.empty()) {
            gtk_label_set_text(GTK_LABEL(all_temps_label), "CPU: N/A | NVMe: N/A");
            gtk_label_set_text(GTK_LABEL(cpu_temp_label), "CPU Cores: N/A");
        } else {
            // Parse format: "PKG:48|CORES:40,39,43,45,43,45,45,45,45,45|NVME:37,36"
            std::string pkg_temp;
            std::string nvme_temps;
            std::string cores_temps;
            
            std::stringstream ss(result);
            std::string section;
            
            while (std::getline(ss, section, '|')) {
                if (section.find("PKG:") == 0) {
                    pkg_temp = section.substr(4);
                } else if (section.find("CORES:") == 0) {
                    cores_temps = section.substr(6);
                } else if (section.find("NVME:") == 0) {
                    nvme_temps = section.substr(5);
                }
            }
            
            // Display system temperatures at top with colored package temp
            if (!pkg_temp.empty()) {
                std::string display = "<span foreground='#FF6600'><b>CPU: " + pkg_temp + "°C</b></span>";
                
                // Add all NVMe temps
                if (!nvme_temps.empty()) {
                    std::stringstream nvme_ss(nvme_temps);
                    std::string nvme_temp;
                    int nvme_num = 1;
                    
                    while (std::getline(nvme_ss, nvme_temp, ',')) {
                        display += " | NVMe" + std::to_string(nvme_num) + ": " + nvme_temp + "°C";
                        nvme_num++;
                    }
                }
                
                gtk_label_set_markup(GTK_LABEL(all_temps_label), display.c_str());
            }
            
            // Display all CPU cores at bottom
            if (!cores_temps.empty()) {
                // Format cores nicely: "Core 0: 40°C, Core 1: 39°C, ..."
                std::stringstream cores_ss(cores_temps);
                std::string core;
                int core_num = 0;
                std::string cores_display = "CPU Cores: ";
                
                while (std::getline(cores_ss, core, ',')) {
                    if (core_num > 0) cores_display += ", ";
                    cores_display += "C" + std::to_string(core_num) + ":" + core + "°C";
                    core_num++;
                }
                
                gtk_label_set_text(GTK_LABEL(cpu_temp_label), cores_display.c_str());
            }
        }
    } catch (const std::exception &e) {
        // Failed to get temps
    }
}

void VictusFanControl::on_apply_profile_clicked(GtkButton *button, gpointer data)
{
    VictusFanControl *self = static_cast<VictusFanControl*>(data);
    self->apply_profile();
}
void VictusFanControl::on_interval_changed_static(GtkSpinButton *spin, gpointer data)
{
    VictusFanControl *self = static_cast<VictusFanControl*>(data);
    int new_interval = gtk_spin_button_get_value_as_int(spin);
    self->on_interval_changed(new_interval);
}

void VictusFanControl::on_interval_changed(int new_interval)
{
    if (new_interval != settings.update_interval_sec) {
        settings.update_interval_sec = new_interval;
        settings.save();
        // Restart timers with new interval
        if (temp_timer_id) g_source_remove(temp_timer_id);
        if (fan_timer_id) g_source_remove(fan_timer_id);
        
        temp_timer_id = g_timeout_add_seconds(new_interval, [](gpointer d) -> gboolean {
            static_cast<VictusFanControl*>(d)->update_all_temperatures();
            return G_SOURCE_CONTINUE;
        }, this);
        
        fan_timer_id = g_timeout_add_seconds(new_interval, [](gpointer d) -> gboolean {
            static_cast<VictusFanControl*>(d)->update_fan_speeds();
            return G_SOURCE_CONTINUE;
        }, this);
    }
}
#ifndef FAN_HPP
#define FAN_HPP

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include "socket.hpp"
#include "settings.hpp"

struct FanProfilePoint {
    int temperature;  // in Celsius
    int rpm;
};

class VictusFanControl
{
public:
	GtkWidget *fan_page;
	AppSettings settings;

	VictusFanControl(std::shared_ptr<VictusSocketClient> client);
	~VictusFanControl();

	GtkWidget *get_page();

private:
    // Mode UI Widgets
    GtkWidget *mode_selector;
    
    // Manual mode widgets
    GtkWidget *manual_box;
    GtkWidget *rpm_input;
    GtkWidget *apply_rpm_button;

    // Profile UI Widgets
    GtkWidget *profile_box;
    GtkWidget *profile_points_list;
    GtkWidget *profile_list_scrolled;
    GtkWidget *temp_input;
    GtkWidget *speed_input;
    GtkWidget *add_point_button;
    GtkWidget *remove_point_button;
    GtkWidget *apply_profile_button;

    // Settings UI Widgets
    GtkWidget *settings_box;
    GtkWidget *interval_spin;

    // Labels for displaying current state
	GtkWidget *state_label;
	GtkWidget *cpu_temp_label;
	GtkWidget *all_temps_label;
	GtkWidget *fan1_speed_label;
	GtkWidget *fan2_speed_label;
    GtkWidget *point_count_label;
    
    // Timer IDs for cleanup
    guint temp_timer_id;
    guint fan_timer_id;

    // Profile data (max 10 points)
    std::vector<FanProfilePoint> profile_points;
    static constexpr int MAX_PROFILE_POINTS = 10;

	void update_fan_speeds();
	void update_ui_from_system_state();
    void set_fan_rpm(int rpm);
    void update_profile_display();
    void add_profile_point();
    void remove_profile_point_at(int index);
    void apply_profile();
    int validate_rpm(int rpm);
    void update_all_temperatures();
    void on_interval_changed(int new_interval);

    // Signal handlers
	static void on_mode_changed(GtkComboBox *widget, gpointer data);
	static void on_interval_changed_static(GtkSpinButton *spin, gpointer data);
    static void on_apply_rpm_clicked(GtkButton *button, gpointer data);
    static void on_add_point_clicked(GtkButton *button, gpointer data);
    static void on_remove_point_clicked(GtkButton *button, gpointer data);
    static void on_apply_profile_clicked(GtkButton *button, gpointer data);
    static gboolean on_update_temperatures(gpointer data);

	std::shared_ptr<VictusSocketClient> socket_client;
};

#endif // FAN_HPP

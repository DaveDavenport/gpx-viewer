#ifndef __GPX_VIEWER_H__
#define __GPX_VIEWER_H__

typedef GtkApplication 		GpxViewer;
typedef GtkApplicationClass GpxViewerClass;

/* Make gtkbuilder happy */
void on_view_menu_files_dock_toggled(GtkCheckMenuItem *item, gpointer data);
void on_view_menu_track_info_dock_toggled(GtkCheckMenuItem *item, gpointer data);
void on_view_menu_settings_dock_toggled(GtkCheckMenuItem *item, gpointer data);

void on_destroy(GtkWidget *widget,GdkEvent *event, gpointer gpx_viewer);
void on_destroy_menu(GtkMenuItem *item , gpointer gpx_viewer);
void about_menuitem_activate_cb(void);

void show_marker_layer_toggled_cb(GtkSwitch * button, GParamSpec *spec,gpointer user_data);
void routes_list_changed_cb(GtkTreeSelection * sel, gpointer user_data);
void smooth_factor_change_value_cb(GtkSpinButton * spin, gpointer user_data);
void graph_show_points_toggled_cb(GtkSwitch * button, GParamSpec *spec, gpointer user_data);
void map_zoom_level_change_value_cb(GtkSpinButton * spin, gpointer user_data);
void playback_play_clicked(GtkWidget *widget, GdkEvent *event,gpointer user_data);
void playback_pause_clicked(GtkWidget *widget, GdkEvent *event,gpointer user_data);
void playback_stop_clicked(GtkWidget *widget, GdkEvent *event,gpointer user_data);
void main_window_size_changed(GtkWindow *win, GtkAllocation *alloc, gpointer data);
void row_visible_toggled(GtkCellRendererToggle *toggle, const gchar *path, gpointer data);
void show_elevation(GtkMenuItem *item, gpointer user_data);
void show_speed(GtkMenuItem *item, gpointer user_data);
void show_distance(GtkMenuItem *item, gpointer user_data);
void map_selection_combo_changed_cb(GtkComboBox *box, gpointer data);
void open_gpx_file(GtkMenu *item, GpxViewer *gpx_viewer);

void close_show_current_track(GtkWidget *widget,gint response_id, gpointer user_data); 
void show_current_track(GtkWidget *menu_item, gpointer user_data);
void gpx_viewer_preferences_close(GtkWidget *dialog, gint respose, gpointer user_data); 
void playback_speedup_spinbutton_value_changed_cb(GtkSpinButton *sp, gpointer gpx_viewer);
void gpx_viewer_show_preferences_dialog(GtkWidget *menu_item, gpointer user_data);
void show_vertical_speed(GtkMenuItem *item, gpointer user_data);
void show_acceleration_h(GtkMenuItem *item, gpointer user_data);

enum _SpeedFormat
{
	DISTANCE,
	SPEED,
	ELEVATION,
	ACCEL,
	NA
};
typedef enum _SpeedFormat SpeedFormat;

gchar * gpx_viewer_misc_convert(gdouble speed, SpeedFormat format);

void gv_set_speed_label(GtkWidget *label, gdouble speed, SpeedFormat format);
void show_waypoints_layer_toggled_cb(GtkSwitch * button, GParamSpec *spec,gpointer user_data);
#endif

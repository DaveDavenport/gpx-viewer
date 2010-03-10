#ifndef __GPX_VIEWER_H__
#define __GPX_VIEWER_H__
/* Make gtkbuilder happy */
void on_view_menu_files_dock_toggled(GtkCheckMenuItem *item, gpointer data);
void on_view_menu_track_info_dock_toggled(GtkCheckMenuItem *item, gpointer data);
void on_view_menu_settings_dock_toggled(GtkCheckMenuItem *item, gpointer data);

void on_destroy(void);
void about_menuitem_activate_cb(void);

void show_marker_layer_toggled_cb(GtkToggleButton * button, gpointer user_data);
void routes_list_changed_cb(GtkTreeSelection * sel, gpointer user_data);
void smooth_factor_change_value_cb(GtkSpinButton * spin, gpointer user_data);
void graph_show_points_toggled_cb(GtkToggleButton * button, gpointer user_data);
void map_zoom_level_change_value_cb(GtkSpinButton * spin, gpointer user_data);
void playback_play_clicked(void);
void playback_pause_clicked(void);
void playback_stop_clicked(void);
void main_window_size_changed(GtkWindow *win, GtkAllocation *alloc, gpointer data);
void row_visible_toggled(GtkCellRendererToggle *toggle, const gchar *path, gpointer data);
void show_elevation(GtkMenuItem item, gpointer user_data);
void show_speed(GtkMenuItem item, gpointer user_data);
void show_distance(GtkMenuItem item, gpointer user_data);
void map_selection_combo_changed_cb(GtkComboBox *box, gpointer data);
void open_gpx_file(GtkMenu *item);

void close_show_current_track(GtkWidget *widget,gint response_id, GtkBuilder *fbuilder);
void show_current_track(void);
#endif

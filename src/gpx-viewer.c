/* Gpx Viewer
 * Copyright (C) 2009-2010 Qball Cow <qball@sarine.nl>
 * Project homepage: http://blog.sarine.nl/

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <config.h>
#define __USE_POSIX
#include <time.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <champlain/champlain.h>
#include <champlain-gtk/champlain-gtk.h>
#include <clutter-gtk/clutter-gtk.h>
#include <unique/unique.h>
#include <gdl/gdl.h>
#include "gpx-viewer.h"
#include "gpx.h"

static GtkWidget        *dock_items[3];
static GdlDockLayout    *dock_layout = NULL;

static GpxViewerPreferences  *preferences = NULL;

UniqueApp           *app                    = NULL;
typedef enum
{
    OPEN_URIS   =   1
}AppCommands;
/* List of gpx files */
GList               *files                  = NULL;
/* The Map view widget */
GtkWidget           *champlain_view         = NULL;
/* The interface builder */
GtkBuilder          *builder                = NULL;
/* The graph */
GpxGraph            *gpx_graph              = NULL;
/* The container holding the graph */
GtkWidget           *gpx_graph_container    = NULL;
/* Recent manager */
GtkRecentManager    *recent_man             = NULL;
/* */
GpxPlayback *playback               = NULL;
/* */
ClutterActor *click_marker          = NULL;
guint click_marker_source           = 0;

/* List of routes */
GList *routes                       = NULL;

/* Clutter  collors */
ClutterColor waypoint               = { 0xf3, 0x94, 0x07, 0xff };
ClutterColor highlight_track_color  = { 0xf3, 0x94, 0x07, 0xff };
ClutterColor normal_track_color     = { 0x00, 0x00, 0xff, 0x66 };

/**
 * This structure holds all information related to
 * a track.
 * The file the track belongs to, the polygon on the map start/stop marker
 * and the visible state.
 */
typedef struct Route
{
    GpxFile *file;
    GpxTrack *track;
    ChamplainPolygon *polygon;
    ChamplainBaseMarker *start;
    ChamplainBaseMarker *stop;
    gboolean visible;
} Route;

/**
 * This points to the currently active Route.
 */
Route *active_route                 = NULL;

/**
 * Dock loading/restoring
 */

static void restore_layout(void)
{
    const gchar *config_dir = g_get_user_config_dir();
    gchar *layout_path = NULL;
    g_assert(config_dir != NULL);

    layout_path = g_build_filename(config_dir, "gpx-viewer", "dock-layout.xml",NULL);
    if(g_file_test(layout_path, G_FILE_TEST_EXISTS))
    {
        gdl_dock_layout_load_from_file(dock_layout, layout_path);
        gdl_dock_layout_load_layout(dock_layout, "my_layout");
    }
    g_free(layout_path);

}


static void save_layout(void)
{
    gchar *layout_path = NULL;
    const gchar *config_dir = g_get_user_config_dir();
    g_assert(config_dir != NULL);
    g_debug("Config dir is: %s", config_dir);

    layout_path = g_build_filename(config_dir, "gpx-viewer", NULL);
    if(!g_file_test(layout_path, G_FILE_TEST_IS_DIR))
    {
        g_mkdir_with_parents(layout_path, 0700);
    }
    g_free(layout_path);
    layout_path = g_build_filename(config_dir, "gpx-viewer", "dock-layout.xml",NULL);
    if(dock_layout)
    {
        g_debug("Saving layout: %s", layout_path);
        gdl_dock_layout_save_layout(dock_layout, "my_layout");
        gdl_dock_layout_save_to_file(dock_layout, layout_path);
    }
    g_free(layout_path);
}


/**
 * On program quit
 */
static void free_Route(Route *route)
{
    /* Do not free these. The are (now) automagically cleanup
       when main widget is destroyed*/
    /*	if(route->polygon) g_object_unref(route->polygon); */
    if(route->track) g_object_unref(route->track);
    g_free(route);
}


/**
 * This is called when the main window is destroyed
 */
void on_destroy(void)
{
    g_debug("Quit...");
    gtk_main_quit();

    save_layout();

    gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(builder, "gpx_viewer_window")));
    g_object_unref(builder);

    g_debug("Cleanup routes");
    g_list_foreach(g_list_first(routes), (GFunc)free_Route, NULL);
    g_list_free(routes); routes = NULL;
}


/**
 * The about dialog
 */
void about_menuitem_activate_cb(void)
{
    const gchar *authors[] =
    {
        "Qball Cow <qball@sarine.nl>",
        "Andrew Harvey",
        NULL
    };

    const char *gpl_short_version =
        "This program is free software; you can redistribute it and/or modify\n"\
        "it under the terms of the GNU General Public License as published by\n"\
        "the Free Software Foundation; either version 2 of the License, or\n"\
        "(at your option) any later version.\n"\
        "\n"\
        "This program is distributed in the hope that it will be useful,\n"\
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"\
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"\
        "GNU General Public License for more details.\n"\
        "\n"\
        "You should have received a copy of the GNU General Public License along\n"\
        "with this program; if not, write to the Free Software Foundation, Inc.,\n"\
        "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.";

    /* TODO: Add translators here */
    gtk_show_about_dialog (NULL,
        "program-name",     PACKAGE_NAME,
        "logo-icon-name",   "gpx-viewer",
        "website",          PACKAGE_URL,
        "website-label",    PACKAGE_URL,
        "license",          gpl_short_version,
        "version",          VERSION,
        "authors",          authors,
        "comments",         _("A simple program to visualize one or more gpx files."),
        "title",            _("About GPX Viewer"),
        NULL);
}


/**
 * Tool function for readability
 */

static GString *misc_get_time(time_t temp)
{
    GString *string = g_string_new("");
    gulong hour = temp / 3600;
    gulong minutes = ((temp % 3600) / 60);
    gulong seconds = (temp % 60);
    if (hour > 0)
    {
        g_string_append_printf(string, "%lu %s", hour,g_dngettext(NULL, "hour", "hours", hour));
    }

    if (minutes > 0)
    {
        if (hour > 0)
            g_string_append(string, ", ");
        g_string_append_printf(string, "%lu %s", minutes, g_dngettext(NULL, "minute", "minutes",minutes));
    }

    if (seconds > 0)
    {
        if (minutes > 0)
            g_string_append(string, ", ");
        g_string_append_printf(string, "%lu %s", seconds, g_dngettext(NULL, "second", "seconds",seconds));
    }
    return string;
}


/**
 * Update on track changes
 * TODO: This function is _way_ to long.
 */
static void interface_update_heading(GtkBuilder * c_builder, GpxTrack * track, GpxPoint *start, GpxPoint *stop)
{
    time_t temp = 0;
    gdouble gtemp = 0;
    double max_speed = 0;
    double points = 0;
    GtkWidget *label = NULL;
    /* Duration */
    label = (GtkWidget *) gtk_builder_get_object(builder, "duration_label");
    if(start && stop)
    {
        temp = gpx_point_get_time(stop) - gpx_point_get_time(start);
    }
    if (temp > 0)
    {
        GString *string = misc_get_time(temp);
        gtk_label_set_text(GTK_LABEL(label), string->str);
        g_string_free(string, TRUE);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }
    /* Start time */
    label = (GtkWidget *) gtk_builder_get_object(builder, "start_time_label");
    if(start) {
            char buffer[128];
            struct tm ltm;
            temp = gpx_point_get_time(start);
            localtime_r(&temp, &ltm);
            strftime(buffer, 128, "%D %X", &ltm);
            gtk_label_set_text(GTK_LABEL(label), buffer); 
    }else{
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Stop time */
    label = (GtkWidget *) gtk_builder_get_object(builder, "stop_time_label");
    if(stop) {
            char buffer[128];
            struct tm ltm;
            temp = gpx_point_get_time(stop);
            localtime_r(&temp, &ltm);
            strftime(buffer, 128, "%D %X", &ltm);
            gtk_label_set_text(GTK_LABEL(label), buffer); 
    }else{
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }
    /* Distance */
    label = (GtkWidget *) gtk_builder_get_object(builder, "distance_label");
    if(start && stop)
    {
        gtemp = stop->distance-start->distance;
    }
    if (gtemp > 0)
    {
        gchar *string = g_strdup_printf("%.2f km", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Average */
    label = (GtkWidget *) gtk_builder_get_object(builder, "average_label");
    gtemp = 0;
    if(start && stop)
    {
        gtemp = gpx_track_calculate_point_to_point_speed(track,start, stop);
    }
    if (gtemp > 0)
    {
        gchar *string = g_strdup_printf("%.2f km/h", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Moving Average */
    label = (GtkWidget *) gtk_builder_get_object(builder, "moving_average_label");
    gtemp = 0;
    temp = 0;
    if(start && stop)
    {
        /* Calculates both time and km/h */
        gtemp = gpx_track_calculate_moving_average(track,start, stop, &temp);
    }
    if (gtemp > 0)
    {
        gchar *string = g_strdup_printf("%.2f km/h", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    label = (GtkWidget *) gtk_builder_get_object(builder, "moving_average_time_label");
    if (gtemp > 0)
    {
        GString *string = misc_get_time(temp);
        gtk_label_set_text(GTK_LABEL(label), string->str);
        g_string_free(string, TRUE);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Max speed */
    if(track && start && stop)
    {
        GList *list ;
        for(list = g_list_find(track->points, start); list && list->data != stop; list = g_list_next(list))
        {
            points++;
            max_speed = MAX(max_speed, ((GpxPoint *)list->data)->speed);
        }
        points++;

    }
    label = (GtkWidget *) gtk_builder_get_object(builder, "max_speed_label");
    if (max_speed > 0)
    {
        gchar *string = g_strdup_printf("%.2f km/h", max_speed);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Gradient */
    {
        gdouble elevation_diff = 0;
        gdouble distance_diff = 0;
        label = (GtkWidget *) gtk_builder_get_object(builder, "gradient_label");

        if (start && stop)
        {
            elevation_diff = stop->elevation - start->elevation;
            distance_diff = stop->distance - start->distance;
        }
        if (distance_diff > 0)
        {
            /* The gradient here is a percentage, using the start and end point only.
               distance_diff is in km so we *1000 to change to m first as elevation_diff
               is in the same units as supplied in the GPX file which as per the GPX
               schema is meters. */
            gchar *string = g_strdup_printf("%.2f %%", (elevation_diff / (distance_diff*1000)) * 100);
            gtk_label_set_text(GTK_LABEL(label), string);
            g_free(string);
        }
        else
        {
            gtk_label_set_text(GTK_LABEL(label), "n/a");
        }

        /* Elevation Difference */
        label = (GtkWidget *) gtk_builder_get_object(builder, "elevation_difference_label");

        elevation_diff = 0;
        if (start && stop)
        {
            elevation_diff = stop->elevation - start->elevation;
            distance_diff = stop->distance - start->distance;
        }
        if (distance_diff > 0)
        {
            gchar *string = g_strdup_printf("%.2f m", elevation_diff);
            gtk_label_set_text(GTK_LABEL(label), string);
            g_free(string);
        }
        else
        {
            gtk_label_set_text(GTK_LABEL(label), "n/a");
        }
    }
}


/**
 * Creates a Polygon for Route, and adds it to the view
 */
static void interface_map_plot_route(ChamplainView * view, struct Route *route)
{
    GList *iter;
    /* If Route has allready a route, exit */
    if(route->polygon != NULL)
    {
        g_warning("Route allready has a polygon.\n");
        return;
    }
    route->polygon = champlain_polygon_new();
    for (iter = g_list_first(route->track->points); iter; iter = iter->next)
    {
        GpxPoint *p = iter->data;
        champlain_polygon_append_point(route->polygon, p->lat_dec, p->lon_dec);
    }
    champlain_polygon_set_stroke_width(route->polygon, 5.0);
    champlain_polygon_set_stroke_color(route->polygon, &normal_track_color);
    champlain_view_add_polygon(CHAMPLAIN_VIEW(view), route->polygon);
    if(!route->visible) champlain_polygon_hide(route->polygon);
}


static void interface_map_file_waypoints(ChamplainView *view, GpxFile *file)
{
    GList *it;
    for(it = g_list_first(file->waypoints); it; it = g_list_next(it))
    {
        GpxPoint *p = it->data;
        gpx_viewer_map_view_add_waypoint(GPX_VIEWER_MAP_VIEW(champlain_view),p);
    }
}


static void interface_map_make_waypoints(ChamplainView * view)
{
    GList *iter;
    for (iter = g_list_first(files); iter != NULL; iter = g_list_next(iter))
    {
        GpxFile *file = iter->data;
        interface_map_file_waypoints(view, file);
    }
}


/* Show and hide waypoint layer */
void show_marker_layer_toggled_cb(GtkToggleButton * button, gpointer user_data)
{
    gboolean active = gtk_toggle_button_get_active(button);
    if(active != gpx_viewer_map_view_get_show_waypoints(GPX_VIEWER_MAP_VIEW(champlain_view)))
    {
        gpx_viewer_map_view_set_show_waypoints(GPX_VIEWER_MAP_VIEW(champlain_view), active);
    }
}


static void show_marker_layer_changed(GpxViewerMapView *view, GParamSpec * gobject, GtkWidget *sp)
{
    gboolean active = gpx_viewer_map_view_get_show_waypoints(GPX_VIEWER_MAP_VIEW(champlain_view));
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp)) != active)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp), active);
    }
}


/**
 * Handle user selecting another track
 */
void routes_list_changed_cb(GtkTreeSelection * sel, gpointer user_data)
{
    GtkTreeView *tree = gtk_tree_selection_get_tree_view(sel);
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    GtkTreeIter iter;
    if(gtk_tree_selection_get_selected(sel, &model, &iter))
    {
        Route *route = NULL;
        gtk_tree_model_get(model, &iter, 1, &route, -1);
        /* Unset active route */
        if (active_route)
        {
            /* Give it ' non-active' colour */
            champlain_polygon_set_stroke_color(active_route->polygon, &normal_track_color);
            /* Hide stop marker */
            if(active_route->stop)
                clutter_actor_hide(CLUTTER_ACTOR(active_route->stop));

            /* Hide start marker */
            if(active_route->start)
                clutter_actor_hide(CLUTTER_ACTOR(active_route->start));

            /* Stop playback */
            gpx_playback_stop(playback);
            /* Clear graph */
            gpx_graph_set_track(gpx_graph, NULL);
            /* Hide graph */
            gtk_widget_hide(GTK_WIDGET(gpx_graph_container));
        }

        active_route = route;
        if (route)
        {
            ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));

            gpx_playback_set_track(playback, active_route->track);
            if(route->polygon != NULL)
                champlain_polygon_set_stroke_color(route->polygon, &highlight_track_color);

            if (route->visible)
            {
                champlain_polygon_show(route->polygon);
            }
            if (route->track->top && route->track->bottom)
            {
                champlain_view_ensure_visible(view,
                    route->track->top->lat_dec, route->track->top->lon_dec,
                    route->track->bottom->lat_dec, route->track->bottom->lon_dec, FALSE);
            }

            if (gpx_track_get_total_time(active_route->track) > 5)
            {
                gpx_graph_set_track(gpx_graph, active_route->track);
                gtk_widget_show(GTK_WIDGET(gpx_graph_container));
            }
            else
            {
                gpx_graph_set_track(gpx_graph, NULL);
                gtk_widget_hide(GTK_WIDGET(gpx_graph_container));
            }

            if(route->stop)
                clutter_actor_show(CLUTTER_ACTOR(route->stop));

            if(route->start)
                clutter_actor_show(CLUTTER_ACTOR(route->start));
        }
        else
        {
            /* Create a false route here. f.e. to show multiple tracks concatenated */
        }
    }
}


static void map_view_map_source_changed(GpxViewerMapView *view, GParamSpec * gobject, GtkWidget *combo)
{
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    GtkTreeIter iter;
    const gchar *source_id = gpx_viewer_map_view_get_map_source(view);

    if(gtk_tree_model_get_iter_first(model, &iter))
    {
        do
        {
            gchar *a;
            gtk_tree_model_get(model, &iter,1, &a, -1);
            if(strcmp(a, source_id) == 0)
            {
                g_debug("map_view_source_changed: %s",source_id);
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
                g_free(a);
                return;
            }
            g_free(a);
        }while(gtk_tree_model_iter_next(model, &iter));
    }
}


/* Smooth factor changed */
static void smooth_factor_changed(GpxGraph * graph, GParamSpec * gobject, GtkSpinButton * spinbutton)
{
    gint zoom;
    g_object_get(G_OBJECT(graph), "smooth-factor", &zoom, NULL);
    gtk_spin_button_set_value(spinbutton, zoom);
}


void smooth_factor_change_value_cb(GtkSpinButton * spin, gpointer user_data)
{
    int current = gpx_graph_get_smooth_factor(gpx_graph);
    int new = gtk_spin_button_get_value_as_int(spin);
    if (current != new)
    {
        gpx_graph_set_smooth_factor(gpx_graph, new);
    }
}


/* Show and hide points on graph */
void graph_show_points_toggled_cb(GtkToggleButton * button, gpointer user_data)
{
    gboolean new = gtk_toggle_button_get_active(button);
    gpx_graph_set_show_points(gpx_graph, new);
}


static void graph_show_points_changed(GpxGraph *graph, GParamSpec *sp, GtkWidget *toggle)
{
    int current = gpx_graph_get_show_points(gpx_graph);
    if(current != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),current);
    }
}


void map_zoom_level_change_value_cb(GtkSpinButton * spin, gpointer user_data)
{
    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    int current = champlain_view_get_zoom_level(view);
    int new = gtk_spin_button_get_value_as_int(spin);
    if (current != new)
    {
        champlain_view_set_zoom_level(view, new);
    }
}


static gboolean graph_point_remove(ClutterActor * marker)
{
    clutter_actor_destroy(click_marker);
    click_marker = NULL;
    click_marker_source =0;
    return FALSE;
}


static void graph_selection_changed(GpxGraph *graph,GpxTrack *track, GpxPoint *start, GpxPoint *stop)
{
    interface_update_heading(builder, track, start, stop);
    if(active_route && active_route->track->points != NULL)
    {
        if(start)
        {
            champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(active_route->start), start->lat_dec, start->lon_dec);
        }

        if(stop)
        {
            champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(active_route->stop), stop->lat_dec, stop->lon_dec);
        }
    }
}


static void graph_point_clicked(GpxGraph *graph, GpxPoint *point)
{
    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    ChamplainBaseMarker *marker[2] = {NULL, NULL};
    if(click_marker == NULL)
    {
        GtkIconInfo *ii = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
            "pin-red",
            100, 0);

        if (ii)
        {
            const gchar *path2 = gtk_icon_info_get_filename(ii);
            if (path2)
            {
                click_marker = champlain_marker_new_from_file(path2, NULL);
                champlain_marker_set_draw_background(CHAMPLAIN_MARKER(click_marker), FALSE);
            }
        }
        if (!click_marker)
        {
            click_marker = champlain_marker_new();
        }
        /* Create the marker */
        champlain_marker_set_color(CHAMPLAIN_MARKER(click_marker), &waypoint);
        gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), CHAMPLAIN_BASE_MARKER(click_marker));
    }

    champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(click_marker), point->lat_dec, point->lon_dec);
    clutter_actor_show(CLUTTER_ACTOR(click_marker));

    if(click_marker_source >0)
    {
        g_source_remove(click_marker_source);
    }

    marker[0] =(ChamplainBaseMarker *) click_marker;
    champlain_view_ensure_markers_visible(view, marker, FALSE);

    click_marker_source = g_timeout_add_seconds(5, (GSourceFunc) graph_point_remove, click_marker);
}


void playback_play_clicked(void)
{
    if(active_route)
    {
        gpx_playback_start(playback);
    }
}


void playback_pause_clicked(void)
{
    if(active_route)
    {
        gpx_playback_pause(playback);
    }
}


void playback_stop_clicked(void)
{
    if(active_route)
    {
        gpx_playback_stop(playback);
    }
}


static void route_playback_tick(GpxPlayback *route_playback, GpxPoint *current)
{
    if(current != NULL)
    {
        time_t ptime = gpx_point_get_time(current);

        gpx_graph_show_info(gpx_graph, current);
        gpx_graph_set_highlight(gpx_graph, ptime);

        graph_point_clicked(gpx_graph, current);
    }
    else
    {
        gpx_graph_set_highlight(gpx_graph, 0);
        gpx_graph_hide_info(gpx_graph);
    }
}


static void route_playback_state_changed(GpxPlayback *route_playback, GpxPlaybackState state)
{
    GtkWidget *w_stopped =(GtkWidget *) gtk_builder_get_object(builder, "eventbox2");
    GtkWidget *w_play = (GtkWidget *)gtk_builder_get_object(builder, "eventbox3");
    GtkWidget *w_paused = (GtkWidget *)gtk_builder_get_object(builder, "eventbox1");
    if(state == GPX_PLAYBACK_STATE_STOPPED)
    {
        g_debug("playback stopped");
        gtk_widget_set_sensitive(w_stopped, FALSE);
        gtk_widget_set_sensitive(w_play, TRUE);
        gtk_widget_set_sensitive(w_paused, FALSE);
    }
    else if (state == GPX_PLAYBACK_STATE_PAUSED)
    {
        g_debug("playback paused");
        gtk_widget_set_sensitive(w_stopped, TRUE);
        gtk_widget_set_sensitive(w_play, TRUE);
        gtk_widget_set_sensitive(w_paused, TRUE);
    }
    else if  (state == GPX_PLAYBACK_STATE_PLAY)
    {
        g_debug("playback started");
        gtk_widget_set_sensitive(w_stopped, TRUE);
        gtk_widget_set_sensitive(w_play, FALSE);
        gtk_widget_set_sensitive(w_paused, TRUE);
    }

}


static gboolean first = TRUE;
static void interface_plot_add_track(GtkTreeIter *parent, GpxTrack *track, double *lat1, double *lon1, double *lat2, double *lon2)
{
    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    /* Plot all tracks, and get total bounding box */
    GtkTreeIter liter;
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
    GtkIconInfo *ii;
    struct Route *route = g_new0(Route, 1);
    /* Route */
    /*    if(config_get_boolean("Track", "Cleanup speed using chauvenets criterion", FALSE))
        {
            route->track = gpx_track_cleanup_speed(track);
        }
        else
        {*/
    route->track = g_object_ref(track);
    /*    }*/
    route->visible = TRUE;

    /* draw the track */
    interface_map_plot_route(view, route);
    if (track->top && track->top->lat_dec < *lat1)
        *lat1 = track->top->lat_dec;
    if (track->top && track->top->lon_dec < *lon1)
        *lon1 = track->top->lon_dec;

    if (track->bottom && track->bottom->lat_dec > *lat2)
        *lat2 = track->bottom->lat_dec;
    if (track->bottom && track->bottom->lon_dec > *lon2)
        *lon2 = track->bottom->lon_dec;

    gtk_tree_store_append(GTK_TREE_STORE(model), &liter,parent);
    gtk_tree_store_set(GTK_TREE_STORE(model), &liter,
        0, (gpx_track_get_name(route->track)) ? gpx_track_get_name(route->track): "n/a",
        1, route,
        2, TRUE,
        3, route->visible,
        4, g_list_length(route->track->points),
		5, gpx_track_get_track_average(route->track),
        -1);

    if(first)
    {
        GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &liter);
        if(path != NULL)
        {
            gtk_tree_view_expand_to_path(GTK_TREE_VIEW(gtk_builder_get_object(builder, "TracksTreeView")), path);
            gtk_tree_path_free(path);
        }

    }
    first = FALSE;
    /* Pin's */
    if(route->track)
    {
        const GList *start = g_list_first(route->track->points);
        const GList *stop = g_list_last(route->track->points);
        if(start && stop)
        {
            ii = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
                "pin-green",
                100, 0);
            if (ii)
            {
                const gchar *path2 = gtk_icon_info_get_filename(ii);
                if (path2)
                {
                    route->start = (ChamplainBaseMarker *)champlain_marker_new_from_file(path2, NULL);
                    champlain_marker_set_draw_background(CHAMPLAIN_MARKER(route->start), FALSE);
                }
                gtk_icon_info_free(ii);
            }
            if (!route->start)
            {
                route->start = (ChamplainBaseMarker *)champlain_marker_new();
            }
            /* Create the marker */
            champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(route->start),
                ((GpxPoint*)start->data)->lat_dec,
                ((GpxPoint*)start->data)->lon_dec);
            champlain_marker_set_color(CHAMPLAIN_MARKER(route->start), &waypoint);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), route->start);

            ii = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
                "pin-blue",
                100, 0);
            if (ii)
            {
                const gchar *path2 = gtk_icon_info_get_filename(ii);
                if (path2)
                {
                    route->stop =  (ChamplainBaseMarker *)champlain_marker_new_from_file(path2, NULL);
                    champlain_marker_set_draw_background(CHAMPLAIN_MARKER(route->stop), FALSE);
                }
                gtk_icon_info_free(ii);
            }
            if (!route->stop)
            {
                route->stop = (ChamplainBaseMarker *)champlain_marker_new();
            }
            /* Create the marker */
            champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(route->stop),
                ((GpxPoint*)stop->data)->lat_dec,
                ((GpxPoint*)stop->data)->lon_dec);
            champlain_marker_set_color(CHAMPLAIN_MARKER(route->stop), &waypoint);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), route->stop);

            clutter_actor_hide(CLUTTER_ACTOR(route->stop));
            clutter_actor_hide(CLUTTER_ACTOR(route->start));
        }
    }

    routes = g_list_append(routes, route);
}


static void main_window_pane_pos_changed(GtkWidget * panel, GParamSpec * arg1, gpointer data)
{
    gint position = 0;
    g_object_get(G_OBJECT(panel), "position", &position, NULL);
    gpx_viewer_preferences_set_integer(preferences, "Window", "main_view_pane_pos", position);
    g_debug("Position: %i\n", position);
}


static void main_window_pane2_pos_changed(GtkWidget * panel, GParamSpec * arg1, gpointer data)
{
    gint position = 0;
    g_object_get(G_OBJECT(panel), "position", &position, NULL);
    gpx_viewer_preferences_set_integer(preferences, "Window", "main_view_pane2_pos", position);
    g_debug("Position2: %i\n", position);
}


void main_window_size_changed(GtkWindow *win, GtkAllocation *alloc, gpointer data)
{
    if(alloc)
    {
        gpx_viewer_preferences_set_integer(preferences, "Window", "width", alloc->width);
        gpx_viewer_preferences_set_integer(preferences, "Window", "height", alloc->height);
        g_debug("size: %i - %i\n", alloc->width, alloc->height);
    }

}


void row_visible_toggled(GtkCellRendererToggle *toggle, const gchar *path, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
    GtkTreeIter iter;
    struct Route *route;

    if(gtk_tree_model_get_iter_from_string(model, &iter, path))
    {
        gtk_tree_model_get(model, &iter, 1, &route, -1);
        if(route)
        {
            gboolean active = !gtk_cell_renderer_toggle_get_active(toggle);
            gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 3, active, -1);
            route->visible = active;
            if (active)
            {
                champlain_polygon_show(route->polygon);
            }
            else
            {
                champlain_polygon_hide(route->polygon);
            }
        }
    }
}


void show_elevation(GtkMenuItem item, gpointer user_data)
{
    if(gpx_graph_get_mode(gpx_graph) != GPX_GRAPH_GRAPH_MODE_ELEVATION)
    {
        g_debug("switch to elevation\n");
        gpx_graph_set_mode(gpx_graph, GPX_GRAPH_GRAPH_MODE_ELEVATION);
    }
}


void show_speed(GtkMenuItem item, gpointer user_data)
{
    if(gpx_graph_get_mode(gpx_graph) != GPX_GRAPH_GRAPH_MODE_SPEED)
    {
        g_debug("switch to speed\n");
        gpx_graph_set_mode(gpx_graph, GPX_GRAPH_GRAPH_MODE_SPEED);
    }
}


void show_distance(GtkMenuItem item, gpointer user_data)
{
    if(gpx_graph_get_mode(gpx_graph) != GPX_GRAPH_GRAPH_MODE_DISTANCE)
    {
        g_debug("switch to distance\n");
        gpx_graph_set_mode(gpx_graph, GPX_GRAPH_GRAPH_MODE_DISTANCE);
    }
}

void show_acceleration_h(GtkMenuItem item, gpointer user_data)
{
    if(gpx_graph_get_mode(gpx_graph) != GPX_GRAPH_GRAPH_MODE_ACCELERATION_H)
    {
        g_debug("switch to acceleration\n");
        gpx_graph_set_mode(gpx_graph, GPX_GRAPH_GRAPH_MODE_ACCELERATION_H);
    }
}
void show_vertical_speed(GtkMenuItem item, gpointer user_data)
{
    if(gpx_graph_get_mode(gpx_graph) != GPX_GRAPH_GRAPH_MODE_SPEED_V)
    {
        g_debug("switch to vertical speed\n");
        gpx_graph_set_mode(gpx_graph, GPX_GRAPH_GRAPH_MODE_SPEED_V);
    }
}



static void interface_create_fake_master_track(GpxFile *file, GtkTreeIter *liter)
{
    GList *iter;
    GtkIconInfo *ii;
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
    struct Route *route = g_new0(Route, 1);
    route->visible = TRUE;
    route->polygon = NULL;
    route->track = gpx_track_new();
    gpx_track_set_name(route->track, _("Combined track"));

    if (file->tracks)
    {
        for (iter = g_list_first(file->tracks); iter; iter = g_list_next(iter))
        {
            GList *piter;
            for(piter = g_list_first(GPX_TRACK(iter->data)->points); piter != NULL; piter = g_list_next(piter))
            {
                GpxPoint *p = piter->data;
                gpx_track_add_point(route->track, gpx_point_copy(p));
            }
        }
    }
    if(file->routes)
    {
        for (iter = g_list_first(file->routes); iter; iter = g_list_next(iter))
        {
            GList *piter;
            for(piter = g_list_first(GPX_TRACK(iter->data)->points); piter != NULL; piter = g_list_next(piter))
            {
                GpxPoint *p = piter->data;
                gpx_track_add_point(route->track, gpx_point_copy(p));
            }
        }
    }

    if(route->track)
    {
        const GList *start = g_list_first(route->track->points);
        const GList *stop = g_list_last(route->track->points);
        if(start && stop)
        {
            ii = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
                "pin-green",
                100, 0);
            if (ii)
            {
                const gchar *path2 = gtk_icon_info_get_filename(ii);
                if (path2)
                {
                    route->start = (ChamplainBaseMarker *)champlain_marker_new_from_file(path2, NULL);
                    champlain_marker_set_draw_background(CHAMPLAIN_MARKER(route->start), FALSE);
                }
                gtk_icon_info_free(ii);
            }
            if (!route->start)
            {
                route->start = (ChamplainBaseMarker *)champlain_marker_new();
            }
            /* Create the marker */
            champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(route->start),
                ((GpxPoint*)start->data)->lat_dec,
                ((GpxPoint*)start->data)->lon_dec);
            champlain_marker_set_color(CHAMPLAIN_MARKER(route->start), &waypoint);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), route->start);

            ii = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
                "pin-blue",
                100, 0);
            if (ii)
            {
                const gchar *path2 = gtk_icon_info_get_filename(ii);
                if (path2)
                {
                    route->stop =  (ChamplainBaseMarker *)champlain_marker_new_from_file(path2, NULL);
                    champlain_marker_set_draw_background(CHAMPLAIN_MARKER(route->stop), FALSE);
                }
                gtk_icon_info_free(ii);
            }
            if (!route->stop)
            {
                route->stop = (ChamplainBaseMarker *)champlain_marker_new();
            }
            /* Create the marker */
            champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(route->stop),
                ((GpxPoint*)stop->data)->lat_dec,
                ((GpxPoint*)stop->data)->lon_dec);
            champlain_marker_set_color(CHAMPLAIN_MARKER(route->stop), &waypoint);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), route->stop);

            clutter_actor_hide(CLUTTER_ACTOR(route->stop));
            clutter_actor_hide(CLUTTER_ACTOR(route->start));
        }
    }
    routes = g_list_append(routes, route);
    gtk_tree_store_set(GTK_TREE_STORE(model), liter, 1, route, -1);
}


static void recent_chooser_file_picked(GtkRecentChooser *grc, gpointer data)
{
    GtkTreeIter liter;
    double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
    GList *iter;
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
    gchar *basename,*uri = gtk_recent_chooser_get_current_uri(grc);
    GFile *afile = g_file_new_for_uri(uri);
    /* Try to open the gpx file */
    GpxFile *file = gpx_file_new(afile);
    g_object_unref(afile);
    g_free(uri);
    files = g_list_append(files, file);

    basename = g_file_get_basename(file->file);
    gtk_tree_store_append(GTK_TREE_STORE(model), &liter, NULL);
    gtk_tree_store_set(GTK_TREE_STORE(model), &liter,
        0, basename,
        1, NULL,
        2, FALSE,
        3, FALSE,
        -1);
    g_free(basename);
    if (file->tracks)
    {
        for (iter = g_list_first(file->tracks); iter; iter = g_list_next(iter))
        {
            interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
        }
    }
    if(file->routes)
    {
        for (iter = g_list_first(file->routes); iter; iter = g_list_next(iter))
        {
            interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
        }
    }
    interface_create_fake_master_track(file, &liter);
}


static void
view_state_changed (ChamplainView *view,
GParamSpec *gobject,
GtkImage *image)
{
    static guint sb_context = 0;
    ChamplainState state;
    GtkWidget *sb = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar2"));
    if(sb_context == 0)
    {
        sb_context = gtk_statusbar_get_context_id(GTK_STATUSBAR(sb), "loadingstatecontext");
    }
    g_object_get (G_OBJECT (view), "state", &state, NULL);
    if (state == CHAMPLAIN_STATE_LOADING)
    {
        gtk_statusbar_push(GTK_STATUSBAR(sb), sb_context, _("Loading map data.."));
        g_debug("STATE: loading.");
    }
    else
    {
        gtk_statusbar_pop(GTK_STATUSBAR(sb), sb_context);
        g_debug("STATE: done.");
    }
}


static void map_view_zoom_level_changed(GpxViewerMapView *view, int zoom_level, int min_level, int max_level, GtkWidget
*sp)
{
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(sp),
        (double)min_level,
        (double)max_level
        );
    if(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sp)) != zoom_level)
    {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)zoom_level);
    }
}


void map_selection_combo_changed_cb(GtkComboBox *box, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_combo_box_get_model(box);

    if(gtk_combo_box_get_active_iter(box, &iter))
    {
        gchar *id;
        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &id, -1);
        gpx_viewer_map_view_set_map_source(GPX_VIEWER_MAP_VIEW(champlain_view), id);
    }

}


void on_view_menu_settings_dock_toggled(GtkCheckMenuItem *item, gpointer data)
{
    gboolean active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
    if(GDL_DOCK_OBJECT_ATTACHED(dock_items[2]) != active)
    {
        if(active)
        {
            gdl_dock_item_show_item(GDL_DOCK_ITEM(dock_items[2]));
        }
        else
        {
            gdl_dock_item_hide_item(GDL_DOCK_ITEM(dock_items[2]));
        }
    }
}


void on_view_menu_track_info_dock_toggled(GtkCheckMenuItem *item, gpointer data)
{
    gboolean active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
    if(GDL_DOCK_OBJECT_ATTACHED(dock_items[1]) != active)
    {
        if(active)
        {
            gdl_dock_item_show_item(GDL_DOCK_ITEM(dock_items[1]));
        }
        else
        {
            gdl_dock_item_hide_item(GDL_DOCK_ITEM(dock_items[1]));
        }
    }
}


void on_view_menu_files_dock_toggled(GtkCheckMenuItem *item, gpointer data)
{
    gboolean active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
    if(GDL_DOCK_OBJECT_ATTACHED(dock_items[0]) != active)
    {
        if(active)
        {
            gdl_dock_item_show_item(GDL_DOCK_ITEM(dock_items[0]));
        }
        else
        {
            gdl_dock_item_hide_item(GDL_DOCK_ITEM(dock_items[0]));
        }
    }
}


static void dock_layout_changed(GdlDock *dock, gpointer data)
{
    GtkWidget *item;

    item = (GtkWidget *)gtk_builder_get_object(builder, "view_menu_settings_dock");
    if(GDL_DOCK_OBJECT_ATTACHED(dock_items[2]))
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    }
    else
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), FALSE);
    }
    item = (GtkWidget *)gtk_builder_get_object(builder, "view_menu_track_info_dock");
    if(GDL_DOCK_OBJECT_ATTACHED(dock_items[1]))
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    }
    else
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), FALSE);
    }
    item = (GtkWidget *)gtk_builder_get_object(builder, "view_menu_files_dock");
    if(GDL_DOCK_OBJECT_ATTACHED(dock_items[0]))
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    }
    else
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), FALSE);
    }
}


/* React when graph mode changes */
static void graph_mode_changed(GpxGraph *graph, GParamSpec *sp, gpointer data)
{
    int mode = gpx_graph_get_mode(graph);
    g_debug("Graph mode switched: %i\n", mode);
    switch(mode)
    {
        case GPX_GRAPH_GRAPH_MODE_ELEVATION:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(builder, "view_menu_elevation")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_SPEED:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(builder, "view_menu_speed")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_DISTANCE:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(builder, "view_menu_distance")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_ACCELERATION_H:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(builder, "view_menu_acceleration")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_SPEED_V:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(builder, "view_menu_vertical_speed")), TRUE);
            break;
        default:
            break;
    }

}


/* Create the interface */
static void create_interface(void)
{
    ChamplainView *view;
    GList *fiter,*iter;
    double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
    GError *error = NULL;
    GtkWidget *sp = NULL;
    gchar *path = g_build_filename(DATA_DIR, "gpx-viewer.ui", NULL);
    GtkTreeSelection *selection;
    GtkWidget *sw,*item,*rc;
    int current;
    int pos;
    gint w,h;
    GtkRecentFilter *grf;

    /* Open UI description file */
    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, path, &error))
    {
        g_error("Failed to create ui: %s\n", error->message);
    }
    g_free(path);

    item = gtk_menu_item_new_with_mnemonic(_("_Recent file"));
    gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(builder, "menu1")),
        item,1);
    rc = gtk_recent_chooser_menu_new();
    g_signal_connect(G_OBJECT(rc), "item-activated", G_CALLBACK(recent_chooser_file_picked), NULL);
    grf = gtk_recent_filter_new();
    gtk_recent_filter_add_pattern(GTK_RECENT_FILTER(grf), "*.gpx");
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(rc),grf);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), rc);

    w =gpx_viewer_preferences_get_integer(preferences,"Window", "width", 400);
    h =gpx_viewer_preferences_get_integer(preferences,"Window", "height", 300);
    gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(builder,"gpx_viewer_window")), w,h);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(builder, "TracksTreeView")));
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(routes_list_changed_cb), NULL);
    /* Create map view */
    champlain_view = (GtkWidget *)gpx_viewer_map_view_new();

    gtk_widget_set_size_request(champlain_view, 640, 280);
    sw = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(sw), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(sw), champlain_view);
    gtk_paned_pack1(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), sw, TRUE, TRUE);

    /* graph */
    gpx_graph = gpx_graph_new();
    gpx_graph_container = gtk_frame_new(NULL);
    gtk_widget_set_size_request(gpx_graph_container, -1, 120);
    gtk_frame_set_shadow_type(GTK_FRAME(gpx_graph_container), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(gpx_graph_container), GTK_WIDGET(gpx_graph));
    gtk_widget_show(GTK_WIDGET(gpx_graph));
    gtk_widget_set_no_show_all(GTK_WIDGET(gpx_graph_container), TRUE);
    gtk_paned_pack2(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), GTK_WIDGET(gpx_graph_container), FALSE, FALSE);

    /* show the interface */
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "gpx_viewer_window")));

    /* Set position */
    pos = gpx_viewer_preferences_get_integer(preferences,"Window", "main_view_pane_pos", 200);
    gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), pos);
    g_signal_connect( gtk_builder_get_object(builder, "main_view_pane"), "notify::position",
        G_CALLBACK(main_window_pane_pos_changed), NULL);
    /* Set position */
    pos = gpx_viewer_preferences_get_integer(preferences,"Window", "main_view_pane2_pos", 100);
    gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(builder, "main_view_hpane")), pos);

    g_signal_connect( gtk_builder_get_object(builder, "main_view_hpane"), "notify::position",
        G_CALLBACK(main_window_pane2_pos_changed), NULL);

    view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    g_signal_connect (view, "notify::state", G_CALLBACK (view_state_changed),
        NULL);

    interface_map_make_waypoints(view);

    for (fiter = g_list_first(files); fiter; fiter = g_list_next(fiter))
    {
        GpxFile *file = fiter->data;
        GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
        GtkTreeIter liter;
        gchar *basename = g_file_get_basename(file->file);
        gtk_tree_store_append(GTK_TREE_STORE(model), &liter, NULL);
        gtk_tree_store_set(GTK_TREE_STORE(model), &liter,
            0, basename,
            1, NULL,
            2, FALSE,
            3, FALSE,
            -1);
        g_free(basename);
        if (file->tracks)
        {
            for (iter = g_list_first(file->tracks); iter; iter = g_list_next(iter))
            {
                interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
            }
        }
        if(file->routes)
        {
            for (iter = g_list_first(file->routes); iter; iter = g_list_next(iter))
            {
                interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
            }
        }
        interface_create_fake_master_track(file, &liter);
    }
    /* Set up the zoom widget */
    sp = GTK_WIDGET(gtk_builder_get_object(builder, "map_zoom_level"));
    current = champlain_view_get_zoom_level(view);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)current);
    g_signal_connect(G_OBJECT(champlain_view), "zoom-level-changed", G_CALLBACK(map_view_zoom_level_changed), sp);

    /* Set up the smooth widget */
    sp = GTK_WIDGET(gtk_builder_get_object(builder, "smooth_factor"));
    gpx_viewer_preferences_add_object_property(preferences, G_OBJECT(gpx_graph), "smooth-factor");
    current = gpx_graph_get_smooth_factor(gpx_graph);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)current);
    g_signal_connect(gpx_graph, "notify::smooth-factor", G_CALLBACK(smooth_factor_changed), sp);

    /* */
    sp = GTK_WIDGET(gtk_builder_get_object(builder, "show_marker_layer"));
    g_signal_connect(G_OBJECT(champlain_view), "notify::show-waypoints", G_CALLBACK(show_marker_layer_changed), sp);
    show_marker_layer_changed(NULL, NULL, sp);

    g_signal_connect(gpx_graph, "point-clicked", G_CALLBACK(graph_point_clicked), NULL);
    g_signal_connect(gpx_graph, "selection-changed", G_CALLBACK(graph_selection_changed), NULL);

    /* Set up show points checkbox. Load state from config */
    sp = GTK_WIDGET(gtk_builder_get_object(builder, "graph_show_points"));
    g_signal_connect(gpx_graph, "notify::show-points", G_CALLBACK(graph_show_points_changed), sp);
    gpx_viewer_preferences_add_object_property(preferences, G_OBJECT(gpx_graph), "show-points");

    /**
     * Restore/Set graph mode
     */
    g_signal_connect(gpx_graph, "notify::mode", G_CALLBACK(graph_mode_changed), NULL);
    gpx_viewer_preferences_add_object_property(preferences, G_OBJECT(gpx_graph), "mode");

    /* Setup the map selector widget */
    {
        GtkWidget *combo = GTK_WIDGET(gtk_builder_get_object(builder, "map_selection_combo"));
        GtkCellRenderer *renderer = (GtkCellRenderer *)gtk_builder_get_object(builder, "cellrenderertext3");
        GtkTreeModel *model = gpx_viewer_map_view_get_model(GPX_VIEWER_MAP_VIEW(champlain_view));
        /* hack to work around GtkBuilder limitation that it cannot set expand
            on packing a cell renderer */
        g_object_ref(renderer);
        gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0,NULL);
        g_object_unref(renderer);

        gtk_combo_box_set_model(GTK_COMBO_BOX(gtk_builder_get_object(builder, "map_selection_combo")),model);
    }

    {
        GtkWidget *dock = gdl_dock_new();

        GtkWidget *flw = (GtkWidget *)gtk_builder_get_object(builder, "FileListWidget");
        GtkWidget *tiw = (GtkWidget *)gtk_builder_get_object(builder, "TrackInfoWidget");
        GtkWidget *swi = (GtkWidget *)gtk_builder_get_object(builder, "SettingWidget");

        /* Dock item */
        dock_items[0] = item = gdl_dock_item_new(
            "Files",
            "File and track list",
            GDL_DOCK_ITEM_BEH_CANT_CLOSE);
        gtk_container_add(GTK_CONTAINER(item), flw);
        gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(item), GDL_DOCK_TOP);
        gtk_widget_show(item);

        /* Dock item */
        dock_items[1] =     item = gdl_dock_item_new(
            "Information",
            "Detailed track information",
            GDL_DOCK_ITEM_BEH_CANT_CLOSE);
        gtk_container_add(GTK_CONTAINER(item), tiw);
        gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(item), GDL_DOCK_BOTTOM);
        gtk_widget_show(item);

        /* Dock item */
        dock_items[2] =item = gdl_dock_item_new(
            "Settings",
            "Map and graph settings",
            GDL_DOCK_ITEM_BEH_CANT_CLOSE);
        gtk_container_add(GTK_CONTAINER(item), swi);
        gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(item), GDL_DOCK_BOTTOM);
        gtk_widget_show(item);

        g_signal_connect(G_OBJECT(dock), "layout-changed", G_CALLBACK(dock_layout_changed), NULL);

        gtk_widget_show_all(dock);
        gtk_box_pack_end(GTK_BOX(gtk_builder_get_object(builder, "vbox1")), dock, TRUE, TRUE, 0);

        dock_layout = gdl_dock_layout_new(GDL_DOCK(dock));
        restore_layout();

    }
    gpx_viewer_preferences_add_object_property(preferences, G_OBJECT(champlain_view), "map-source");
    map_view_map_source_changed(GPX_VIEWER_MAP_VIEW(champlain_view), NULL,
        GTK_WIDGET(gtk_builder_get_object(builder, "map_selection_combo")));
    g_signal_connect(G_OBJECT(champlain_view),
        "notify::map-source",
        G_CALLBACK(map_view_map_source_changed),
        GTK_WIDGET(gtk_builder_get_object(builder, "map_selection_combo"))
        );

    /* Connect signals */
    gtk_builder_connect_signals(builder, NULL);

    /* Try to center the track on map correctly */
    if (lon1 < 1000.0 && lon2 < 1000.0)
    {
        champlain_view_set_zoom_level(view, 15);
        champlain_view_ensure_visible(view, lat1, lon1, lat2, lon2, FALSE);
    }
}


void open_gpx_file(GtkMenu *item)
{
    GtkWidget *dialog;
    GtkBuilder *fbuilder = gtk_builder_new();
    /* Show dialog */
    gchar *path = g_build_filename(DATA_DIR, "gpx-viewer-file-chooser.ui", NULL);
    if (!gtk_builder_add_from_file(fbuilder, path, NULL))
    {
        g_error("Failed to load gpx-viewer.ui");
    }
    g_free(path);

    dialog = GTK_WIDGET(gtk_builder_get_object(fbuilder, "gpx_viewer_file_chooser"));

    path = gpx_viewer_preferences_get_string(preferences, "open-dialog", "last-dir", NULL);
    if(path)
    {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),path);
        g_free(path); path = NULL;
    }
    /* update filter */
    {
        GtkFileFilter *filter =
            (GtkFileFilter *) gtk_builder_get_object(fbuilder, "gpx_viewer_file_chooser_filter");
        gtk_file_filter_add_pattern(filter, "*.gpx");

    }
    switch (gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        case 1:
        {
            GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
            GSList *iter, *choosen_files = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
            for (iter = choosen_files; iter; iter = g_slist_next(iter))
            {
                GpxFile *file;
                gchar *basename;
                GtkTreeIter liter;
                double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
                /* Create a GFile */
                GFile *afile = g_file_new_for_uri((gchar*)iter->data);
                /* Add entry to recent manager */
                gtk_recent_manager_add_item(GTK_RECENT_MANAGER(recent_man), (gchar *)iter->data);
                /* Try to open the gpx file */
                file = gpx_file_new(afile);
                files = g_list_append(files, file);
                g_object_unref(afile);

                basename = g_file_get_basename(file->file);
                gtk_tree_store_append(GTK_TREE_STORE(model), &liter, NULL);
                gtk_tree_store_set(GTK_TREE_STORE(model), &liter,
                    0, basename,
                    1, NULL,
                    2, FALSE,
                    3, FALSE,
                    -1);
                g_free(basename);
                if (file->tracks)
                {
                    GList *track_iter;
                    for (track_iter = g_list_first(file->tracks); track_iter; track_iter = g_list_next(track_iter))
                    {
                        interface_plot_add_track(&liter, track_iter->data, &lat1, &lon1, &lat2, &lon2);
                    }
                }
                if(file->routes)
                {
                    GList *route_iter;
                    for (route_iter = g_list_first(file->routes); route_iter; route_iter = g_list_next(route_iter))
                    {
                        interface_plot_add_track(&liter, route_iter->data, &lat1, &lon1, &lat2, &lon2);
                    }
                }
                interface_create_fake_master_track(file, &liter);
            }
            g_slist_foreach(choosen_files, (GFunc) g_free, NULL);
            g_slist_free(choosen_files);
        }
        default:
            break;
    }
    path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
    if(path)
    {
        gpx_viewer_preferences_set_string(preferences, "open-dialog" , "last-dir" , path);
        g_free(path);
    }
    gtk_widget_destroy(dialog);
    g_object_unref(fbuilder);
}


/**
 * IPC: Handle commands gpx-viewer gets via UniqueApp.
 */
static UniqueResponse unique_response(UniqueApp *uapp, gint command, UniqueMessageData *data, guint time_)
{
    switch(command)
    {
        case OPEN_URIS:
        {
            GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
            int i;
            gchar ** uris = unique_message_data_get_uris(data);
            g_debug("Asked to open uris\n");
            for (i = 0; uris != NULL && uris[i] != NULL ; i++)
            {
                GpxFile *file;
                GFile *afile = g_file_new_for_commandline_arg(uris[i]);
                gchar *basename;
                GtkTreeIter liter;
                double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
                /* Create a GFile */
                /* Try to open the gpx file */
                file = gpx_file_new(afile);
                files = g_list_append(files, file);
                g_object_unref(afile);
                /* Add entry to recent manager */
                gtk_recent_manager_add_item(GTK_RECENT_MANAGER(recent_man), (gchar *)file->file);

                basename = g_file_get_basename(file->file);
                gtk_tree_store_append(GTK_TREE_STORE(model), &liter, NULL);
                gtk_tree_store_set(GTK_TREE_STORE(model), &liter,
                    0, basename,
                    1, NULL,
                    2, FALSE,
                    3, FALSE,
                    -1);
                g_free(basename);
                if (file->tracks)
                {
                    GList *track_iter;
                    for (track_iter = g_list_first(file->tracks); track_iter; track_iter = g_list_next(track_iter))
                    {
                        interface_plot_add_track(&liter, track_iter->data, &lat1, &lon1, &lat2, &lon2);
                    }
                }
                if(file->routes)
                {
                    GList *route_iter;
                    for (route_iter = g_list_first(file->routes); route_iter; route_iter = g_list_next(route_iter))
                    {
                        interface_plot_add_track(&liter, route_iter->data, &lat1, &lon1, &lat2, &lon2);
                    }
                }
                interface_create_fake_master_track(file, &liter);
            }
            g_strfreev(uris);
            return UNIQUE_RESPONSE_OK;
        }
        default:
            break;
    }
    return UNIQUE_RESPONSE_INVALID;
}


int main(int argc, char **argv)
{
    int i = 0;
    gchar *website;
    GOptionContext *context = NULL;
    GError *error = NULL;
    gchar *path;

    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);

    context = g_option_context_new(_("[FILE...] - GPX Viewer"));

    g_option_context_set_summary(context, N_("A simple program to visualize one or more gpx files."));

    website = g_strconcat(N_("Website: "), PACKAGE_URL, NULL);
    g_option_context_set_description(context, website);
    g_free(website);

    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if (error)
    {
        g_log(NULL, G_LOG_LEVEL_ERROR, "Failed to parse commandline options: %s", error->message);
        g_error_free(error);
        error = NULL;
    }
    if(!g_thread_supported())
    {
        g_thread_init(NULL);
    }

    gtk_clutter_init(&argc, &argv);

    app = unique_app_new("nl.Sarine.gpx-viewer",NULL);
    unique_app_add_command(app,"open-uris", OPEN_URIS);
    g_signal_connect(G_OBJECT(app),"message-received", G_CALLBACK(unique_response), NULL);

    printf(" %i\n", unique_app_is_running(app));
    /* Why does it say always running? */
    if(unique_app_is_running(app))
    {
        /* Program is allready running */
        UniqueMessageData *msd = unique_message_data_new();
        g_debug("Program is allready running\n");
        if(argc > 1)
        {
            printf("uri: %s\n", argv[1]);
            unique_message_data_set_uris(msd,&argv[1]);
            unique_app_send_message(app,1, msd);
        }
        /* free msd? */
        unique_message_data_free(msd);
        /* Free app */
        g_object_unref(app);
        return EXIT_SUCCESS;
    }
    /* Setup responding to commands */

    preferences = gpx_viewer_preferences_new();

    /* REcent manager */
    recent_man = gtk_recent_manager_get_default();

    /* Add own icon strucutre to the theme engine search */
    path = g_build_filename(DATA_DIR, "icons", NULL);
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
        path);
    g_free(path);

    /* Open all the files given on the command line */
    for (i = 1; i < argc; i++)
    {
        GpxFile *file;
        GFile *afile = g_file_new_for_commandline_arg(argv[i]);
        gchar *uri = g_file_get_uri(afile);

        /* Try to open the gpx file */
        gtk_recent_manager_add_item(GTK_RECENT_MANAGER(recent_man), uri);
        file = gpx_file_new(afile);
        files = g_list_prepend(files, file);

        g_free(uri);
        g_object_unref(afile);
    }
    files = g_list_reverse(files);

    playback = gpx_playback_new(NULL);

    gpx_viewer_preferences_add_object_property(preferences, G_OBJECT(playback), "speedup");
    g_signal_connect(GPX_PLAYBACK(playback), "tick", G_CALLBACK(route_playback_tick), NULL);
    g_signal_connect(GPX_PLAYBACK(playback), "state-changed", G_CALLBACK(route_playback_state_changed), NULL);

    gtk_init_add((GtkFunction)create_interface,NULL);

    gtk_main();

    g_object_unref(playback);

    /* Destroy the files */
    g_debug("Cleaning up files");
    g_list_foreach(g_list_first(files), (GFunc) g_object_unref, NULL);
    g_list_free(files);

    g_object_unref(preferences);

    return EXIT_SUCCESS;
}


/**
 * Track list viewer
 */

void close_show_current_track(GtkWidget *widget,gint response_id, GtkBuilder *fbuilder)
{

    GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(fbuilder, "track_list_dialog"));
    gtk_widget_destroy(dialog);

    g_object_unref(fbuilder);
}


void show_current_track(void)
{
    if(active_route && active_route->track)
    {
        GtkWidget *dialog;
        GtkTreeView *tree;
        GtkTreeModel *model = (GtkTreeModel *)gpx_track_tree_model_new(active_route->track);
        GtkBuilder *fbuilder = gtk_builder_new();
        /* Show dialog */
        gchar *path = g_build_filename(DATA_DIR, "gpx-viewer-tracklist.ui", NULL);
        if (!gtk_builder_add_from_file(fbuilder, path, NULL))
        {
            g_error("Failed to load gpx-viewer.ui");
        }
        g_free(path);

        dialog = GTK_WIDGET(gtk_builder_get_object(fbuilder, "track_list_dialog"));
        tree = GTK_TREE_VIEW(gtk_builder_get_object(fbuilder, "treeview"));

        gtk_tree_view_set_model(tree, model);
        g_object_unref(model);
        gtk_builder_connect_signals(fbuilder, fbuilder);
        gtk_widget_show(GTK_WIDGET(dialog));
    }
}


/**
 * Preferences
 */
void gpx_viewer_preferences_close(GtkWidget *dialog, gint respose, GtkBuilder *fbuilder)
{
    gtk_widget_destroy(dialog);
    g_object_unref(fbuilder);
}


void playback_speedup_spinbutton_value_changed_cb(GtkSpinButton *sp)
{
    gint value = gtk_spin_button_get_value_as_int(sp);
    gpx_playback_set_speedup(playback, value);
}


void gpx_viewer_show_preferences_dialog(void)
{
    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    GtkWidget *dialog;
    GtkTreeModel *model;
    GtkWidget *widget;
    GtkBuilder *fbuilder = gtk_builder_new();
    /* Show dialog */
    gchar *path = g_build_filename(DATA_DIR, "gpx-viewer-preferences.ui", NULL);
    if (!gtk_builder_add_from_file(fbuilder, path, NULL))
    {
        g_error("Failed to load gpx-viewer-preferences.ui");
    }
    g_free(path);
    dialog = GTK_WIDGET(gtk_builder_get_object(fbuilder, "preferences_dialog"));
    model = gpx_viewer_map_view_get_model(GPX_VIEWER_MAP_VIEW(champlain_view));
    /**
     * Setup map selection widget
     */
    widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"map_source_combobox");
    gtk_combo_box_set_model(GTK_COMBO_BOX(widget), model);
    g_signal_connect_object(G_OBJECT(champlain_view),
        "notify::map-source",
        G_CALLBACK(map_view_map_source_changed),
        widget,
        0
        );
    map_view_map_source_changed(GPX_VIEWER_MAP_VIEW(champlain_view), NULL, widget);
    /* TODO */
    /* to sync this, we need to create a wrapper around the gpx-graph that nicely has these
        properties. */

    /* Zoom level */

    widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"spin_button_zoom_level");
    g_signal_connect_object(G_OBJECT(champlain_view), "zoom-level-changed", G_CALLBACK(map_view_zoom_level_changed),
        widget,0);
    map_view_zoom_level_changed(GPX_VIEWER_MAP_VIEW(champlain_view),
        champlain_view_get_zoom_level(view),
        champlain_view_get_min_zoom_level(view),
        champlain_view_get_max_zoom_level(view),
        widget);

    /* Show Waypoints */
    widget = GTK_WIDGET(gtk_builder_get_object(fbuilder, "check_button_show_waypoints"));
    g_signal_connect_object(G_OBJECT(champlain_view), "notify::show-waypoints", G_CALLBACK(show_marker_layer_changed),
        widget,0);
    show_marker_layer_changed(NULL, NULL, widget);

    /** Graph **/
    /* smooth factor */
    widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"spin_button_smooth_factor");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), (double)gpx_graph_get_smooth_factor(gpx_graph));
    g_signal_connect_object(gpx_graph, "notify::smooth-factor", G_CALLBACK(smooth_factor_changed), widget,0);

    /* Show points */
    widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"check_button_data_points");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), gpx_graph_get_show_points(gpx_graph));
    g_signal_connect_object(gpx_graph, "notify::show-points", G_CALLBACK(graph_show_points_changed), widget,0);
    /* sppedup */
    widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"playback_speedup_spinbutton");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
        (double)gpx_playback_get_speedup(playback));

    gtk_builder_connect_signals(fbuilder, fbuilder);
    gtk_widget_show(GTK_WIDGET(dialog));
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

/* Gpx Viewer 
 * Copyright (C) 2009-2009 Qball Cow <qball@sarine.nl>
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <champlain/champlain.h>
#include <champlain-gtk/champlain-gtk.h>
#include <clutter-gtk/clutter-gtk.h>
#include "gpx.h"


static GKeyFile *config_file		= NULL;

/* List of gpx files */
GList *files                        = NULL;
GtkWidget *champlain_view           = NULL;
GtkBuilder *builder                 = NULL;
GpxGraph *gpx_graph                 = NULL;
GtkWidget *gpx_graph_container		= NULL;
ChamplainLayer *waypoint_layer		= NULL;
ChamplainLayer *marker_layer        = NULL;


/* List of routes */
GList *routes                       = NULL;

/* Clutter  collors */
ClutterColor waypoint               = { 0xf3, 0x94, 0x07, 0xff };
ClutterColor highlight_track_color  = { 0xf3, 0x94, 0x07, 0xff };
ClutterColor normal_track_color     = { 0x00, 0x00, 0xff, 0x66 };

typedef struct Route {
    GpxFile *file;
    GpxTrack *track;
    ChamplainPolygon *polygon;
	ChamplainBaseMarker *start;
	ChamplainBaseMarker *stop;
    gboolean visible;
	GpxPlayback *playback;
} Route;

/* The currently active route */
Route *active_route                 = NULL;



int config_get_integer(const char *a, const char *b, int def)
{
	GError *error = NULL;
	int retv = g_key_file_get_integer(config_file,a,b, &error);
	if(error) {
		g_debug("Failed to get value: %s", error->message);
		g_error_free(error); error = NULL;
		return def;
	}
	return retv;
}

static void free_Route(Route *route)
{
	if(route->polygon) g_object_unref(route->polygon);
	if(route->playback) g_object_unref(route->playback);
    g_free(route);
}

void on_destroy(void)
{
    printf("Quit...\n");
    gtk_main_quit();

    gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(builder, "gpx_viewer_window")));
    g_object_unref(builder);

    g_list_foreach(g_list_first(routes), (GFunc)free_Route, NULL);
    g_list_free(routes); routes = NULL;
}

/**
 * Update on track changes
 */
static void interface_update_heading(GtkBuilder * builder, GpxTrack * track, GpxPoint *start, GpxPoint *stop)
{
    time_t temp;
    gdouble gtemp;
	double max_speed = 0;
	double points = 0;
    GtkWidget *label = NULL;
    /* Duration */
    label = (GtkWidget *) gtk_builder_get_object(builder, "duration_label");

    temp = 0;
	if(start && stop) {
		temp = gpx_point_get_time(stop) - gpx_point_get_time(start);
	}
    if (temp > 0) {
        int hour = temp / 3600;
        int minutes = ((temp % 3600) / 60);
        int seconds = (temp % 60);
        GString *string = g_string_new("");
        if (hour > 0) {
            g_string_append_printf(string, "%i %s", hour, (hour == 1) ? "hour" : "hours");
        }

        if (minutes > 0) {
            if (hour > 0)
                g_string_append(string, ", ");
            g_string_append_printf(string, "%i %s", minutes, (minutes == 1) ? "minute" : "minutes");
        }

        if (seconds > 0) {
            if (minutes > 0)
                g_string_append(string, ", ");
            g_string_append_printf(string, "%i %s", seconds, (seconds == 1) ? "second" : "seconds");
        }

        gtk_label_set_text(GTK_LABEL(label), string->str);
        g_string_free(string, TRUE);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }
    /* Distance */
    label = (GtkWidget *) gtk_builder_get_object(builder, "distance_label");

    gtemp = 0;
	if(start && stop) gtemp = stop->distance-start->distance;
	if (gtemp > 0) {
        gchar *string = g_strdup_printf("%.2f km", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Average */
    label = (GtkWidget *) gtk_builder_get_object(builder, "average_label");
    gtemp = 0;
	if(start && stop) gtemp = gpx_track_calculate_point_to_point_speed(track,start, stop);
    if (gtemp > 0) {
        gchar *string = g_strdup_printf("%.2f km/h", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Moving Average */
    label = (GtkWidget *) gtk_builder_get_object(builder, "moving_average_label");
    gtemp = 0;
	temp = 0;
	if(start && stop) gtemp = gpx_track_calculate_moving_average(track,start, stop, &temp);
    if (gtemp > 0) {
        gchar *string = g_strdup_printf("%.2f km/h", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    label = (GtkWidget *) gtk_builder_get_object(builder, "moving_average_time_label");
    if (gtemp > 0) {
        int hour = temp / 3600;
        int minutes = ((temp % 3600) / 60);
        int seconds = (temp % 60);
        GString *string = g_string_new("");
        if (hour > 0) {
            g_string_append_printf(string, "%i %s", hour, (hour == 1) ? "hour" : "hours");
        }

        if (minutes > 0) {
            if (hour > 0)
                g_string_append(string, ", ");
            g_string_append_printf(string, "%i %s", minutes, (minutes == 1) ? "minute" : "minutes");
        }

        if (seconds > 0) {
            if (minutes > 0)
                g_string_append(string, ", ");
            g_string_append_printf(string, "%i %s", seconds, (seconds == 1) ? "second" : "seconds");
        }

        gtk_label_set_text(GTK_LABEL(label), string->str);
        g_string_free(string, TRUE);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }
    /* Max speed */
	if(track && start && stop)
	{
		GList *list ;
		for(list = g_list_find(track->points, start); list && list->data != stop; list = g_list_next(list)){
			points++;
			max_speed = MAX(max_speed, ((GpxPoint *)list->data)->speed);
		}
		points++;

	}
    label = (GtkWidget *) gtk_builder_get_object(builder, "max_speed_label");
    if (max_speed > 0) {
        gchar *string = g_strdup_printf("%.2f km/h", max_speed);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

}

static void interface_map_plot_route(ChamplainView * view, struct Route *route)
{
    route->polygon = champlain_polygon_new();
    for (GList *iter = g_list_first(route->track->points); iter; iter = iter->next) {
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
	for(GList *it = g_list_first(file->waypoints); it; it = g_list_next(it))
	{
		GpxPoint *p = it->data;
		const gchar *name = gpx_point_get_name(p);
		ClutterActor *marker = champlain_marker_new_with_text(name, "Seric 12", NULL, NULL);
		champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(marker), p->lat_dec, p->lon_dec);
		champlain_marker_set_color(CHAMPLAIN_MARKER(marker), &waypoint);
		clutter_container_add(CLUTTER_CONTAINER(waypoint_layer), CLUTTER_ACTOR(marker), NULL);
	}
}

static void interface_map_make_waypoints(ChamplainView * view)
{
    GList *iter;
    if (waypoint_layer == NULL) {
        waypoint_layer = champlain_layer_new();
        champlain_view_add_layer(view, waypoint_layer);
    }
    for (iter = g_list_first(files); iter != NULL; iter = g_list_next(iter)) {
		GpxFile *file = iter->data;
		interface_map_file_waypoints(view, file);
	}
	clutter_actor_show(CLUTTER_ACTOR(waypoint_layer));
}

/* Show and hide waypoint layer */
void show_marker_layer_toggled_cb(GtkToggleButton * button, gpointer user_data)
{
    if (waypoint_layer) {
        gboolean active = gtk_toggle_button_get_active(button);
        if (active) {
            clutter_actor_show_all(CLUTTER_ACTOR(waypoint_layer));
        } else {
            clutter_actor_hide(CLUTTER_ACTOR(waypoint_layer));
        }
    }
}

void routes_list_changed_cb(GtkTreeSelection * sel, gpointer user_data)
{
	GtkTreeView *tree = gtk_tree_selection_get_tree_view(sel);
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(sel, &model, &iter))
	{
		Route *route = NULL;
		gtk_tree_model_get(model, &iter, 1, &route, -1);
		if (active_route) {
			champlain_polygon_set_stroke_color(active_route->polygon, &normal_track_color);
			if(active_route->stop) 
				clutter_actor_hide(CLUTTER_ACTOR(active_route->stop));

			if(active_route->start) 
				clutter_actor_hide(CLUTTER_ACTOR(active_route->start));

			gpx_playback_stop(active_route->playback);
			printf("hide\n");
			gpx_graph_set_track(gpx_graph, NULL);
			gtk_widget_hide(GTK_WIDGET(gpx_graph_container));
		}

		active_route = route;
		if (route) {
			ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
			champlain_polygon_set_stroke_color(route->polygon, &highlight_track_color);

			if (route->visible)
				champlain_polygon_show(route->polygon);
			if (route->track->top && route->track->bottom) {
				champlain_view_ensure_visible(view,
						route->track->top->lat_dec, route->track->top->lon_dec,
						route->track->bottom->lat_dec, route->track->bottom->lon_dec, FALSE);
			}

			if (gpx_track_get_total_time(route->track) > 5) {
				gpx_graph_set_track(gpx_graph, route->track);
				gtk_widget_show(GTK_WIDGET(gpx_graph_container));
			} else {
				gpx_graph_set_track(gpx_graph, NULL);
				gtk_widget_hide(GTK_WIDGET(gpx_graph_container));
			}

			if(route->stop) 
				clutter_actor_show(CLUTTER_ACTOR(route->stop));

			if(route->start) 
				clutter_actor_show(CLUTTER_ACTOR(route->start));
		}
	}
}

/* Smooth factor changed */
static void smooth_factor_changed(GpxGraph * graph, GParamSpec * gobject, GtkSpinButton * spinbutton)
{
    gint zoom;
    g_object_get(G_OBJECT(graph), "smooth-factor", &zoom, NULL);
    gtk_spin_button_set_value(spinbutton, zoom);
	g_key_file_set_integer(config_file, "Window", "smooth-factor", zoom);
}

void smooth_factor_change_value_cb(GtkSpinButton * spin, gpointer user_data)
{
    int current = gpx_graph_get_smooth_factor(gpx_graph);
    int new = gtk_spin_button_get_value_as_int(spin);
    if (current != new) {
        gpx_graph_set_smooth_factor(gpx_graph, new);
		g_key_file_set_integer(config_file, "Window", "smooth-factor", new);
	}
}

/* Zoom level changed */
static void map_zoom_changed(ChamplainView * view, GParamSpec * gobject, GtkSpinButton * spinbutton)
{
    gint zoom;
    g_object_get(G_OBJECT(view), "zoom-level", &zoom, NULL);
    gtk_spin_button_set_value(spinbutton, zoom);
}

void map_zoom_level_change_value_cb(GtkSpinButton * spin, gpointer user_data)
{
    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    int current = champlain_view_get_zoom_level(view);
    int new = gtk_spin_button_get_value_as_int(spin);
    if (current != new) {
        champlain_view_set_zoom_level(view, new);
    }
}

ClutterActor *click_marker = NULL;
guint click_marker_source = 0;
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
		if(start) {
			champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(active_route->start), start->lat_dec, start->lon_dec);
		}

		if(stop) {
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


		if (ii) {
			const gchar *path2 = gtk_icon_info_get_filename(ii);
			if (path2) {
				click_marker = champlain_marker_new_from_file(path2, NULL);
				champlain_marker_set_draw_background(CHAMPLAIN_MARKER(click_marker), FALSE);
			}
		}
		if (!click_marker) {
			click_marker = champlain_marker_new();
		}
		/* Create the marker */
		champlain_marker_set_color(CHAMPLAIN_MARKER(click_marker), &waypoint);
		clutter_container_add(CLUTTER_CONTAINER(marker_layer), CLUTTER_ACTOR(click_marker), NULL);
	}

	champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(click_marker), point->lat_dec, point->lon_dec);
	clutter_actor_show(CLUTTER_ACTOR(click_marker));

	if(click_marker_source >0) {
		g_source_remove(click_marker_source);
	}

	marker[0] =(ChamplainBaseMarker *) click_marker;
	champlain_view_ensure_markers_visible(view, marker, FALSE);

    click_marker_source = g_timeout_add_seconds(5, (GSourceFunc) graph_point_remove, click_marker);
}
void playback_play_clicked(void)
{
	if(active_route) {
			gpx_playback_start(active_route->playback);
	}
}

void playback_pause_clicked(void)
{
	if(active_route) {
			gpx_playback_pause(active_route->playback);
	}
}
void playback_stop_clicked(void)
{
	if(active_route) {
			gpx_playback_stop(active_route->playback);
	}
}
static void route_playback_tick(GpxPlayback *playback, GpxPoint *current)
{
	if(current != NULL){
		time_t time = gpx_point_get_time(current);
		gpx_graph_set_highlight(gpx_graph, time);
		graph_point_clicked(gpx_graph, current);
	}
	else{
		time_t time = 0;
		gpx_graph_set_highlight(gpx_graph, time);
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
	route->track = track;
	route->visible = first;

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
			-1);

	if(first){
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(builder, "TracksTreeView")));
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &liter);
		if(path != NULL){
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
		if (ii) {
			const gchar *path2 = gtk_icon_info_get_filename(ii);
			if (path2) {
				route->start = (ChamplainBaseMarker *)champlain_marker_new_from_file(path2, NULL);
				champlain_marker_set_draw_background(CHAMPLAIN_MARKER(route->start), FALSE);
			}
			gtk_icon_info_free(ii);
		}
		if (!route->start) {
			route->start = (ChamplainBaseMarker *)champlain_marker_new();
		}
		/* Create the marker */
		champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(route->start), 
				((GpxPoint*)start->data)->lat_dec, 
				((GpxPoint*)start->data)->lon_dec);
		champlain_marker_set_color(CHAMPLAIN_MARKER(route->start), &waypoint);
		clutter_container_add(CLUTTER_CONTAINER(marker_layer), CLUTTER_ACTOR(route->start), NULL);

		ii = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
				"pin-blue",
				100, 0);
		if (ii) {
			const gchar *path2 = gtk_icon_info_get_filename(ii);
			if (path2) {
				route->stop =  (ChamplainBaseMarker *)champlain_marker_new_from_file(path2, NULL);
				champlain_marker_set_draw_background(CHAMPLAIN_MARKER(route->stop), FALSE);
			}
			gtk_icon_info_free(ii);
		}
		if (!route->stop) {
			route->stop = (ChamplainBaseMarker *)champlain_marker_new();
		}
		/* Create the marker */
		champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(route->stop), 
				((GpxPoint*)stop->data)->lat_dec, 
				((GpxPoint*)stop->data)->lon_dec);
		champlain_marker_set_color(CHAMPLAIN_MARKER(route->stop), &waypoint);
		clutter_container_add(CLUTTER_CONTAINER(marker_layer), CLUTTER_ACTOR(route->stop), NULL);

		clutter_actor_hide(CLUTTER_ACTOR(route->stop));
		clutter_actor_hide(CLUTTER_ACTOR(route->start));
		}
	}

	route->playback = gpx_playback_new(route->track);
	g_signal_connect(GPX_PLAYBACK(route->playback), "tick", G_CALLBACK(route_playback_tick), NULL);
	routes = g_list_append(routes, route);
}

static void main_window_pane_pos_changed(GtkWidget * panel, GParamSpec * arg1, gpointer data)
{
	gint position = 0;
	g_object_get(G_OBJECT(panel), "position", &position, NULL);
	g_key_file_set_integer(config_file, "Window", "main_view_pane_pos", position);
	g_debug("Position: %i\n", position);
}


static void main_window_pane2_pos_changed(GtkWidget * panel, GParamSpec * arg1, gpointer data)
{
	gint position = 0;
	g_object_get(G_OBJECT(panel), "position", &position, NULL);
	g_key_file_set_integer(config_file, "Window", "main_view_pane2_pos", position);
	g_debug("Position2: %i\n", position);
}

void main_window_size_changed(GtkWindow *win, GtkAllocation *alloc, gpointer data)
{
	if(alloc)
	{
		g_key_file_set_integer(config_file, "Window", "width", alloc->width);
		g_key_file_set_integer(config_file, "Window", "height", alloc->height);
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
            if (active) {
                champlain_polygon_show(route->polygon);
            } else {
                champlain_polygon_hide(route->polygon);
            }
		}
	}
}

void show_elevation(GtkMenuItem item, gpointer user_data)
{
	printf("switch to elevation\n");
	gpx_graph_switch_mode(gpx_graph, GPX_GRAPH_GRAPH_MODE_ELEVATION);
	g_key_file_set_integer(config_file, "Graph", "GraphMode", GPX_GRAPH_GRAPH_MODE_ELEVATION);
}
void show_speed(GtkMenuItem item, gpointer user_data)
{
	printf("switch to speed\n");
	gpx_graph_switch_mode(gpx_graph, GPX_GRAPH_GRAPH_MODE_SPEED);
	g_key_file_set_integer(config_file, "Graph", "GraphMode", GPX_GRAPH_GRAPH_MODE_SPEED);
}


/* Create the interface */
static void create_interface(void)
{
    double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
    GError *error = NULL;
    GtkWidget *sp = NULL;
    gchar *path = g_build_filename(DATA_DIR, "gpx-viewer.ui", NULL);
	GtkTreeSelection *selection;
	GtkWidget *sw;
    int current;
	int pos;
	gint w,h;

	/* Open UI description file */
    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, path, NULL)) {
        g_error("Failed to create ui: %s\n", error->message);
    }
    g_free(path);
	
	w =config_get_integer("Window", "width", 400); 
	h =config_get_integer("Window", "height", 300); 
	gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(builder,"gpx_viewer_window")), w,h); 


	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(builder, "TracksTreeView")));
	g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(routes_list_changed_cb), NULL);
    /* Create map view */
    champlain_view = gtk_champlain_embed_new();
    gtk_widget_set_size_request(champlain_view, 640, 280);
	sw = gtk_frame_new(NULL); 
	gtk_frame_set_shadow_type(GTK_FRAME(sw), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(sw), champlain_view);
    gtk_paned_pack1(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), sw, TRUE, TRUE);

    /* graph */
    gpx_graph = gpx_graph_new();
	gpx_graph_container = gtk_frame_new(NULL); 
	gtk_frame_set_shadow_type(GTK_FRAME(gpx_graph_container), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(gpx_graph_container), GTK_WIDGET(gpx_graph));
	gtk_widget_show(gpx_graph);
	gtk_widget_set_no_show_all(GTK_WIDGET(gpx_graph_container), TRUE);
    gtk_paned_pack2(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), GTK_WIDGET(gpx_graph_container), FALSE, TRUE);

    /* show the interface */
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "gpx_viewer_window")));

	/* Set position */
	pos = config_get_integer("Window", "main_view_pane_pos", 200); 
	gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), pos);
	g_signal_connect( gtk_builder_get_object(builder, "main_view_pane"), "notify::position",
			G_CALLBACK(main_window_pane_pos_changed), NULL);
	/* Set position */
	pos = config_get_integer("Window", "main_view_pane2_pos", 100); 
	gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(builder, "main_view_hpane")), pos);


	g_signal_connect( gtk_builder_get_object(builder, "main_view_hpane"), "notify::position",
			G_CALLBACK(main_window_pane2_pos_changed), NULL);
    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    g_object_set(G_OBJECT(view), "scroll-mode", CHAMPLAIN_SCROLL_MODE_KINETIC, "zoom-level", 5, NULL);


	champlain_view_set_show_scale(CHAMPLAIN_VIEW(view), TRUE);

	if (marker_layer == NULL) {
		marker_layer = champlain_layer_new();
		champlain_view_add_layer(view, marker_layer);
	}

    interface_map_make_waypoints(view);

    for (GList *fiter = g_list_first(files); fiter; fiter = g_list_next(fiter)) {
	    GpxFile *file = fiter->data;
	    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
	    GtkTreeIter liter;
	    gchar *basename = g_path_get_basename(file->filename);
	    gtk_tree_store_append(GTK_TREE_STORE(model), &liter, NULL);
	    gtk_tree_store_set(GTK_TREE_STORE(model), &liter, 
			    0, basename, 
			    1, NULL,
			    2, FALSE,
			    3, FALSE,
			    -1);
	    g_free(basename);
	    if (file->tracks) {
		    for (GList *iter = g_list_first(file->tracks); iter; iter = g_list_next(iter)) {
			    interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
		    }
	    }
	    if(file->routes) {
		    for (GList *iter = g_list_first(file->routes); iter; iter = g_list_next(iter)) {
			    interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
		    }
	    }
    }
	/* Set up the zoom widget */
    sp = GTK_WIDGET(gtk_builder_get_object(builder, "map_zoom_level"));
    champlain_view_set_min_zoom_level(view, 1);
    champlain_view_set_max_zoom_level(view, 18);
    current = champlain_view_get_zoom_level(view);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)current);


	g_signal_connect(view, "notify::zoom-level", G_CALLBACK(map_zoom_changed), sp);
    /* Set up the smooth widget */
    sp = GTK_WIDGET(gtk_builder_get_object(builder, "smooth_factor"));
    current = gpx_graph_get_smooth_factor(gpx_graph);
	pos = config_get_integer("Window", "smooth-factor", current);
	if(pos != current) gpx_graph_set_smooth_factor(gpx_graph, pos);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)pos);

    g_signal_connect(gpx_graph, "notify::smooth-factor", G_CALLBACK(smooth_factor_changed), sp);
    g_signal_connect(gpx_graph, "point-clicked", G_CALLBACK(graph_point_clicked), NULL);
    g_signal_connect(gpx_graph, "selection-changed", G_CALLBACK(graph_selection_changed), NULL);

	/** 
	 * Restore/Set graph mode 
	 */
	pos = config_get_integer("Graph", "GraphMode", 0);
	gpx_graph_switch_mode(gpx_graph, pos);
	switch(pos){
		case GPX_GRAPH_GRAPH_MODE_ELEVATION:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
					gtk_builder_get_object(builder, "view_menu_elevation")), TRUE);
			break;
		default:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
					gtk_builder_get_object(builder, "view_menu_speed")), TRUE);
	}

	/* Connect signals */
    gtk_builder_connect_signals(builder, NULL);
    /* Try to center the track on map correctly */
    if (lon1 < 1000.0 && lon2 < 1000.0) {
		printf("zoom\n");
        champlain_view_set_zoom_level(view, 9);
        champlain_view_ensure_visible(view, lat1, lon1, lat2, lon2, FALSE);
    }
}

void open_gpx_file(GtkMenu *item)
{
	GtkWidget *dialog;
	GtkBuilder *fbuilder = gtk_builder_new();
	/* Show dialog */
	gchar *path = g_build_filename(DATA_DIR, "gpx-viewer-file-chooser.ui", NULL);
	if (!gtk_builder_add_from_file(fbuilder, path, NULL)) {
		g_error("Failed to load gpx-viewer.ui");
	}
	g_free(path);

	dialog = GTK_WIDGET(gtk_builder_get_object(fbuilder, "gpx_viewer_file_chooser"));

	path = g_key_file_get_string(config_file, "open-dialog", "last-dir", NULL);
	if(path) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),path);
		g_free(path); path = NULL;
	}
	/* update filter */
	{
		GtkFileFilter *filter =
			(GtkFileFilter *) gtk_builder_get_object(fbuilder, "gpx_viewer_file_chooser_filter");
		gtk_file_filter_add_pattern(filter, "*.gpx");

	}
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
		case 1:
			{
				GSList *iter, *choosen_files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
				for (iter = choosen_files; iter; iter = g_slist_next(iter)) {
					double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
					/* Try to open the gpx file */
					GpxFile *file = gpx_file_new((gchar *) iter->data);
					files = g_list_append(files, file);

					GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
					GtkTreeIter liter;
					gchar *basename = g_path_get_basename(file->filename);
					gtk_tree_store_append(GTK_TREE_STORE(model), &liter, NULL);
					gtk_tree_store_set(GTK_TREE_STORE(model), &liter, 
							0, basename, 
							1, NULL,
							2, FALSE,
							3, FALSE,
							-1);
					g_free(basename);
					if (file->tracks) {
						for (GList *iter = g_list_first(file->tracks); iter; iter = g_list_next(iter)) {
							interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
						}
					}
					if(file->routes) {
						for (GList *iter = g_list_first(file->routes); iter; iter = g_list_next(iter)) {
							interface_plot_add_track(&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
						}
					}
				}
				g_slist_foreach(choosen_files, (GFunc) g_free, NULL);
				g_slist_free(choosen_files);
			}
			break;
	}
	path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
	if(path) {
		g_key_file_set_string(config_file, "open-dialog" , "last-dir" , path);	
		g_free(path);
	}
	gtk_widget_destroy(dialog);
	g_object_unref(fbuilder);
	if (files == NULL)
		return EXIT_SUCCESS;

}

int main(int argc, char **argv)
{
    int i = 0;
    GOptionContext *context = NULL;
    GError *error = NULL;
	gchar *path;
	const gchar *config_dir;
	gchar *config_path = NULL;

	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);

    context = g_option_context_new(_("[FILE...] - GPX Viewer"));

	g_option_context_set_summary(context, N_("A simple program to visualize one or more gpx files."));
	g_option_context_set_description(context, N_("Website: http://blog.sarine.nl/gpx-viewer"));

    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if (error) {
        g_log(NULL, G_LOG_LEVEL_ERROR, "Failed to parse commandline options: %s", error->message);
        g_error_free(error);
		error = NULL;
    }
	if(!g_thread_supported())
		g_thread_init(NULL);


	config_dir = g_get_user_config_dir();
	g_assert(config_dir != NULL);
	g_debug("Config dir is: %s", config_dir);


	config_path = g_build_filename(config_dir, "gpx-viewer", NULL);
	if(!g_file_test(config_path, G_FILE_TEST_IS_DIR))
	{
		g_mkdir_with_parents(config_path, 0700);
	}
	g_free(config_path);

	config_path = g_build_filename(config_dir, "gpx-viewer", "config.ini", NULL);
	config_file = g_key_file_new();
	g_key_file_load_from_file(config_file, config_path, G_KEY_FILE_NONE, &error);

    if (error) {
        g_debug("Failed to open config file %s: %s",config_path, error->message);
        g_error_free(error);
		error = NULL;
    }


    gtk_clutter_init(&argc, &argv);

	path = g_build_filename(DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
			path);
	g_free(path);

    /* Open all the files given on the command line */
    for (i = 1; i < argc; i++) {
        /* Try to open the gpx file */
        GpxFile *file = gpx_file_new(argv[i]);
        files = g_list_prepend(files, file);
    }
	files = g_list_reverse(files);

    create_interface();

    gtk_main();

    /* Cleanup office */
    /* Destroy the files */
    g_list_foreach(g_list_first(files), (GFunc) g_object_unref, NULL);
    g_list_free(files);

	/* Save config file. */
	if(config_path) {
		gssize length=0;
		gchar *data = g_key_file_to_data(config_file, &length,&error);
		if(error) {
			g_error("Faield to write config file: %s", error->message);
			g_error_free(error);
			error = NULL;
		}
		if(data)
		{
			g_file_set_contents(config_path, data, length, &error);
			if(error) {
				g_error("Faield to write config file: %s", error->message);
				g_error_free(error);
				error = NULL;
			}
		}
		g_free(data);
	}
	g_free(config_path);

	return EXIT_SUCCESS;
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

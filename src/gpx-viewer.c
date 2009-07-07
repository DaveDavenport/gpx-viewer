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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <champlain/champlain.h>
#include <champlain-gtk/champlain-gtk.h>
#include <clutter-gtk/gtk-clutter-embed.h>
#include <config.h>
#include <glib/gi18n.h>
#include "gpx-parser.h"
#include "gpx-graph.h"

/* List of gpx files */
GList *files                        = NULL;
GtkWidget *champlain_view           = NULL;
GtkBuilder *builder                 = NULL;
GpxGraph *gpx_graph                 = NULL;
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
} Route;

/* The currently active route */
Route *active_route                 = NULL;

static void free_Route(Route *route)
{
    g_free(route);
}

void on_destroy(void)
{
    printf("Quit...\n");
    gtk_main_quit();

    gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(builder, "gpx_viewer_window")));
    g_object_unref(builder);

    g_list_foreach(routes, (GFunc)free_Route, NULL);
    g_list_free(routes); routes = NULL;
}

/**
 * Update on track changes
 */
static void interface_update_heading(GtkBuilder * builder, GpxTrack * track)
{
    time_t temp;
    gdouble gtemp;
    GtkWidget *label = NULL;
    /* Duration */
    label = (GtkWidget *) gtk_builder_get_object(builder, "duration_label");
    temp = gpx_track_get_total_time(track);
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
    gtemp = track->total_distance;
    if (gtemp > 0) {
        gchar *string = g_strdup_printf("%.2f km", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Average */
    label = (GtkWidget *) gtk_builder_get_object(builder, "average_label");
    gtemp = gpx_track_get_track_average(track);
    if (gtemp > 0) {
        gchar *string = g_strdup_printf("%.2f km/h", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* Moving Average */
    label = (GtkWidget *) gtk_builder_get_object(builder, "moving_average_label");
    gtemp = gpx_track_calculate_moving_average(track, &temp);
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
    label = (GtkWidget *) gtk_builder_get_object(builder, "max_speed_label");
    gtemp = track->max_speed;
    if (gtemp > 0) {
        gchar *string = g_strdup_printf("%.2f km/h", gtemp);
        gtk_label_set_text(GTK_LABEL(label), string);
        g_free(string);
    } else {
        gtk_label_set_text(GTK_LABEL(label), "n/a");
    }

    /* GPS Points */
    label = (GtkWidget *) gtk_builder_get_object(builder, "num_points_label");
    gtemp = (double)g_list_length(track->points);
    if (gtemp > 0) {
        gchar *string = g_strdup_printf("%.0f points", gtemp);
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
}

static void interface_map_file_waypoints(ChamplainView *view, GpxFile *file)
{
	for(GList *it = g_list_first(file->waypoints); it; it = g_list_next(it))
	{
		GpxPoint *p = it->data;
		ClutterActor *marker = champlain_marker_new_with_text(p->name, "Seric 12", NULL, NULL);
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

/* UI functions */
void route_set_visible(GtkCheckButton * button, gpointer user_data)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    if (active_route) {
        if (active_route->visible != active) {
            active_route->visible = active;
            if (active) {
                champlain_polygon_show(active_route->polygon);
            } else {
                champlain_polygon_hide(active_route->polygon);
            }
        }
    }
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

void routes_combo_changed_cb(GtkComboBox * box, gpointer user_data)
{
    GtkTreeModel *model = gtk_combo_box_get_model(box);
    GtkTreeIter iter;
    if (gtk_combo_box_get_active_iter(box, &iter)) {
        Route *route = NULL;
        gtk_tree_model_get(model, &iter, 1, &route, -1);
        if (active_route) {
            champlain_polygon_set_stroke_color(active_route->polygon, &normal_track_color);
            if (active_route->visible)
                champlain_polygon_show(active_route->polygon);

			if(active_route->stop) 
				clutter_actor_hide(CLUTTER_ACTOR(active_route->stop));

			if(active_route->start) 
				clutter_actor_hide(CLUTTER_ACTOR(active_route->start));
        }

        if (route) {
            ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
            champlain_polygon_set_stroke_color(route->polygon, &highlight_track_color);

            if (route->visible)
                champlain_polygon_show(route->polygon);

            interface_update_heading(builder, route->track);

            if (route->track->top && route->track->bottom) {
                champlain_view_ensure_visible(view,
                        route->track->top->lat_dec, route->track->top->lon_dec,
                        route->track->bottom->lat_dec, route->track->bottom->lon_dec, FALSE);
            }

            if (gpx_track_get_total_time(route->track) > 0) {
                gpx_graph_set_track(gpx_graph, route->track);
                gtk_widget_show(GTK_WIDGET(gpx_graph));
            } else {
                gpx_graph_set_track(gpx_graph, NULL);
                gtk_widget_hide(GTK_WIDGET(gpx_graph));
            }

			if(route->stop) 
				clutter_actor_show(CLUTTER_ACTOR(route->stop));

			if(route->start) 
				clutter_actor_show(CLUTTER_ACTOR(route->start));
        }
        active_route = route;

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "route_visible_check_button")),
                active_route->visible);
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
    if (current != new) {
        gpx_graph_set_smooth_factor(gpx_graph, new);
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

static void graph_point_clicked(double lat_dec, double lon_dec)
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
		champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(click_marker), lat_dec, lon_dec);
		champlain_marker_set_color(CHAMPLAIN_MARKER(click_marker), &waypoint);
		clutter_container_add(CLUTTER_CONTAINER(marker_layer), CLUTTER_ACTOR(click_marker), NULL);
		clutter_actor_show(CLUTTER_ACTOR(click_marker));
	}else{
		champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(click_marker), lat_dec, lon_dec);
		clutter_actor_show(CLUTTER_ACTOR(click_marker));
	}
	if(click_marker_source >0) {
		g_source_remove(click_marker_source);
	}

	marker[0] =(ChamplainBaseMarker *) click_marker;
	champlain_view_ensure_markers_visible(view, marker, TRUE);

    click_marker_source = g_timeout_add_seconds(5, (GSourceFunc) graph_point_remove, click_marker);
}

/* Create the interface */
static void create_interface(void)
{
    double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
    time_t temp;
    GError *error = NULL;
    const gchar *const *dirs = g_get_system_data_dirs();
    int i = 0;
    GtkWidget *sp = NULL;
    gchar *path = g_build_filename(DATA_DIR, "gpx-viewer.ui", NULL);
    int current;

	/* Open UI description file */
    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, path, NULL)) {
        g_error("Failed to create ui: %s\n", error->message);
    }
    g_free(path);

    /* Create map view */
    champlain_view = gtk_champlain_embed_new();
    gtk_widget_set_size_request(champlain_view, 640, 280);
    gtk_paned_pack1(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), champlain_view, TRUE, TRUE);
    /* graph */
    gpx_graph = gpx_graph_new();
    gtk_paned_pack2(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), GTK_WIDGET(gpx_graph), FALSE, TRUE);
    gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(builder, "main_view_pane")), 200);

    /* show the interface */
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "gpx_viewer_window")));

    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    g_object_set(G_OBJECT(view), "scroll-mode", CHAMPLAIN_SCROLL_MODE_KINETIC, "zoom-level", 5, NULL);

	if (marker_layer == NULL) {
		marker_layer = champlain_layer_new();
		champlain_view_add_layer(view, marker_layer);
	}

    interface_map_make_waypoints(view);

    for (GList *fiter = g_list_first(files); fiter; fiter = g_list_next(fiter)) {
        GpxFile *file = fiter->data;
        if (file->tracks) {
            GpxTrack *track = file->tracks->data;
            /* Plot all tracks, and get total bounding box */
            GtkTreeIter liter;
            GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(builder, "routes_store");
            for (GList *iter = g_list_first(file->tracks); iter; iter = g_list_next(iter)) {
				GtkIconInfo *ii;
				struct Route *route = g_new0(Route, 1);
                /* Route */
                route->file = file;
                route->track = iter->data;
                route->visible = TRUE;

                /* draw the track */
                interface_map_plot_route(view, route);
                if (track->top && track->top->lat_dec < lat1)
                    lat1 = track->top->lat_dec;
                if (track->top && track->top->lon_dec < lon1)
                    lon1 = track->top->lon_dec;

                if (track->bottom && track->bottom->lat_dec > lat2)
                    lat2 = track->bottom->lat_dec;
                if (track->bottom && track->bottom->lon_dec > lon2)
                    lon2 = track->bottom->lon_dec;

                gtk_list_store_append(GTK_LIST_STORE(model), &liter);
                gtk_list_store_set(GTK_LIST_STORE(model), &liter, 
                        0, (route->track->name) ? route->track->name : "n/a",
                        1, route, -1);
				/* Pin's */
				if(route->track)
				{
					const GList *start = g_list_first(route->track->points);
					const GList *stop = g_list_last(route->track->points);

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
				routes = g_list_append(routes, route);
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
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)current);

    g_signal_connect(gpx_graph, "notify::smooth-factor", G_CALLBACK(smooth_factor_changed), sp);
    g_signal_connect(gpx_graph, "point-clicked", G_CALLBACK(graph_point_clicked), NULL);
    gtk_builder_connect_signals(builder, NULL);
    /* Try to center the track on map correctly */
    if (lon1 < 1000.0 && lon2 < 1000.0) {
        champlain_view_set_zoom_level(view, 8);
        champlain_view_ensure_visible(view, lat1, lon1, lat2, lon2, FALSE);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "routes_combo")), 0);
}

int main(int argc, char **argv)
{

    int i = 0;
    GOptionContext *context = NULL;
    GError *error = NULL;
	gchar *path;
    context = g_option_context_new(_("GPX Viewer"));

    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if (error) {
        g_log(NULL, G_LOG_LEVEL_ERROR, "Failed to parse commandline options: %s", error->message);
        g_error_free(error);
    }

    g_thread_init(NULL);
    gtk_clutter_init(&argc, &argv);

	path = g_build_filename(DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
			path);
	g_free(path);

	/* If no file(s) given, ask for it */
    if (argc < 2) {
        GtkWidget *dialog;
        GtkBuilder *fbuilder = gtk_builder_new();
        /* Show dialog */
        gchar *path = g_build_filename(DATA_DIR, "gpx-viewer-file-chooser.ui", NULL);
        if (!gtk_builder_add_from_file(fbuilder, path, NULL)) {
            g_error("Failed to load gpx-viewer.ui");
        }
        g_free(path);
        /* update filter */
        {
            GtkFileFilter *filter =
                (GtkFileFilter *) gtk_builder_get_object(fbuilder, "gpx_viewer_file_chooser_filter");
            gtk_file_filter_add_pattern(filter, "*.gpx");

        }
        dialog = GTK_WIDGET(gtk_builder_get_object(fbuilder, "gpx_viewer_file_chooser"));
        switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
            case 1:
                {
                    GSList *iter, *choosen_files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
                    for (iter = choosen_files; iter; iter = g_slist_next(iter)) {
                        /* Try to open the gpx file */
                        GpxFile *file = gpx_file_new((gchar *) iter->data);
                        files = g_list_append(files, file);
                    }
                    g_slist_foreach(choosen_files, (GFunc) g_free, NULL);
                    g_slist_free(choosen_files);
                }
                break;
        }
        gtk_widget_destroy(dialog);
        g_object_unref(fbuilder);
        if (files == NULL)
            return EXIT_SUCCESS;

    }
    /* Open all the files given on the command line */
    for (i = 1; i < argc; i++) {
        /* Try to open the gpx file */
        GpxFile *file = gpx_file_new(argv[i]);
        files = g_list_append(files, file);
    }

    create_interface();

    gtk_main();
    /* Cleanup office */
    /* Destroy the files */
    g_list_foreach(files, (GFunc) g_object_unref, NULL);
    g_list_free(files);

    return EXIT_SUCCESS;
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

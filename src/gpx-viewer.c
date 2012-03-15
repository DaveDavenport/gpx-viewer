/* Gpx Viewer
 * Copyright (C) 2009-2011 Qball Cow <qball@sarine.nl>
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
#include <time.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <champlain/champlain.h>
#include <champlain-gtk/champlain-gtk.h>
#include <clutter-gtk/clutter-gtk.h>


#include <gdl/gdl.h>
#include "gpx-viewer.h"
#include "gpx.h"
#include "gpx-viewer-path-layer.h"


/************************************************************
 *  GPX-Viewer                                              *
 ************************************************************/
typedef struct _GpxViewerPrivate {
	int id;
	GpxViewerSettings	*settings;
	/* Recent manager */
	GtkRecentManager    *recent_man;
	/* Track playback widget */
	GpxPlayback *playback;
	/* */
	guint click_marker_source;
	/* List of gpx files */
	GList               *files;
	/* The interface builder */
	GtkBuilder          *builder;
	/* The graph */
	GpxGraph            *gpx_graph;

} GpxViewerPrivate;

G_DEFINE_TYPE (GpxViewer, gpx_viewer, GTK_TYPE_APPLICATION)
#define GPX_VIEWER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPX_TYPE_VIEWER, GpxViewerPrivate))

#define GPX_TYPE_VIEWER  (gpx_viewer_get_type())


static GtkWidget        *dock_items[3];
static GdlDockLayout    *dock_layout = NULL;

/* The Map view widget */
GtkWidget           *champlain_view         = NULL;
/**
 * This structure holds all information related to
 * a track.
 * The file the track belongs to, the path on the map start/stop marker
 * and the visible state.
 */
typedef struct Route
{
    GpxFile *file;
    GpxTrack *track;
    GpxViewerPathLayer *path;
    ChamplainMarker *start;
    ChamplainMarker *stop;
    gboolean visible;
} Route;

/* List of routes GList<Route> *routes */
GList *routes                       = NULL;

/**
 * This points to the currently active Route.
 */
Route *active_route                 = NULL;


/* TODO get correct values */
#define KM_IN_MILE 0.621371192
#define M_IN_FEET 0.3048

void gv_set_speed_label(GtkWidget *label, gdouble speed, SpeedFormat format)
{
	gchar *val = gpx_viewer_misc_convert(speed, (speed == 0)?NA:format);
	gtk_label_set_text(GTK_LABEL(label), val);
	g_free(val);
}

gchar * gpx_viewer_misc_convert(gdouble speed, SpeedFormat format)
{
	gchar 		*retv	 = NULL;
	/* TODO: Make config option? */
	gboolean 	do_miles = FALSE;
	switch(format)
	{
		case DISTANCE:
			if(do_miles)
				retv = g_strdup_printf( "%' '.2f %s", speed/KM_IN_MILE, _("Miles"));
			else
				retv = g_strdup_printf( "%' '.2f %s", speed, _("km"));
			break;
		case SPEED:
			if(do_miles)
				retv = g_strdup_printf( "%' '.2f %s", speed/KM_IN_MILE, _("Miles/h"));
			else
				retv = g_strdup_printf( "%' '.2f %s", speed, _("km/h"));
			break;
		case ELEVATION:
			if(do_miles)
				/* TODO: */
				retv = g_strdup_printf( "%' '.2f %s", speed/M_IN_FEET, _("ft"));
			else
				retv = g_strdup_printf( "%' '.2f %s", speed, _("m"));
			break;
		case ACCEL:
			if(do_miles)
				/* TODO: */
				retv = g_strdup_printf( "%' '.2f %s", speed/M_IN_FEET, _("ft/s²"));
			else
				retv = g_strdup_printf( "%' '.2f %s", speed, _("m/s²"));
			break;
		case NA:
		default:
			retv = g_strdup(_("n/a"));
			break;
	}
	return retv;
}
/**
 * Dock loading/restoring
 */

static void restore_layout(void)
{
    const gchar *config_dir = g_get_user_config_dir();
    gchar *layout_path = NULL;
    g_assert(config_dir != NULL);

    layout_path = g_build_filename(config_dir, "gpx-viewer", "dock-layout2.xml",NULL);
    if(g_file_test(layout_path, G_FILE_TEST_EXISTS))
    {
        gdl_dock_layout_load_from_file(dock_layout, layout_path);
        gdl_dock_layout_load_layout(dock_layout, "my_layout");
    }else
	{
		gchar *path = g_build_filename(DATA_DIR, "default-layout.xml", NULL);
        gdl_dock_layout_load_from_file(dock_layout, path);
        gdl_dock_layout_load_layout(dock_layout, "my_layout");
		g_free(path);
	}
    g_free(layout_path);

}

/**
 * Stores the layout off the docks. 
 */
static void save_layout(void)
{
    gchar *layout_path = NULL;
    const gchar *config_dir = g_get_user_config_dir();
    g_assert(config_dir != NULL);
    g_debug("Config dir is: %s", config_dir);

	/**
	 * Create config directory 
	 */
    layout_path = g_build_filename(config_dir, "gpx-viewer", NULL);
    if(!g_file_test(layout_path, G_FILE_TEST_IS_DIR))
    {
        g_mkdir_with_parents(layout_path, 0700);
    }
    g_free(layout_path);

	/**
 	 * Save dock layout 
	 */
    layout_path = g_build_filename(config_dir, "gpx-viewer", "dock-layout2.xml",NULL);
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
    /*	if(route->path) g_object_unref(route->path); */
    if(route->track) g_object_unref(route->track);
    g_free(route);
}


/**
 * This is called when the main window is destroyed
 */
void on_destroy(GtkWidget *widget, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    g_debug("Quit...");

    save_layout();

    gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(priv->builder, "gpx_viewer_window")));
    g_object_unref(priv->builder);
	priv->builder = NULL;

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
static void interface_update_heading(GtkBuilder * c_builder, GpxTrack * track, GpxPoint *start, GpxPoint *stop, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    GtkWidget *label = NULL;
    time_t temp = 0;
    gdouble gtemp = 0, max_speed = 0;

    /* Duration */
    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "duration_label");
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
        gtk_label_set_text(GTK_LABEL(label), _("n/a"));
    }
    /* Start time */
    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "start_time_label");
    if(start)
    {
        char buffer[128];
        struct tm ltm;
        temp = gpx_point_get_time(start);
        localtime_r(&temp, &ltm);
        strftime(buffer, 128, "%D %X", &ltm);
        gtk_label_set_text(GTK_LABEL(label), buffer);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), _("n/a"));
    }

    /* Stop time */
    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "stop_time_label");
    if(stop)
    {
        char buffer[128];
        struct tm ltm;
        temp = gpx_point_get_time(stop);
        localtime_r(&temp, &ltm);
        strftime(buffer, 128, "%D %X", &ltm);
        gtk_label_set_text(GTK_LABEL(label), buffer);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), _("n/a"));
    }
    /* Distance */
    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "distance_label");
    gtemp = 0;
    if(start && stop)
    {
        gtemp = stop->distance-start->distance;
    }
	gv_set_speed_label(label, gtemp, DISTANCE);

    /* Average */
    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "average_label");
    gtemp = 0;
    if(start && stop)
    {
        gtemp = gpx_track_calculate_point_to_point_speed(track,start, stop);
    }
	gv_set_speed_label(label, gtemp, SPEED);

    /* Moving Average */
    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "moving_average_label");
    gtemp = 0;
    temp = 0;
    if(start && stop)
    {
        /* Calculates both time and km/h */
        gtemp = gpx_track_calculate_moving_average(track,start, stop, &temp);
    }
	gv_set_speed_label(label, gtemp, SPEED);

    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "moving_average_time_label");
    if (gtemp > 0)
    {
        GString *string = misc_get_time(temp);
        gtk_label_set_text(GTK_LABEL(label), string->str);
        g_string_free(string, TRUE);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), _("n/a"));
    }

    /* Max speed */
    if(track && start && stop)
    {
        GList *list ;
        for(list = g_list_find(track->points, start); list && list->data != stop; list = g_list_next(list))
        {
            max_speed = MAX(max_speed, ((GpxPoint *)list->data)->speed);
        }
    }
    label = (GtkWidget *) gtk_builder_get_object(priv->builder, "max_speed_label");
	gv_set_speed_label(label, max_speed, SPEED);

    /* Gradient */
    {
        gdouble elevation_diff = 0;
        gdouble distance_diff = 0;
        label = (GtkWidget *) gtk_builder_get_object(priv->builder, "gradient_label");

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
        label = (GtkWidget *) gtk_builder_get_object(priv->builder, "elevation_difference_label");

        elevation_diff = 0;
        if (start && stop)
        {
            elevation_diff = stop->elevation - start->elevation;
            distance_diff = stop->distance - start->distance;
        }
	    gv_set_speed_label(label, elevation_diff, ELEVATION);
    }
}


/**
 * Creates a Path for Route, and adds it to the view
 */
static void interface_map_plot_route(ChamplainView * view, struct Route *route)
{
    /* If Route has allready a route, exit */
    if(route->path != NULL)
    {
        g_warning("Route allready has a path.\n");
        return;
    }
    route->path = gpx_viewer_path_layer_new();
    gpx_viewer_path_layer_set_stroke_width(route->path, 5.0);
	// TODO: Set track not sensitive.
    //gpx_viewer_path_layer_set_stroke_color(route->path, &normal_track_color);
	gpx_viewer_path_layer_set_track(route->path,route->track);
    champlain_view_add_layer(CHAMPLAIN_VIEW(view), CHAMPLAIN_LAYER(route->path));
	clutter_actor_set_depth(CLUTTER_ACTOR(route->path), -10);
    if(!route->visible) gpx_viewer_path_layer_set_visible(route->path, FALSE);
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


static void interface_map_make_waypoints(ChamplainView * view, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
    GList *iter;
    for (iter = g_list_first(priv->files); iter != NULL; iter = g_list_next(iter))
    {
        GpxFile *file = iter->data;
        interface_map_file_waypoints(view, file);
    }
}


/* Show and hide waypoint layer */
void show_waypoints_layer_toggled_cb(GtkSwitch * button, GParamSpec *spec,gpointer user_data)
{
    gboolean active = gtk_switch_get_active(button);
    if(active != gpx_viewer_map_view_get_show_waypoints(GPX_VIEWER_MAP_VIEW(champlain_view)))
    {
        gpx_viewer_map_view_set_show_waypoints(GPX_VIEWER_MAP_VIEW(champlain_view), active);
    }
}


static void show_waypoints_layer_changed(GpxViewerMapView *view, GParamSpec * gobject, GtkWidget *sp)
{
    gboolean active = gpx_viewer_map_view_get_show_waypoints(GPX_VIEWER_MAP_VIEW(champlain_view));
    if(gtk_switch_get_active(GTK_SWITCH(sp)) != active)
    {
        gtk_switch_set_active(GTK_SWITCH(sp), active);
    }
}


/**
 * Handle user selecting another track
 */
void routes_list_changed_cb(GtkTreeSelection * sel, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
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
			if(active_route->path != NULL)
			{
				// Todo set track not sensitive.
				//gpx_viewer_path_layer_set_stroke_color(active_route->path, &normal_track_color);
			}
			/* Hide stop marker */
            if(active_route->stop)
                clutter_actor_hide(CLUTTER_ACTOR(active_route->stop));

            /* Hide start marker */
            if(active_route->start)
                clutter_actor_hide(CLUTTER_ACTOR(active_route->start));

            /* Stop playback */
            gpx_playback_stop(priv->playback);
            /* Clear graph */
            gpx_graph_set_track(priv->gpx_graph, NULL);
            /* Hide graph */
			/* if not visible hide track again */
			if(!active_route->visible) {
                gpx_viewer_path_layer_set_visible(active_route->path, FALSE);
			}
        }

        active_route = route;
        if (route)
        {
            ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));

            gpx_playback_set_track(priv->playback, active_route->track);
            if(route->path != NULL) {
				// TODO: set track sensitive.
//                gpx_viewer_path_layer_set_stroke_color(route->path, &highlight_track_color);
			}

            if(route->path /*&& route->visible*/)
			{
				gpx_viewer_path_layer_set_visible(route->path, TRUE);
            }
            if (route->track->top && route->track->bottom)
            {
                ChamplainBoundingBox *track_bounding_box;
                printf("zet zoom leveland view track\n");
				champlain_view_set_zoom_level(view,
						champlain_view_get_max_zoom_level(view));
                track_bounding_box = champlain_bounding_box_new();
				champlain_bounding_box_extend(track_bounding_box, route->track->top->lat_dec, route->track->top->lon_dec);
				champlain_bounding_box_extend(track_bounding_box, route->track->bottom->lat_dec, route->track->bottom->lon_dec);
                champlain_view_ensure_visible(view, track_bounding_box, TRUE);
                champlain_bounding_box_free(track_bounding_box);
            }

            if (gpx_track_get_total_time(active_route->track) > 5)
            {
                gpx_graph_set_track(priv->gpx_graph, active_route->track);
            }
            else
            {
                gpx_graph_set_track(priv->gpx_graph, NULL);
            }

            if(route->stop){
                clutter_actor_show(CLUTTER_ACTOR(route->stop));
				printf("show stop marker\n");
			}

            if(route->start)
                clutter_actor_show(CLUTTER_ACTOR(route->start));
        }
        else
        {
            /* Create a false route here. f.e. to show multiple tracks concatenated */
        }
    }
	else
	{
		gpx_graph_set_track(priv->gpx_graph, NULL);
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
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
    int current = gpx_graph_get_smooth_factor(priv->gpx_graph);
    int new = gtk_spin_button_get_value_as_int(spin);
    if (current != new)
    {
        gpx_graph_set_smooth_factor(priv->gpx_graph, new);
    }
}


/* Show and hide points on graph */
void graph_show_points_toggled_cb(GtkSwitch * button, GParamSpec *spec,gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
    gboolean new = gtk_switch_get_active(button);
    gpx_graph_set_show_points(priv->gpx_graph, new);
}


static void graph_show_points_changed(GtkWidget *graph, GParamSpec *sp, GtkWidget *toggle)
{
    int current = gpx_graph_get_show_points(GPX_GRAPH(graph));
    if(current != gtk_switch_get_active(GTK_SWITCH(toggle)))
    {
        gtk_switch_set_active(GTK_SWITCH(toggle),current);
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


static gboolean graph_point_remove(gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
	gpx_viewer_map_view_click_marker_hide(GPX_VIEWER_MAP_VIEW(champlain_view));
	gpx_graph_highlight_point(priv->gpx_graph, NULL);
    return FALSE;
}


static void graph_selection_changed(GpxGraph *graph,GpxTrack *track, GpxPoint *start, GpxPoint *stop, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);	
    interface_update_heading(priv->builder, track, start, stop, gpx_viewer);
    if(active_route && active_route->track->points != NULL)
    {
        if(start)
        {
            champlain_location_set_location (CHAMPLAIN_LOCATION (active_route->start), start->lat_dec, start->lon_dec);
        }

        if(stop)
        {
            champlain_location_set_location (CHAMPLAIN_LOCATION (active_route->stop), stop->lat_dec, stop->lon_dec);
        }
    }
}


static void graph_point_clicked(GpxGraph *graph, GpxPoint *point, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));

	gpx_viewer_map_view_click_marker_show(GPX_VIEWER_MAP_VIEW(champlain_view), point);
    if(priv->click_marker_source >0)
    {
        g_source_remove(priv->click_marker_source);
    }

	gpx_graph_highlight_point(priv->gpx_graph, point);
    gpx_graph_show_info(priv->gpx_graph, point);

	// Center map on this point.
	champlain_view_center_on(view, point->lat_dec, point->lon_dec);


    priv->click_marker_source = g_timeout_add_seconds(5, (GSourceFunc) graph_point_remove, gpx_viewer);
}


void playback_play_clicked(GtkWidget *widget, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
    if(active_route)
    {
        gpx_playback_start(priv->playback);
    }
}


void playback_pause_clicked(GtkWidget *widget, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
    if(active_route)
    {
        gpx_playback_pause(priv->playback);
    }
}


void playback_stop_clicked(GtkWidget *widget, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
    if(active_route)
    {
        gpx_playback_stop(priv->playback);
    }
}


static void route_playback_tick(GpxPlayback *route_playback, GpxPoint *current, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    if(current != NULL)
    {
        graph_point_clicked(priv->gpx_graph, current,gpx_viewer);
    }
}


static void route_playback_state_changed(GpxPlayback *route_playback, GpxPlaybackState state, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);	
    GtkWidget *w_stopped =(GtkWidget *) gtk_builder_get_object(priv->builder, "eventbox2");
    GtkWidget *w_play = (GtkWidget *)gtk_builder_get_object(priv->builder, "eventbox3");
    GtkWidget *w_paused = (GtkWidget *)gtk_builder_get_object(priv->builder, "eventbox1");
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
static void interface_plot_add_track(GpxViewer *gpx_viewer, GtkTreeIter *parent, GpxTrack *track, double *lat1, double *lon1, double *lat2, double *lon2)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);

    ChamplainView *view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    /* Plot all tracks, and get total bounding box */
    GtkTreeIter liter;
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(priv->builder, "routes_store");
    struct Route *route = g_new0(Route, 1);
    /* Route */
    if(gpx_viewer_settings_get_integer(priv->settings,"Track", "Cleanup", 0) > 0)
    {
        route->track = gpx_track_cleanup_speed(track);
    }
    else
    {
        route->track = g_object_ref(track);
    }
    route->visible = FALSE;

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
            gtk_tree_view_expand_to_path(GTK_TREE_VIEW(gtk_builder_get_object(priv->builder, "TracksTreeView")), path);
            gtk_tree_path_free(path);
        }

    }
    first = FALSE;
    /* Pin's */
    if(route->track)
    {
        const GList *start = g_list_first(route->track->points);
        GpxPoint *stop = gpx_track_get_last(route->track);
        if(start && stop)
        {
            /* create start marker */
            route->start = gpx_viewer_map_view_create_marker(GPX_VIEWER_MAP_VIEW(champlain_view),start->data, "pin-green", 64);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), CHAMPLAIN_MARKER(route->start));

            /* create end marker */
            route->stop = gpx_viewer_map_view_create_marker(GPX_VIEWER_MAP_VIEW(champlain_view),stop, "pin-red",64);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), CHAMPLAIN_MARKER(route->stop));

            clutter_actor_hide(CLUTTER_ACTOR(route->stop));
            clutter_actor_hide(CLUTTER_ACTOR(route->start));
        }
    }

    routes = g_list_append(routes, route);
}

void main_window_size_changed(GtkWindow *win, GtkAllocation *alloc, gpointer data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(data);
    if(alloc)
    {
        gpx_viewer_settings_set_integer(priv->settings, "Window", "width", alloc->width);
        gpx_viewer_settings_set_integer(priv->settings, "Window", "height", alloc->height);
        g_debug("size: %i - %i\n", alloc->width, alloc->height);
    }

}


void row_visible_toggled(GtkCellRendererToggle *toggle, const gchar *path, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);	
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(priv->builder, "routes_store");
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
                gpx_viewer_path_layer_set_visible(route->path, TRUE);
            }
            else
            {
                gpx_viewer_path_layer_set_visible(route->path, FALSE);
            }
        }
    }
}


void show_elevation(GtkMenuItem *item, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);	
    if(gpx_graph_get_mode(priv->gpx_graph) != GPX_GRAPH_GRAPH_MODE_ELEVATION)
    {
        g_debug("switch to elevation\n");
        gpx_graph_set_mode(priv->gpx_graph, GPX_GRAPH_GRAPH_MODE_ELEVATION);
    }
}


void show_speed(GtkMenuItem *item, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);	
    if(gpx_graph_get_mode(priv->gpx_graph) != GPX_GRAPH_GRAPH_MODE_SPEED)
    {
        g_debug("switch to speed\n");
        gpx_graph_set_mode(priv->gpx_graph, GPX_GRAPH_GRAPH_MODE_SPEED);
    }
}


void show_distance(GtkMenuItem *item, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);	
    if(gpx_graph_get_mode(priv->gpx_graph) != GPX_GRAPH_GRAPH_MODE_DISTANCE)
    {
        g_debug("switch to distance\n");
        gpx_graph_set_mode(priv->gpx_graph, GPX_GRAPH_GRAPH_MODE_DISTANCE);
    }
}


void show_acceleration_h(GtkMenuItem *item, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);	
    if(gpx_graph_get_mode(priv->gpx_graph) != GPX_GRAPH_GRAPH_MODE_ACCELERATION_H)
    {
        g_debug("switch to acceleration\n");
        gpx_graph_set_mode(priv->gpx_graph, GPX_GRAPH_GRAPH_MODE_ACCELERATION_H);
    }
}


void show_vertical_speed(GtkMenuItem *item, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);	
    if(gpx_graph_get_mode(priv->gpx_graph) != GPX_GRAPH_GRAPH_MODE_SPEED_V)
    {
        g_debug("switch to vertical speed\n");
        gpx_graph_set_mode(priv->gpx_graph, GPX_GRAPH_GRAPH_MODE_SPEED_V);
    }
}


static void interface_create_fake_master_track(GpxFile *file, GtkTreeIter *liter, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    GList *iter;
    ClutterColor *start_point_color;
    ClutterColor *stop_point_color;
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(priv->builder, "routes_store");
    struct Route *route = g_new0(Route, 1);
    route->visible = TRUE;
    route->path = NULL;
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
        GpxPoint *stop = gpx_track_get_last(route->track);
		/*g_list_last(route->track->points);*/
        if(start && stop)
        {
            /* create start marker */
            start_point_color = clutter_color_new(0, 255, 0, 1);
            route->start = (ChamplainMarker *)champlain_point_new_full(3, start_point_color);
            clutter_color_free(start_point_color);
            /* Create the marker */
            champlain_location_set_location (CHAMPLAIN_LOCATION (route->start),
                ((GpxPoint*)start->data)->lat_dec,
                ((GpxPoint*)start->data)->lon_dec);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), route->start);

            /* create end marker */
            stop_point_color = clutter_color_new(255, 0, 0, 1);
            route->stop = (ChamplainMarker *)champlain_point_new_full(3, stop_point_color);
            clutter_color_free(stop_point_color);
            /* Create the marker */
            champlain_location_set_location (CHAMPLAIN_LOCATION (route->stop),
                (stop)->lat_dec,
                (stop)->lon_dec);
            gpx_viewer_map_view_add_marker(GPX_VIEWER_MAP_VIEW(champlain_view), route->stop);

            clutter_actor_hide(CLUTTER_ACTOR(route->stop));
            clutter_actor_hide(CLUTTER_ACTOR(route->start));
        }
    }
    routes = g_list_append(routes, route);
    gtk_tree_store_set(GTK_TREE_STORE(model), liter, 1, route, -1);
}


static void recent_chooser_file_picked(GtkRecentChooser *grc, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    GtkTreeIter liter;
    double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
    GList *iter;
    GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(priv->builder, "routes_store");
    gchar *basename,*uri = gtk_recent_chooser_get_current_uri(grc);
    GFile *afile = g_file_new_for_uri(uri);
    /* Try to open the gpx file */
    GpxFile *file = gpx_file_new(afile);
    g_object_unref(afile);
    g_free(uri);
    priv->files = g_list_append(priv->files, file);


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
            interface_plot_add_track(gpx_viewer,&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
        }
    }
    if(file->routes)
    {
        for (iter = g_list_first(file->routes); iter; iter = g_list_next(iter))
        {
            interface_plot_add_track(gpx_viewer,&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
        }
    }
    interface_create_fake_master_track(file, &liter,gpx_viewer);
}


static void
view_state_changed (ChamplainView *view,
GParamSpec *gobject,
gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    static guint sb_context = 0;
    ChamplainState state;
    GtkWidget *sb = GTK_WIDGET(gtk_builder_get_object(priv->builder, "statusbar2"));
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

static void map_view_clicked(GpxViewerMapView *view, double lat, double lon, gpointer gpx_viewer)
{
	if(active_route)
	{
		GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);	
		/* Check if the clicked point is within the bounding box of the current track */
		if(active_route->track->top->lon_dec > lon && active_route->track->top->lat_dec > lat && 
			active_route->track->bottom->lon < lon && active_route->track->bottom->lat_dec < lat)
			{
				double lat_r = lat*M_PI/180;
				double lon_r = lon*M_PI/180;
				
				GpxPoint *d = NULL;
				double distance = 0;
				/* Find closest point */
				GList *iter = g_list_first(active_route->track->points);
				for(;iter;iter = g_list_next(iter))
				{
					GpxPoint *a = iter->data;
					double di = gpx_track_calculate_distance_coords(a->lon, a->lat, lon_r, lat_r);
					if(d == NULL || di < distance) {
						d = iter->data;
						distance = di;
					}
				}
				if(distance < 0.5)
				{
			        graph_point_clicked(priv->gpx_graph, d, gpx_viewer);
				}
			}
	}
}
static void map_view_zoom_level_changed(GpxViewerMapView *view, 
			int zoom_level, int min_level, 
			int max_level, GtkWidget *sp)
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




/* React when graph mode changes */
static void graph_mode_changed(GpxGraph *graph, GParamSpec *sp, gpointer gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
    int mode = gpx_graph_get_mode(graph);
    g_debug("Graph mode switched: %i\n", mode);
    switch(mode)
    {
        case GPX_GRAPH_GRAPH_MODE_ELEVATION:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(priv->builder, "view_menu_elevation")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_SPEED:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(priv->builder, "view_menu_speed")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_DISTANCE:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(priv->builder, "view_menu_distance")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_ACCELERATION_H:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(priv->builder, "view_menu_acceleration")), TRUE);
            break;
        case GPX_GRAPH_GRAPH_MODE_SPEED_V:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
                gtk_builder_get_object(priv->builder, "view_menu_vertical_speed")), TRUE);
            break;
        default:
            break;
    }

}


/* Create the interface */
static void create_interface(GtkApplication *gtk_app)
{
	GpxViewerPrivate *  priv = GPX_VIEWER_GET_PRIVATE(gtk_app);
    ChamplainView *view;
    GList *fiter,*iter;
	GtkWidget *gpx_graph_container;
    double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
    GError *error = NULL;
    GtkWidget *sp = NULL;
    gchar *path = g_build_filename(DATA_DIR, "gpx-viewer.ui", NULL);
    GtkTreeSelection *selection;
    GtkWidget *sw,*item,*rc;
    int current;
    gint w,h;
    GtkRecentFilter *grf;
	GtkWidget *dock;
	GtkWidget *map_dock_item, *graph_dock_item;

    /* Open UI description file */
    priv->builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(priv->builder, path, &error))
    {
        g_error("Failed to create ui: %s\n", error->message);
    }
    g_free(path);

    item = gtk_menu_item_new_with_mnemonic(_("_Recent file"));
    gtk_menu_shell_insert(GTK_MENU_SHELL(gtk_builder_get_object(priv->builder, "menu1")),
        item,1);
    rc = gtk_recent_chooser_menu_new();
    g_signal_connect(G_OBJECT(rc), "item-activated", G_CALLBACK(recent_chooser_file_picked), gtk_app);
    grf = gtk_recent_filter_new();
    gtk_recent_filter_add_pattern(GTK_RECENT_FILTER(grf), "*.gpx");
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(rc),grf);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), rc);

    w =gpx_viewer_settings_get_integer(priv->settings,"Window", "width", 400);
    h =gpx_viewer_settings_get_integer(priv->settings,"Window", "height", 300);
    gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(priv->builder,"gpx_viewer_window")), w,h);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(priv->builder, "TracksTreeView")));
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(routes_list_changed_cb), gtk_app);


	
	dock = gdl_dock_new();

   /* Create map view */
    champlain_view = (GtkWidget *)gpx_viewer_map_view_new();

    gtk_widget_set_size_request(champlain_view, 640, 280);
    sw = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(sw), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(sw), champlain_view);
	g_signal_connect(G_OBJECT(champlain_view), "clicked", G_CALLBACK(map_view_clicked), gtk_app);

	map_dock_item = gdl_dock_item_new("Map", "Map",
				GDL_DOCK_ITEM_BEH_CANT_CLOSE|
				GDL_DOCK_ITEM_BEH_CANT_ICONIFY|
				GDL_DOCK_ITEM_BEH_LOCKED|
				GDL_DOCK_ITEM_BEH_NEVER_FLOATING);
	gtk_container_add(GTK_CONTAINER(map_dock_item), sw);
	gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(map_dock_item), GDL_DOCK_RIGHT);
	gtk_widget_show_all(map_dock_item);




    /* graph */
    priv->gpx_graph = gpx_graph_new();
    gpx_graph_container = gtk_frame_new(NULL);
    gtk_widget_set_size_request(gpx_graph_container, -1, 120);
    gtk_frame_set_shadow_type(GTK_FRAME(gpx_graph_container), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(gpx_graph_container), GTK_WIDGET(priv->gpx_graph));
    gtk_widget_show(GTK_WIDGET(priv->gpx_graph));
    //gtk_widget_set_no_show_all(GTK_WIDGET(gpx_graph_container), TRUE);

	graph_dock_item = gdl_dock_item_new("Graph", "Graph",
				GDL_DOCK_ITEM_BEH_CANT_CLOSE|
				GDL_DOCK_ITEM_BEH_CANT_ICONIFY|
				GDL_DOCK_ITEM_BEH_NEVER_FLOATING);

	gtk_container_add(GTK_CONTAINER(graph_dock_item), GTK_WIDGET(gpx_graph_container));
	gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(graph_dock_item), GDL_DOCK_BOTTOM);
	gtk_widget_show_all(graph_dock_item);

    /* show the interface */
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(priv->builder, "gpx_viewer_window")));


    view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(champlain_view));
    g_signal_connect (view, "notify::state", G_CALLBACK (view_state_changed),
        gtk_app);

    interface_map_make_waypoints(view,gtk_app);

    for (fiter = g_list_first(priv->files); fiter; fiter = g_list_next(fiter))
    {
        GpxFile *file = fiter->data;
        GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(priv->builder, "routes_store");
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
                interface_plot_add_track(gtk_app,&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
            }
        }
        if(file->routes)
        {
            for (iter = g_list_first(file->routes); iter; iter = g_list_next(iter))
            {
                interface_plot_add_track(gtk_app,&liter, iter->data, &lat1, &lon1, &lat2, &lon2);
            }
        }
        interface_create_fake_master_track(file, &liter, gtk_app);
    }
    /* Set up the zoom widget */
    sp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "map_zoom_level"));
    current = champlain_view_get_zoom_level(view);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)current);
    g_signal_connect(G_OBJECT(champlain_view), "zoom-level-changed", G_CALLBACK(map_view_zoom_level_changed), sp);

    /* Set up the smooth widget */
    sp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "smooth_factor"));
    gpx_viewer_settings_add_object_property(priv->settings, G_OBJECT(priv->gpx_graph), "smooth-factor");
    current = gpx_graph_get_smooth_factor(priv->gpx_graph);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp), (double)current);
    g_signal_connect(priv->gpx_graph, "notify::smooth-factor", G_CALLBACK(smooth_factor_changed), sp);

    /* */
    sp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "show_waypoints_layer"));
    g_signal_connect(G_OBJECT(champlain_view), "notify::show-waypoints", G_CALLBACK(show_waypoints_layer_changed), sp);
    gpx_viewer_settings_add_object_property(priv->settings, G_OBJECT(champlain_view), "show-waypoints");
    show_waypoints_layer_changed(NULL, NULL, sp);

    g_signal_connect(priv->gpx_graph, "point-clicked", G_CALLBACK(graph_point_clicked), gtk_app);
    g_signal_connect(priv->gpx_graph, "selection-changed", G_CALLBACK(graph_selection_changed), gtk_app);

    /* Set up show points checkbox. Load state from config */
    sp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "graph_show_points"));
    g_signal_connect(priv->gpx_graph, "notify::show-points", G_CALLBACK(graph_show_points_changed), sp);
    gpx_viewer_settings_add_object_property(priv->settings, G_OBJECT(priv->gpx_graph), "show-points");

    /**
     * Restore/Set graph mode
     */
    g_signal_connect(priv->gpx_graph, "notify::mode", G_CALLBACK(graph_mode_changed), gtk_app);
    gpx_viewer_settings_add_object_property(priv->settings, G_OBJECT(priv->gpx_graph), "mode");

    /* Setup the map selector widget */
    {
        GtkWidget *combo = GTK_WIDGET(gtk_builder_get_object(priv->builder, "map_selection_combo"));
        GtkCellRenderer *renderer = (GtkCellRenderer *)gtk_builder_get_object(priv->builder, "cellrenderertext3");
        GtkTreeModel *model = gpx_viewer_map_view_get_model(GPX_VIEWER_MAP_VIEW(champlain_view));
        /* hack to work around GtkBuilder limitation that it cannot set expand
            on packing a cell renderer */
        g_object_ref(renderer);
        gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0,NULL);
        g_object_unref(renderer);

        gtk_combo_box_set_model(GTK_COMBO_BOX(gtk_builder_get_object(priv->builder, "map_selection_combo")),model);
    }

    {

        GtkWidget *flw = (GtkWidget *)gtk_builder_get_object(priv->builder, "FileListWidget");
        GtkWidget *tiw = (GtkWidget *)gtk_builder_get_object(priv->builder, "TrackInfoWidget");
        GtkWidget *swi = (GtkWidget *)gtk_builder_get_object(priv->builder, "SettingWidget");

        /* Dock item */
        dock_items[0] = item = gdl_dock_item_new(
            "Files",
            "File and track list",
            GDL_DOCK_ITEM_BEH_CANT_CLOSE
            |GDL_DOCK_ITEM_BEH_CANT_ICONIFY
			);
        gtk_container_add(GTK_CONTAINER(item), flw);
        gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(item), GDL_DOCK_LEFT);
        gtk_widget_show(item);

        /* Dock item */
        dock_items[1] =     item = gdl_dock_item_new(
            "Information",
            "Detailed track information",
            GDL_DOCK_ITEM_BEH_CANT_CLOSE
            |GDL_DOCK_ITEM_BEH_CANT_ICONIFY
			);
        gtk_container_add(GTK_CONTAINER(item), tiw);
        gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(item), GDL_DOCK_CENTER);
        gtk_widget_show(item);

        /* Dock item */
        dock_items[2] =item = gdl_dock_item_new(
            "Settings",
            "Map and graph settings",
            GDL_DOCK_ITEM_BEH_CANT_CLOSE
            |GDL_DOCK_ITEM_BEH_CANT_ICONIFY
			);
        gtk_container_add(GTK_CONTAINER(item), swi);
        gdl_dock_add_item(GDL_DOCK(dock), GDL_DOCK_ITEM(item), GDL_DOCK_CENTER);
        gtk_widget_show(item);

        gtk_widget_show_all(dock);
		/*GtkWidget *bar = gdl_dock_bar_new(GDL_DOCK(dock));
		gdl_dock_bar_set_orientation(GDL_DOCK_BAR(bar), GTK_ORIENTATION_VERTICAL);
		gtk_widget_show(bar);
        gtk_box_pack_start(GTK_BOX(gtk_builder_get_object(builder, "main_view_hpane")), bar, FALSE, FALSE, 0);
		*/
        gtk_box_pack_end(GTK_BOX(gtk_builder_get_object(priv->builder, "main_view_hpane")), dock, TRUE, TRUE, 0);

        dock_layout = gdl_dock_layout_new(GDL_DOCK(dock));
        restore_layout();

    }
    gpx_viewer_settings_add_object_property(priv->settings, G_OBJECT(champlain_view), "map-source");
    map_view_map_source_changed(GPX_VIEWER_MAP_VIEW(champlain_view), NULL,
        GTK_WIDGET(gtk_builder_get_object(priv->builder, "map_selection_combo")));
    g_signal_connect(G_OBJECT(champlain_view),
        "notify::map-source",
        G_CALLBACK(map_view_map_source_changed),
        GTK_WIDGET(gtk_builder_get_object(priv->builder, "map_selection_combo"))
        );

    /* Connect signals */
    gtk_builder_connect_signals(priv->builder, gtk_app);

    /* Try to center the track on map correctly */
    if (lon1 < 1000.0 && lon2 < 1000.0)
    {
        ChamplainBoundingBox *bounding_box;

        champlain_view_set_zoom_level(view, 15);

        bounding_box = champlain_bounding_box_new();
		champlain_bounding_box_extend(bounding_box,lat1,lon1); 
		champlain_bounding_box_extend(bounding_box,lat2, lon2); 
		champlain_view_ensure_visible(view, bounding_box, FALSE);

        champlain_bounding_box_free(bounding_box);
    }
    gtk_window_set_application(GTK_WINDOW(gtk_builder_get_object(priv->builder,"gpx_viewer_window")), gtk_app);
}


void open_gpx_file(GtkMenu *item, GpxViewer *gpx_viewer)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(gpx_viewer);
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

    path = gpx_viewer_settings_get_string(priv->settings, "open-dialog", "last-dir", NULL);
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
            GtkTreeModel *model = (GtkTreeModel *) gtk_builder_get_object(priv->builder, "routes_store");
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
				gtk_recent_manager_add_item(GTK_RECENT_MANAGER(priv->recent_man), (gchar *)iter->data);
                /* Try to open the gpx file */
                file = gpx_file_new(afile);
                priv->files = g_list_append(priv->files, file);
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
                        interface_plot_add_track(gpx_viewer,&liter, track_iter->data, &lat1, &lon1, &lat2, &lon2);
                    }
                }
                if(file->routes)
                {
                    GList *route_iter;
                    for (route_iter = g_list_first(file->routes); route_iter; route_iter = g_list_next(route_iter))
                    {
                        interface_plot_add_track(gpx_viewer,&liter, route_iter->data, &lat1, &lon1, &lat2, &lon2);
                    }
                }
                interface_create_fake_master_track(file, &liter,gpx_viewer);
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
        gpx_viewer_settings_set_string(priv->settings, "open-dialog" , "last-dir" , path);
        g_free(path);
    }
    gtk_widget_destroy(dialog);
    g_object_unref(fbuilder);
}


static void gpx_viewer_activate (GpxViewer *object)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(object);
	if(priv->settings == NULL)
	{
		/* Setup responding to commands */
		priv->settings = gpx_viewer_settings_new();

		/* REcent manager */
		priv->recent_man = gtk_recent_manager_get_default();

		/* Create playback option */
		priv->playback = gpx_playback_new(NULL);
		/* Connect settings */
		gpx_viewer_settings_add_object_property(priv->settings,
				G_OBJECT(priv->playback), "speedup");
		/* Watch signals */
		g_signal_connect(GPX_PLAYBACK(priv->playback),
				"tick",
				G_CALLBACK(route_playback_tick),
				object);
		g_signal_connect(GPX_PLAYBACK(priv->playback),
				"state-changed",
				G_CALLBACK(route_playback_state_changed),
				object);

		/* Create interface */
		create_interface(GTK_APPLICATION(object));
	}
}

static void
gpx_viewer_finalize (GObject *object)
{
	GpxViewerPrivate * priv = GPX_VIEWER_GET_PRIVATE(object);

	/* Destroy the settings object */
	g_debug("destroying Settings");
	if(priv->settings) {
		g_object_unref(priv->settings);
		priv->settings = NULL;
	}
	if(priv->recent_man) {
		// Do not free
		priv->recent_man = NULL;
	}
	/* unref playback object */
	g_debug("Destroying playback");
	if(priv->playback) {
		g_object_unref(priv->playback);
		priv->playback = NULL;
	}
	/* Destroy the files */
	g_debug("Cleaning up files");
	if(priv->files)
	{
		g_list_foreach(g_list_first(priv->files), (GFunc) g_object_unref, NULL);
		g_list_free(priv->files);
		priv->files = NULL;
	}
	/* Class */
	G_OBJECT_CLASS (gpx_viewer_parent_class)->finalize (object);
}

static void gpx_viewer_init (GpxViewer *app)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(app);
	priv->settings = NULL;
	priv->recent_man = NULL;
	priv->playback = NULL;
	priv->click_marker_source= 0;
	priv->files = NULL;
	priv->builder = NULL;
	priv->gpx_graph = NULL;
}

static void gpx_viewer_open(GpxViewer *app, GFile **input_files, gint n_files, const gchar *hint)
{
	int i = 0;
	GtkTreeModel *model;
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(app);

	gpx_viewer_activate(app);
	model = (GtkTreeModel *) gtk_builder_get_object(priv->builder, "routes_store");

	for(i=0; i < n_files;i++)
	{
		GpxFile *file;
		gchar *basename;
		gchar *filename;
		GtkTreeIter liter;
		double lon1 = 1000, lon2 = -1000, lat1 = 1000, lat2 = -1000;
		/* Create a GFile */
		/* Try to open the gpx file */
		file = gpx_file_new(input_files[i]);
		priv->files = g_list_append(priv->files, file);
		/* Add entry to recent manager */
		filename = g_file_get_uri(file->file);
		gtk_recent_manager_add_item(GTK_RECENT_MANAGER(priv->recent_man),filename); 

		basename = g_file_get_basename(file->file);
		gtk_tree_store_append(GTK_TREE_STORE(model), &liter, NULL);
		gtk_tree_store_set(GTK_TREE_STORE(model), &liter,
				0, basename,
				1, NULL,
				2, FALSE,
				3, FALSE,
				-1);
		g_free(basename);
		g_free(filename);
		if (file->tracks)
		{
			GList *track_iter;
			for (track_iter = g_list_first(file->tracks); track_iter; track_iter = g_list_next(track_iter))
			{
				interface_plot_add_track(app,&liter, track_iter->data, &lat1, &lon1, &lat2, &lon2);
			}
		}
		if(file->routes)
		{
			GList *route_iter;
			for (route_iter = g_list_first(file->routes); route_iter; route_iter = g_list_next(route_iter))
			{
				interface_plot_add_track(app,&liter, route_iter->data, &lat1, &lon1, &lat2, &lon2);
			}
		}
		interface_create_fake_master_track(file, &liter, app);
	}
}

static void
gpx_viewer_class_init (GpxViewerClass *class)
{
	g_type_class_add_private(class, sizeof(GpxViewerPrivate));
	G_OBJECT_CLASS (class)->finalize= gpx_viewer_finalize;
	G_OBJECT_CLASS (class)->constructed = gpx_viewer_init;

	G_APPLICATION_CLASS (class)->activate = gpx_viewer_activate;
	G_APPLICATION_CLASS (class)->open = gpx_viewer_open;
}




static GpxViewer * gpx_viewer_new (void)
{
  return g_object_new (gpx_viewer_get_type (),
                       "application-id", "nl.sarine.gpx-viewer",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

/*******************************************************************
 * Main program
 *******************************************************************/
int main(int argc, char **argv)
{
	int retv;
	GpxViewer *app = NULL;
	gchar *path;

	/* setup translation */
	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
	setlocale (LC_ALL, "");

	gtk_clutter_init(&argc, &argv);

	/* Add own icon strucutre to the theme engine search */
	path = g_build_filename(DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
			path);
	g_free(path);

	if(!g_thread_supported())
	{
		g_thread_init(NULL);
	}


	app = gpx_viewer_new();	

	retv = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);
	return retv;
}


/**
 * Track list viewer
 */

void close_show_current_track(GtkWidget *widget,gint response_id,gpointer user_data) 
{

//	GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(fbuilder, "track_list_dialog"));
	gtk_widget_destroy(widget);

	// TODO: How to free these now?
//	g_object_unref(fbuilder);
}


void show_current_track(GtkWidget *menu_item, gpointer user_data)
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
		gtk_builder_connect_signals(fbuilder, user_data);
		gtk_widget_show(GTK_WIDGET(dialog));
		g_object_unref(fbuilder);
	}
}


/**
 * Settings
 */
void gpx_viewer_preferences_close(GtkWidget *dialog, gint respose, gpointer user_data) 
{
	gtk_widget_destroy(dialog);
	// TODO: How to free these now?
//	g_object_unref(fbuilder);
}


void playback_speedup_spinbutton_value_changed_cb(GtkSpinButton *sp, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
	gint value = gtk_spin_button_get_value_as_int(sp);
	gpx_playback_set_speedup(priv->playback, value);
}


void gpx_viewer_show_preferences_dialog(GtkWidget *menu_item, gpointer user_data)
{
	GpxViewerPrivate *priv = GPX_VIEWER_GET_PRIVATE(user_data);
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
	g_signal_connect_object(G_OBJECT(champlain_view), "notify::show-waypoints", G_CALLBACK(show_waypoints_layer_changed),
			widget,0);
	show_waypoints_layer_changed(NULL, NULL, widget);

	/** Graph **/
	/* smooth factor */
	widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"spin_button_smooth_factor");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), (double)gpx_graph_get_smooth_factor(priv->gpx_graph));
	g_signal_connect_object(priv->gpx_graph, "notify::smooth-factor", G_CALLBACK(smooth_factor_changed), widget,0);

	/* Show points */
	widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"check_button_data_points");
	g_signal_connect_object(priv->gpx_graph, "notify::show-points", G_CALLBACK(graph_show_points_changed), widget,0);
	graph_show_points_changed(widget, NULL, widget);

	/* speedup */
	widget = (GtkWidget *)gtk_builder_get_object(fbuilder,"playback_speedup_spinbutton");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
			(double)gpx_playback_get_speedup(priv->playback));

	gtk_builder_connect_signals(fbuilder, user_data);
	gtk_widget_show(GTK_WIDGET(dialog));
	g_object_unref(fbuilder);
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

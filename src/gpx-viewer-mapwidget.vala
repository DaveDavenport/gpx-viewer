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
using Champlain;
using Gtk;
using GLib;
using Unique;

namespace Gpx
{
    namespace Viewer
    {
        public class MapView : Gtk.ChamplainEmbed
        {
            /* Color */
            private Clutter.Color waypoint_color;


            /* Waypoint layer */
            private Champlain.Layer waypoint_layer = new Champlain.Layer();
            private bool _show_waypoints = false;
            public bool show_waypoints {
                    get { return _show_waypoints;}
                    set {
                        if(value) {
                            this.waypoint_layer.show();
                        }else{
                            this.waypoint_layer.hide();
                        }
                        this._show_waypoints = value;
                    }
            }

            public void add_waypoint(Gpx.Point p)
            {
                Champlain.Marker marker = new Marker.with_text(p.name, "Serif 12", null, waypoint_color);
                marker.set_position(p.lat_dec, p.lon_dec);
                waypoint_layer.add(marker);
            }
            /* Marker layer */
            private Champlain.Layer marker_layer = new Champlain.Layer();

            private bool _show_markers = false;
            public bool show_markers {
                    get { return _show_markers;}
                    set {
                        if(value) {
                            this.marker_layer.show();
                        }else{
                            this.marker_layer.hide();
                        }
                    }
            }

            public void add_marker(BaseMarker marker)
            {
                marker_layer.add(marker);
            }


            /* A TreeModel with all the maps, <name>,<id> */
            private Gtk.ListStore map_source_list = new Gtk.ListStore(2,typeof(string), typeof(string));

            private string _map_source = null;
            public string map_source {
                get { return _map_source; }
                set {
                    if(value == _map_source) return;
                    this.switch_map_source(value);
                }
            }
            /* Get the model (for use in combo box and equal) */
            public Gtk.TreeModel get_model()
            {
                return map_source_list as Gtk.TreeModel;
            }
            /* construction */
            public MapView ()
            {
                this.waypoint_color.red = 0xf3;
                this.waypoint_color.green = 0x94;
                this.waypoint_color.blue = 0x07;
                this.waypoint_color.alpha =0xff;
                /* Do default setup of the view. */
                /* We want kinetic scroling. */
                this.view.scroll_mode = Champlain.ScrollMode.KINETIC;
                /* We do want to show the scale */
                this.view.show_scale = true;

                /* Create a ListStore with all the available maps. Used for selectors */
                var fact= new Champlain.MapSourceFactory.dup_default();
                var l = fact.dup_list();
                foreach(weak MapSourceDesc a in l)
                {
                    Gtk.TreeIter iter;
                    /* If no map set, pick the first one */
                    if(this._map_source == null) {
                        this._map_source = a.id;
                    }
                    /* Add the available map's to the list */
                    map_source_list.append(out iter);
                    map_source_list.set(iter, 0, a.name, 1, a.id);
                }
                /* Keep track of changed zoom level, and signal this */
                this.view.notify["zoom-level"].connect(()=>{
                    zoom_level_changed(this.view.zoom_level, this.view.min_zoom_level,this.view.max_zoom_level);
                });
                this.view.add_layer(waypoint_layer);
                this.view.add_layer(marker_layer);
            }

            signal void zoom_level_changed(int zoom, int min_zoom, int max_zoom);

            private void switch_map_source(string id)
            {
                var fact= new Champlain.MapSourceFactory.dup_default();
                Champlain.MapSource source = fact.create_cached_source(id);
                if(source != null)
                {
                    this.view.set_map_source(source);
                    this._map_source = id;
                    zoom_level_changed(this.view.zoom_level, this.view.min_zoom_level,this.view.max_zoom_level);
                }else{
                    GLib.error("Failed to get map source");
                }
            }
            /* Destroy */
            ~MapView ()
            {
                GLib.debug("Destroying map-view");
            }
        }

    }
}

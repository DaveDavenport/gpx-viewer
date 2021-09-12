/* Gpx Viewer
 * Copyright (C) 2009-2015 Qball Cow <qball@sarine.nl>
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

namespace Gpx
{
    namespace Viewer
    {
        public class MapView : GtkChamplain.Embed
		{
			private unowned Champlain.View view = null;
			/* Color */
			private Clutter.Color waypoint_color;

			/*  Marker  */
			private Champlain.Marker click_marker = null;




			public Champlain.Marker? create_marker(Gpx.Point p, string icon, int size)
			{
				Champlain.Label? marker = null;
				var info = Gtk.IconTheme.get_default().lookup_icon(
						icon,
						size,0);
				if(info != null)
				{
					var path = info.get_filename();
					if(path != null){
						try {
							marker = new Champlain.Label.from_file(path);
						} catch (GLib.Error e) {
							GLib.warning ("%s", e.message);
						}
						marker.set_draw_background(false);
					}
				}
				else stdout.printf("Info is null: %s\n", icon);
				if(marker == null) {
					marker = new Champlain.Label();
				}
				marker.set_size((float)size, (float)size);
				//marker.set_translation(-(float)size/2.0f, -(float)size/2.0f, 0);
				marker.set_location((float)p.lat_dec,(float)p.lon_dec);
				return marker;
			}
			/**
			 * Show marker at Point
			 */
			public void click_marker_show(Gpx.Point p)
			{
				if(click_marker == null) 
				{
					click_marker = create_marker(p, "pin-blue",100);
					this.add_marker(click_marker);
					click_marker.show();
				}else{         
					click_marker.set_location((float)p.lat_dec,(float)p.lon_dec);
					click_marker.show();
				}
			}
			public void click_marker_hide()
			{
				click_marker.hide();
			}
			public void increase_zoom_level()
			{
				get_view().zoom_level += 1;
			}
			public void decrease_zoom_level()
			{
				get_view().zoom_level -= 1;
			}



			bool markers_added = false;

			public void add_layer(Champlain.Layer layer) {
				var view = get_view();
				view.add_layer(layer);
				if (!markers_added) {
				    view.add_layer(waypoint_layer);
				    view.add_layer(marker_layer);
				}
			}
			/* Waypoint layer */
			private Champlain.MarkerLayer waypoint_layer = new Champlain.MarkerLayer();
			private bool _show_waypoints = false;
			public bool show_waypoints {
				get { return _show_waypoints;}
				set {
					this._show_waypoints = value;
					this.waypoint_layer.visible = _show_waypoints;
				}
			}

			public void add_waypoint(Gpx.Point p)
			{
				Champlain.Marker marker = new Champlain.Label.with_text(p.name, "Serif 12", null, waypoint_color);
				marker.set_location(p.lat_dec, p.lon_dec);
				waypoint_layer.add_marker(marker);
			}
			/* Marker layer */
			private Champlain.MarkerLayer marker_layer = new Champlain.MarkerLayer();

			private bool _show_markers = true;
			public bool show_markers {
				get { return _show_markers;}
				set {
					_show_markers = value;
					marker_layer.visible = _show_markers;
				}
			}

			public void add_marker(Champlain.Marker marker)
			{
				marker_layer.add_marker(marker);
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
				view = this.get_view();
				stdout.printf("MapView init\n");
				/* Setup waypoint color */
				this.waypoint_color.red = 0xf3;
				this.waypoint_color.green = 0x94;
				this.waypoint_color.blue = 0x07;
				this.waypoint_color.alpha =0xff;

				/* Do default setup of the view. */
				/* We want kinetic scroling. */
				view.kinetic_mode = true;

				/* Create a ListStore with all the available maps. Used for selectors */
				var fact = Champlain.MapSourceFactory.dup_default();
				var l = fact.get_registered();
				foreach(weak Champlain.MapSourceDesc a in l)
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
				view.notify["zoom-level"].connect(()=>{
						zoom_level_changed(view.zoom_level,
							view.min_zoom_level,
							view.max_zoom_level);
						});
				marker_layer.show();
				/* Set it to recieve signals */
				view.reactive = true;
				view.button_release_event.connect(button_press_callback);
			}

			private bool button_press_callback(Clutter.ButtonEvent event)
			{
				var default_modifiers = Gtk.accelerator_get_default_mod_mask ();
				if(event.button == Gdk.BUTTON_PRIMARY &&
				   (event.modifier_state & Clutter.ModifierType.CONTROL_MASK) == Clutter.ModifierType.CONTROL_MASK)
				{
					double lat,lon;
					lat = view.y_to_latitude (event.y);
					lon = view.x_to_longitude (event.x);
					clicked(lat,lon);
					return true;
				}
				return false;
			}

			private void switch_map_source(string id)
			{
				var fact= Champlain.MapSourceFactory.dup_default();
				Champlain.MapSource source = fact.create_cached_source(id);
				if(source != null)
				{
					view.set_map_source(source);
					this._map_source = id;
					zoom_level_changed(
							view.zoom_level,
							view.min_zoom_level,
							view.max_zoom_level);
				}else{
					GLib.error("Failed to get map source");
				}
			}

			/* Destroy */
			~MapView ()
			{
				GLib.debug("Destroying map-view");
			}

			/* Signals */
			/**
			 * @param lat_dec the latitude of the click point in dec.
			 * @param lon_dec the longitude of the click point.
			 * 
			 * Fired when the users right or middle clicks on the  map.
			 */
			signal void clicked(double lat_dec, double lon_dec);

			/**
			 * @param zoom the current zoom level.
			 * @param min_zoom the minimum supported zoom level.
			 * @param max_zoom the maximum supported zoom level.
			 * 
			 * Fired when the zoomlevel changed.
			 */
			signal void zoom_level_changed(uint zoom, uint min_zoom, uint max_zoom);

		}

    }
}

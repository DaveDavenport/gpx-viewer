/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2011 Jiri Techet <techet@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/**
 * TODO:
 * Accept GpxTrack, not points.
 * Then remove object type from point.
 */
/**
 * SECTION:champlain-path-layer
 * @short_description: A layer displaying line path between inserted #GpxPoint
 * objects
 *
 * This layer shows a connection between inserted objects implementing the
 * #GpxPoint interface. This means that both #ChamplainMarker
 * objects and #ChamplainCoordinate objects can be inserted into the layer.
 * Of course, custom objects implementing the #GpxPoint interface
 * can be used as well.
 */

namespace Gpx.Viewer
{
	struct Color
	{
		double r;
		double g;
		double b;
	}

	const Color HEIGHT_COLORS[] =
	{
		{ 0.0, 0.0, 1.0 },
		{ 0.0, 1.0, 1.0 },
		{ 0.0, 1.0, 0.0 },
		{ 1.0, 0.8, 0.0 },
		{ 1.0, 0.0, 0.0 }
	};

	const Clutter.Color DEFAULT_STROKE_COLOR = { 0xa4, 0x00, 0x00, 0xff };

	public class PathLayer : Champlain.Layer
	{
		public Clutter.Color? stroke_color {
			get { return _stroke_color; }
			set {
				if (value == null) {
					_stroke_color = DEFAULT_STROKE_COLOR;
				} else {
					_stroke_color = value;
				}
				schedule_redraw ();
			}
		}

		public double stroke_width {
			get {
				return _stroke_width;
			}
			set {
				_stroke_width = value;
				schedule_redraw ();
			}
		}

		public bool visible {
			get {
				return _visible;
			}
			set {
				_visible = value;
				if (value) {
					path_actor.show ();
				} else {
					path_actor.hide ();
				}
			}
		}

		public Gpx.Track track {
			set {
				_track = value;
				_track.point_removed.connect(() => {
					schedule_redraw ();
				});
				schedule_redraw ();
			}
		}

		Clutter.Color? _stroke_color;
		double _stroke_width;

		Gpx.Track _track;
		bool _visible;
		Champlain.View view;
		Clutter.Group content_group;
		Clutter.CairoTexture path_actor;
		bool redraw_scheduled;

		construct
		{
			_stroke_color = DEFAULT_STROKE_COLOR;
			_stroke_width = 2.0;
			_visible = true;

			redraw_scheduled = false;

			content_group = new Clutter.Group ();
			content_group.set_parent (this);
			path_actor = new Clutter.CairoTexture (256, 256);
			content_group.add_actor (path_actor);

			queue_relayout ();
		}

		public override void paint ()
		{
			content_group.paint ();
		}

		public override void pick (Clutter.Color color)
		{
			base.pick (color);
			content_group.paint ();
		}

		public override void get_preferred_width (float for_height, out float min_width_p, out float natural_width_p)
		{
			content_group.get_preferred_width (for_height, out min_width_p, out natural_width_p);
		}

		public override void get_preferred_height (float for_width, out float min_height_p, out float natural_height_p)
		{
			content_group.get_preferred_height (for_width, out min_height_p, out natural_height_p);
		}

		public override void allocate (Clutter.ActorBox box, Clutter.AllocationFlags flags)
		{
			base.allocate (box, flags);
			Clutter.ActorBox child_box = { 0.0f, 0.0f, box.x2 - box.x1, box.y2 - box.y1 };
			content_group.allocate (child_box, flags);
		}

		public override void map ()
		{
			base.map ();
			content_group.map ();
		}

		public override void unmap ()
		{
			base.unmap ();
			content_group.unmap ();
		}

		public override void dispose ()
		{
			if (view != null) {
				set_view (null);
			}
			if (content_group != null) {
				content_group.unparent ();
				content_group = null;
			}

			base.dispose ();
		}

		bool redraw_path ()
		{
			redraw_scheduled = false;

			/* layer not yet added to the view */
			if (view == null || content_group == null)
				return false;

			float width, height;
			view.get_size (out width, out height);

			if (!visible || width == 0.0f || height == 0.0f)
				return false;

			uint last_width, last_height;
			path_actor.get_surface_size (out last_width, out last_height);

			if ((uint) width != last_width || (uint) height != last_height) {
				path_actor.set_surface_size ((uint) width, (uint) height);
			}

			int x, y;
			view.get_viewport_origin (out x, out y);
			path_actor.set_position (x, y);

			var cr = path_actor.create ();

			/* Clear the drawing area */
			cr.set_operator (Cairo.Operator.CLEAR);
			cr.paint ();
			cr.set_operator (Cairo.Operator.OVER);

			// For colouring.. only > 50 meters difference.
			double min_elv, max_elv, range;
			min_elv = _track.min_elevation;
			max_elv = _track.max_elevation;
			range = double.max (250.0, max_elv - min_elv);

			cr.set_line_width (stroke_width);
			cr.set_line_cap (Cairo.LineCap.ROUND);
			cr.set_line_join (Cairo.LineJoin.ROUND);

			int old_val = -1;
			foreach (var location in _track.points)
			{
				int val = (int) Math.round ((HEIGHT_COLORS.length - 1) * (location.elevation - min_elv) / range);
				float c_x, c_y;
				c_x = (float) view.longitude_to_x (location.lon_dec);
				c_y = (float) view.latitude_to_y (location.lat_dec);
				if (val != old_val) {
					cr.line_to (c_x, c_y);
					cr.stroke ();
					cr.set_source_rgb (HEIGHT_COLORS[val].r, HEIGHT_COLORS[val].g, HEIGHT_COLORS[val].b);
					old_val = val;
					cr.move_to (c_x, c_y);
				} else {
					cr.line_to (c_x, c_y);
				}
			}

			cr.stroke ();

			return false;
		}

		void schedule_redraw ()
		{
			if (!redraw_scheduled) {
				redraw_scheduled = true;
				Idle.add_full (Clutter.PRIORITY_REDRAW, redraw_path);
			}
		}

		ulong view_relocate_handler_id = 0U;
		ulong view_latitude_handler_id = 0U;

#if CHAMPLAIN_0_12_21
		public override void set_view (Champlain.View? view)
#else
		public override void set_view (Champlain.View view)
#endif
		{
			if (this.view != null) {
				this.view.disconnect (view_relocate_handler_id);
				this.view.disconnect (view_latitude_handler_id);
			}

			this.view = view;

			if (view != null) {
				view_relocate_handler_id = view.layer_relocated.connect (() => schedule_redraw ());
				view_latitude_handler_id = view.notify["latitude"].connect (() => schedule_redraw ());
				schedule_redraw ();
			}
		}

		public override Champlain.BoundingBox get_bounding_box ()
		{
			var bbox = new Champlain.BoundingBox ();
			foreach (var point in _track.points) {
				bbox.extend (point.lat_dec, point.lon_dec);
			}

			if (bbox.left == bbox.right) {
				bbox.left -= 0.0001;
				bbox.right += 0.0001;
			}

			if (bbox.bottom == bbox.top) {
				bbox.bottom -= 0.0001;
				bbox.top += 0.0001;
			}

			return bbox;
		}
	}
}

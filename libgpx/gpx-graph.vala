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
	const string LOG_DOMAIN = "GPX_GRAPH";

	public class Graph: Gtk.EventBox
	{
		/* Public */
		/* Holds the track */
		public Gpx.Track track = null;
		private Gpx.Point start = null;
		private Gpx.Point stop = null;
		private Gpx.Point selected = null;

		public enum  GraphMode {
			SPEED,
			ELEVATION,
			DISTANCE,
			TOTAL_DISTANCE,
			ACCELERATION_H,
			SPEED_V,
            HEARTRATE,
            CADENCE,
			NUM_GRAPH_MODES
		}
		/* Privates */
		static string[] GraphModeName = {
			N_("Speed (km/h) vs Time (HH:MM)"),
			N_("Elevation (m) vs Time (HH:MM)"),
			N_("Absolute Distance (km) vs Time (HH:MM)"),
			N_("Horizontal acceleration (m/s²) vs Time (HH:MM)"),
			N_("Vertical speed (m/s) vs Time (HH:MM)"),
            N_("Heart-rate (bpm) vs Time (HH:MM)"),
            N_("Cadence (rpm) vs Time (HH:MM)")
		};
		static string[] GraphModeMiles = {
			N_("Speed (Miles/h) vs Time (HH:MM)"),
			N_("Elevation (feet) vs Time (HH:MM)"),
			N_("Distance (Miles) vs Time (HH:MM)"),
			N_("Horizontal acceleration (Miles/s²) vs Time (HH:MM)"),
			N_("Vertical speed (feet/s) vs Time (HH:MM)"),
            N_("Heart-rate (bpm) vs Time (HH:MM)"),
            N_("Cadence (rpm) vs Time (HH:MM)")
		};

		private bool _do_miles = false;

		/* By default elevation is shown */
		private GraphMode _mode = GraphMode.ELEVATION;
		/* By default no smoothing is applied */
		private int _smooth_factor = 1;
		/* By default points are shown on graph */
		private bool _show_points = true;

		private Pango.FontDescription fd = null;
		private Cairo.Surface surf = null;
		private int LEFT_OFFSET = 60;
		private int RIGHT_OFFSET = 10;
		private int BOTTOM_OFFSET = 30;
		private time_t highlight = 0;

		private weak Gpx.Point? draw_current = null;


		public void hide_info()
		{
			draw_current = null;
			this.queue_draw();
		}
		public void show_info(Gpx.Point? cur_point)
		{
			this.draw_current = cur_point;
			this.queue_draw();
		}
		
		/** 
		 * @param p A Gpx.Point we want to highlight.
		 * pass null to unhighlight.
		 */
		public void highlight_point(Gpx.Point? p) 
		{
			time_t pt = (p == null)?0:p.get_time();
			this.set_highlight(pt);
		}
		public void set_highlight (time_t highlight) {
			this.highlight = highlight;
			/* Force a redraw */
			this.queue_draw();
		}

		public int smooth_factor {
			get { return _smooth_factor;}
			set {
				_smooth_factor = value;
				/* Invalidate the previous plot, so it is redrawn */
				this.surf = null;
				/* Force a redraw */
				this.queue_draw();
			}
		}

		public bool show_points {
			get { return _show_points;}
			set {
				_show_points = value;
				/* Invalidate the previous plot, so it is redrawn */
				this.surf = null;
				/* Force a redraw */
				this.queue_draw();
			}
		}

		public GraphMode mode {
			get {return _mode;}
			set {
				if(value != this._mode)
				{
					this._mode= value;
					this.surf = null;
					/* Force a redraw */
					this.queue_draw();
				}
			}
		}

		public Graph ()
		{
			/* Create and setup font description */
			this.fd = new Pango.FontDescription();
			fd.set_family("sans mono");
			/* make the event box paintable and give it an own window to paint on */
			this.app_paintable = true;
			this.visible_window = true;
			/* signals */
			this.size_allocate.connect(size_allocate_cb);
			this.button_press_event.connect(button_press_event_cb);
			this.motion_notify_event.connect(motion_notify_event_cb);
			this.button_release_event.connect(button_release_event_cb);

			this.draw.connect(a_expose_event);
		}

		public void set_track(Gpx.Track? track)
		{
			this.highlight = 0;
			this.track =track;
			/* Invalidate the previous plot, so it is redrawn */
			this.surf = null;
			/* Force a redraw */
			this.queue_draw();
			this.start = null;
			this.stop = null;
			/* */
			if(this.track != null && this.track.points != null)
				selection_changed(this.track, this.track.points.first().data, this.track.points.last().data);
			else
				selection_changed(this.track, null, null);
		}
		/* Signal if the users clicks a point  */
		signal void point_clicked(Gpx.Point point);

		/* signal if the selection range changes */
		signal void selection_changed(Gpx.Track? track, Gpx.Point? start, Gpx.Point? stop);
		/**
		 * Private functions
		 */
		private Gpx.Point? get_point_from_position(double x, double y)
		{
			Gtk.Allocation alloc;
			if(this.track == null) return null;
			this.get_allocation(out alloc);
			if(x <= LEFT_OFFSET)
			{
				return track.points.first().data;
			}
			if(x >= (alloc.width-RIGHT_OFFSET))
			{
				return track.points.last().data;
			}
			if(x > LEFT_OFFSET && x < (alloc.width-RIGHT_OFFSET))
			{
				double elapsed_time = track.get_total_time();
				time_t time = (time_t)((x-LEFT_OFFSET)/(alloc.width-RIGHT_OFFSET-LEFT_OFFSET)*elapsed_time);
				weak List<Point?> iter = this.track.points.first();
				/* calculated time is offset from start time,  get real time */
				time += iter.data.get_time();
				while(iter.next != null)
				{
					var next_time = iter.next.data.get_time();
					var cur_time = iter.data.get_time();
					var mid_next_time = (cur_time + next_time) / 2;
					var mid_prev_time = cur_time;

					if (iter.prev != null) {
						var prev_time = iter.prev.data.get_time();
						mid_prev_time = (prev_time + cur_time) / 2;
					}
					if(time >= mid_prev_time && time <= mid_next_time)
					{
						return iter.data;
					}
					iter = iter.next;
				}
			}
			return null;
		}
		public bool select_next_point() {
			if (this.selected == null) {
				this.selected = this.track.points.first().data;
			}
			var pos = this.track.points.index(selected);
			if (pos < this.track.points.length() - 1 ) {
				this.selected = this.track.points.nth_data(pos + 1);
				point_clicked(selected);
			}
			return true;
		}
		public bool select_prev_point() {
			if (this.selected == null) {
				this.selected = this.track.points.last().data;
			}
			var pos = this.track.points.index(selected);
			if (pos > 0 ) {
				this.selected = this.track.points.nth_data(pos - 1);
				point_clicked(selected);
			}
			return true;
		}
		public void remove_selected() {
			if (start != null && stop != null) {
				int start_pos = track.points.index(start);
				int end_pos = track.points.index(stop);
				if (end_pos < start_pos) {
					int tmp_pos = end_pos;
					end_pos = start_pos;
					start_pos = tmp_pos;
				}
				for (int i = start_pos; i < end_pos; i++) {
					var p = track.points.nth_data(start_pos);
					track.remove_point(p, false);
				}
				selected = track.points.nth_data(start_pos);
				remove_selected_point();
			} else if (this.selected != null) {
				remove_selected_point();
			}
		}
		public void remove_selected_point() {
			if (this.selected != null) {
				var pos = this.track.points.index(selected);
				var last = this.track.points.length() - 1;
				if (pos == last) {
					pos--;
				}
				this.track.remove_point(selected);
				this.selected = this.track.points.nth_data(pos);
				this.surf = null;
				point_clicked(selected);

				selection_changed(track, 
								  start != null
								     ? start
								     : track.points.first().data,
								  stop != null
								     ? stop
								     : track.points.last().data);
				queue_draw();
			}
		}
		public void remove_non_selected_range() {
			if (start != null && stop != null) {
				int start_pos = track.points.index(start);
				int end_pos = track.points.index(stop);
				var last_pos = track.points.length() - 1;
				if (start_pos == end_pos)
					return;

				if (end_pos < start_pos) {
					int tmp_pos = end_pos;
					end_pos = start_pos;
					start_pos = tmp_pos;
				}

				for (int i = 0; i < start_pos; i++) {
					var p = track.points.nth_data(0);
					track.remove_point(p, false);
				}
				end_pos -= start_pos - 1;
				last_pos += start_pos - 1;
				print("removing trail %d to %u\n", end_pos, last_pos);
				for (int i = end_pos; i <= last_pos; i++) {
					var p = track.points.nth_data(end_pos);
					track.remove_point(p, false);
				}
				start = null;
				stop = null;
				selected = track.points.nth_data(0);
				remove_selected_point();
			}
		}
		private bool button_press_event_cb(Gdk.EventButton event)
		{
			if(this.track == null) return true;
			Gpx.Point *point = get_point_from_position(event.x, event.y);
			if(point != null) {
				if(event.button == Gdk.BUTTON_PRIMARY) {
					this.start = null;
					this.selected = point;
					point_clicked(point);
				} else if(event.button == Gdk.BUTTON_SECONDARY) {
					bool range_selected = start != null && stop != null;
					bool point_selected = selected != null;
					Gtk.Menu menu = new Gtk.Menu ();
					Gtk.AccelGroup accel_group = new Gtk.AccelGroup();

					if (range_selected || point_selected) {
					    menu.attach_to_widget (this, null);
					    Gtk.MenuItem menu_item = new Gtk.MenuItem.with_label (range_selected ? "Remove range": "Remove point");
					    menu_item.activate.connect(() => {
					        remove_selected();
					    });
					    menu_item.add_accelerator("activate", accel_group, Gdk.Key.Delete, 0, Gtk.AccelFlags.VISIBLE);
					    menu_item.add_accelerator("activate", accel_group, Gdk.Key.BackSpace, 0, Gtk.AccelFlags.VISIBLE);
					    menu.add (menu_item);
					}
					if (range_selected) {
					    Gtk.MenuItem menu_item = new Gtk.MenuItem.with_label ("Remove non selected range");
					    menu_item.activate.connect(() => {
					        remove_non_selected_range();
					    });
					    menu_item.add_accelerator("activate", accel_group, Gdk.Key.Delete, Gdk.ModifierType.SHIFT_MASK, Gtk.AccelFlags.VISIBLE);
					    menu.add (menu_item);
					}
					if (range_selected || point_selected) {
					    menu.show_all ();
					    menu.popup (null, null, null, event.button, event.time);
					}
				}
			}
			return true;
		}

		private bool motion_notify_event_cb(Gdk.EventMotion event)
		{
			if(this.track == null) return true;

			Gpx.Point *point = this.get_point_from_position(event.x, event.y);
			if(point != null)
			{
				if (start == null)
					this.start = point;
				this.stop = point;
				/* queue redraw so the selection is updated */
				this.queue_draw();
				if(this.start != null && this.stop  != null)
				{
					if(start.get_time() != stop.get_time())
					{
						if(start.get_time() < stop.get_time()) {
							selection_changed(this.track, start, stop);
						} else {
							selection_changed(this.track, stop, start);
						}
						return true;
					}
				}
				selection_changed(this.track, this.track.points.first().data, this.track.points.last().data);
			}
			return true;
		}
		private bool button_release_event_cb(Gdk.EventButton event)
		{
			if(this.track == null) return true;
			Gpx.Point *point = this.get_point_from_position(event.x, event.y);
			if(point != null)
			{
				if(event.button == Gdk.BUTTON_PRIMARY)
					this.stop = point;
				else this.stop = null;
				this.queue_draw();
				if(event.button == Gdk.BUTTON_PRIMARY)
				{
					if(this.start != null && this.stop  != null)
					{
						if(start.get_time() != stop.get_time())
						{
							if(start.get_time() < stop.get_time()) {
								selection_changed(this.track, start, stop);
							} else {
								selection_changed(this.track, stop, start);
							}
							return true;
						}
					}
					selection_changed(this.track, this.track.points.first().data, this.track.points.last().data);
				}
			}
			return true;
		}
		private void size_allocate_cb(Gtk.Allocation alloc)
		{
			/* Invalidate the previous plot, so it is redrawn */
			this.surf = null;
		}

		bool a_expose_event(Cairo.Context ctx)
		{
			//var ctx = Gdk.cairo_create(this.get_window());
			/* If no valid surface, render it */
			if(surf == null)
				update_surface(this);

			/* Get allocation */
			Gtk.Allocation alloc;
			this.get_allocation(out alloc);
			/* Draw the actual surface on the widget */
			ctx.set_source_surface(this.surf, 0, 0);
			ctx.paint();

			ctx.translate(LEFT_OFFSET,20);
			/* Draw selection, if available */
			if(start != null && stop != null)
			{
				if(start.get_time() != stop.get_time())
				{
					Gpx.Point f = this.track.points.first().data;
					double elapsed_time = track.get_total_time();
					double graph_width = alloc.width-LEFT_OFFSET-RIGHT_OFFSET;
					double graph_height = alloc.height-20-BOTTOM_OFFSET;

					ctx.set_source_rgba(0.3, 0.2, 0.3, 0.8);
					ctx.rectangle((start.get_time()-f.get_time())/elapsed_time*graph_width, 0,
							(stop.get_time()-start.get_time())/elapsed_time*graph_width, graph_height);
					ctx.stroke_preserve();
					ctx.fill();
				}

			}
			if(highlight > 0 )
			{
				Gpx.Point f = this.track.points.first().data;
				double elapsed_time = track.get_total_time();
				double graph_width = alloc.width-LEFT_OFFSET-RIGHT_OFFSET;
				double graph_height = alloc.height-20-BOTTOM_OFFSET;

				double hl = (highlight-f.get_time())/elapsed_time*graph_width;

				ctx.set_source_rgba(0.8, 0.2, 0.3, 0.8);
				ctx.move_to(hl, 0);
				ctx.line_to(hl,graph_height);

				ctx.stroke_preserve();
				ctx.fill();
				/* Draw the speed/elavation/distance
				 * in the upper top corner
				 */
				if(this.draw_current != null)
				{
					var layout = Pango.cairo_create_layout(ctx);
					int w,h;
					var text = "";
					var x_pos =0.0;

					text = _("Speed")+":\t"+       Gpx.Viewer.Misc.convert(this.draw_current.speed, 	Gpx.Viewer.Misc.SpeedFormat.SPEED);
					text += "\n"+_("Elevation")+":\t"+ Gpx.Viewer.Misc.convert(this.draw_current.elevation, Gpx.Viewer.Misc.SpeedFormat.ELEVATION);
					text += "\n"+_("Distance")+":\t"+Gpx.Viewer.Misc.convert(this.draw_current.distance,  Gpx.Viewer.Misc.SpeedFormat.DISTANCE);
                    if(f.tpe.heartrate > 0) {
                        text += "\n"+_("Heart-rate")+": "+"%d".printf(this.draw_current.tpe.heartrate)+_("(bpm)");
                    }
                    text += "\n"+_("Cadence")+":\t"+"%u rpm".printf(this.draw_current.cadence);
                    text += "\n"+_("Time")+":\t"+"%s".printf(draw_current.get_datetime().to_local().format("%c"));

					fd.set_absolute_size(12*1024);
					layout.set_font_description(fd);
					layout.set_text(text,-1);
					layout.get_pixel_size(out w, out h);


					x_pos = (hl-(w+8)/2.0);
					if(x_pos < -LEFT_OFFSET) x_pos = 0.0;
					else if(hl+(w+8)/2.0 >= graph_width) x_pos = (double)graph_width - (double)(w+8.0);

					ctx.rectangle(x_pos, double.max(-h-2,-18), w+8, h+4);
					ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
					ctx.stroke_preserve();
					ctx.set_source_rgba(0.7, 0.7, 0.7, 0.9);
					ctx.fill();

					ctx.move_to(x_pos+4,double.max(-h,-16));


					Pango.cairo_layout_path(ctx, layout);

//					ctx.set_source_rgba(1.0, 1.0, 1.0, 1.0);
//					ctx.stroke_preserve();
					ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
					ctx.fill();
				}
			}
			return false;
		}

		/**
		 * Draw the graph to the required surface.
		 */
		private void draw_grid(Cairo.Context ctx, Pango.Layout layout,
				double graph_width, double graph_height,
				double min_value, double max_value, double elapsed_time)
		{
			double j =0.0;
			double step_size = (graph_height)/8.0;
			double range = max_value - min_value;

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw grid lines");
			/* Draw speed and ticks */
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);

			fd.set_absolute_size(12*1024);
			layout.set_font_description(fd);
            layout.set_text("0.0",-1);
            int wt,ht;
            layout.get_pixel_size(out wt, out ht);
			step_size = graph_height/(Math.ceil(graph_height/(ht+10)/5)*5);
			/* Draw horizontal lines + labels */
			for(j=0;j<=(graph_height+1);j+=step_size){
				double speed = min_value + range * ((graph_height-j)/graph_height);
				var text = "%.1f".printf(speed);
				int w,h;
                ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
                layout.set_text(text,-1);
				layout.get_pixel_size(out w, out h);
				ctx.move_to(-w-5, j-h/2.0);
				Pango.cairo_layout_path(ctx, layout);
				ctx.fill();

				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Set speed tick: %s",
						text);

				ctx.move_to(-4, j);
				ctx.line_to(0, j);
				ctx.stroke();
				// do not draw top/bottom. Small offset for float/double inpr.
				if(j <= 0.00001 || j >= (graph_height-0.00001)) continue;
                ctx.set_source_rgba(0.4, 0.4, 0.4, 0.6);
                ctx.set_line_width(1);
                ctx.move_to(0.0,j);
                ctx.line_to(graph_width,j);
				ctx.stroke();
			}

			/* Draw axis */
			ctx.set_line_width(2.5);
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1);
			ctx.move_to(0.0, 0.0);
			ctx.line_to(0.0,graph_height);
			ctx.stroke();

			ctx.line_to(0.0, graph_height+(graph_height/range)*(min_value));
			ctx.line_to(graph_width, graph_height+(graph_height/range)*(min_value));
			ctx.stroke();

			/* Draw time units. */

			fd.set_absolute_size(12*1024);
			layout.set_font_description(fd);
			uint interval = (uint)elapsed_time/((uint)(graph_width/(5*12.0)));
			int current = 0;
			for(uint i=0; i < elapsed_time && interval > 0; i+= interval)
			{
				if(graph_width*(1-(i/elapsed_time)) > 2.5*12 ){
					int w,h;
					var text = "%02i:%02i".printf((int)i/3600, ((int)i%3600)/60);
					layout.set_text(text,-1);
					layout.get_pixel_size(out w, out h);
					ctx.move_to(graph_width*(double)(i/elapsed_time)-w/2.0, graph_height+10);
					Pango.cairo_layout_path(ctx, layout);


					ctx.set_line_width(1.0);
					ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
					ctx.fill();

					log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Set time tick: %s",
							text);

					ctx.set_source_rgba(0.4, 0.4, 0.4, 0.6);
					ctx.set_line_width(1);
					ctx.move_to(graph_width*(double)(i/elapsed_time), graph_height);
					ctx.line_to(graph_width*(double)(i/elapsed_time), graph_height*0);
					ctx.stroke();

					ctx.set_line_width(1.5);
					ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
					ctx.move_to(graph_width*(double)(i/elapsed_time), graph_height+(graph_height/range)*(min_value));
					ctx.line_to(graph_width*(double)(i/elapsed_time), graph_height+(graph_height/range)*(min_value)+5);
					ctx.stroke();

				}
				current++;
			}


		}
		private double calculate_graph_point_value(List<Point?> ii)
		{
			double value =0;

			if(this._mode == GraphMode.SPEED) {
				value = ii.data.speed;
			}else if(this._mode == GraphMode.ELEVATION){
				value = ii.data.elevation;
			}else if(this._mode == GraphMode.TOTAL_DISTANCE){
				value = ii.data.distance;
			}else if(this._mode == GraphMode.DISTANCE){
				value = Gpx.Track.calculate_distance(ii.data, ii.first().data);
			}else if(this._mode == GraphMode.ACCELERATION_H && ii.prev != null){
                if(ii.data.get_time() == ii.prev.data.get_time()) return 0;
				value = (ii.data.speed- ii.prev.data.speed)/(3.6*(ii.data.get_time()-ii.prev.data.get_time()));
			}else if(this._mode == GraphMode.SPEED_V && ii.prev != null){
                if(ii.data.get_time() == ii.prev.data.get_time()) return 0;
				value = (ii.data.elevation- ii.prev.data.elevation)/(3.6*(ii.data.get_time()-ii.prev.data.get_time()));
			}else if (this._mode == GraphMode.HEARTRATE)  {
                 value = ii.data.tpe.heartrate;
            }else if (this._mode == GraphMode.CADENCE) {
                 value = ii.data.cadence;
            }
            return value;
		}
		private double calculate_graph_point_smooth_value(List<Point?> iter)
		{
			if(this._smooth_factor == 1)
			{
				return calculate_graph_point_value(iter);
			}
			else
			{
				double speed = 0;
				weak List<Point?> ii = iter;
				int sf = (ii.data.stopped && this._mode == GraphMode.SPEED)?1:this._smooth_factor;
				int i,items = 0;
				for(i=0;i< sf && ii.prev != null; i++)
				{
					speed += calculate_graph_point_value(ii);
					items++;
					ii = ii.prev;
				}
				ii = iter.next;
				for(i=1;i< sf && ii != null &&  ii.next != null; i++)
				{
					speed += calculate_graph_point_value(ii);
					items++;
					ii = ii.next;
				}
				speed = speed/items;
				return speed;
			}
		}
		private void update_surface(Gtk.Widget win)
		{
			var ctx = Gdk.cairo_create(win.get_window());
			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Updating surface");

			/* Get allocation */
			Gtk.Allocation alloc;

			win.get_allocation(out alloc);
			/* Create new surface */
			this.surf = new Cairo.Surface.similar(ctx.get_target(),
					Cairo.Content.COLOR_ALPHA,
					alloc.width, alloc.height);
			ctx = new Cairo.Context(this.surf);

			/* Paint background white */
			ctx.set_source_rgba(1,1,1,1);
			ctx.paint();

			if(this.track == null || this.track.points == null) {
				return;
			}
			double max_value = 0;
			double min_value = 0;
			double range = 0;
			if(this._mode == GraphMode.SPEED || this._mode == GraphMode.DISTANCE || this._mode == GraphMode.TOTAL_DISTANCE)
			{
				weak List<Point?> iter = this.track.points.first();
				while(iter.next != null)
				{
					weak List<Point?> ii = iter.next;
					double speed = calculate_graph_point_smooth_value(ii)-min_value;
					max_value = (speed > max_value )?speed:max_value;
					iter = iter.next;
				}
			}else if (this._mode == GraphMode.ELEVATION){
				max_value = track.max_elevation;
				min_value = track.min_elevation;
			}else if (this._mode == GraphMode.SPEED_V || this._mode == GraphMode.ACCELERATION_H) {
				weak List<Point?> iter = this.track.points.first();
				while(iter.next != null)
				{
					weak List<Point?> ii = iter.next;
					double speed = calculate_graph_point_smooth_value(ii)-min_value;
					max_value = (speed > max_value )?speed:max_value;
					min_value = (speed < min_value)?speed:min_value;
					iter = iter.next;
				}
			}else if (this.mode == GraphMode.HEARTRATE) {
                min_value = 0.0;
                max_value = 0.0;
				weak List<Point?> iter = this.track.points.first();
				while(iter.next != null)
				{
					weak List<Point?> ii = iter.next;
					double speed = calculate_graph_point_smooth_value(ii);
					max_value = (speed > max_value )?speed:max_value;
					iter = iter.next;
				}
            }else if (this.mode == GraphMode.CADENCE) {
                max_value = 0.0;
				weak List<Point?> iter = this.track.points.first();
				while(iter.next != null)
				{
					weak List<Point?> ii = iter.next;
					max_value = (ii.data.cadence > max_value )?ii.data.cadence:max_value;
					iter = iter.next;
				}

            }
			max_value = GLib.Math.ceil(max_value);

            if(max_value == min_value) return;

			range = max_value-min_value;

			double elapsed_time = track.get_total_time();

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw Axis");

			// Set the top left x,y coordinate as 0,0
			ctx.translate(LEFT_OFFSET,20);
			Point f = track.points.data;

			double graph_width = alloc.width-LEFT_OFFSET-RIGHT_OFFSET;
			double graph_height = alloc.height-20-BOTTOM_OFFSET;
			if(graph_height < 50 ) return;
			var layout = Pango.cairo_create_layout(ctx);
			/*****
			 * Draw Grid 
			 **/
			draw_grid(ctx,layout, graph_width, graph_height, min_value, max_value, elapsed_time);


			/* Draw the graph */
			ctx.set_source_rgba(0.1, 0.2, 0.3, 1);
			ctx.set_line_width(1);
			weak List<Point?> iter = track.points.first();

			// Move to start point of graph.
			if(min_value < 0 && max_value > 0) {
				ctx.move_to(0.0, graph_height*((max_value)/range));
			}else {
				ctx.move_to(0.0, graph_height);
			}


			while(iter.next != null)
			{
				double time_offset = (iter.data.get_time()-f.get_time());
				double speed = calculate_graph_point_smooth_value(iter)-min_value;
                if(this._mode == GraphMode.SPEED)
				{
					// if previous one is stopped, start at 0 
					if(iter.prev != null && iter.prev.data.stopped) {
						ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),
								graph_height);
					}
					// if this one is  stopped, draw line at 0. 
					if(iter.data.stopped)
					{
						ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),
								graph_height);
					}else{
						ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),
								graph_height*(double)(1.0-speed/(range)));
					}

					// if next point is stopped. goto 0. 
					if(iter.next != null && iter.next.data.stopped) {
						ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),
								graph_height*(double)1.0);
					}
				}else{
					ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),
							graph_height*(double)(1.0-speed/(range)));

				}
				iter = iter.next;
			}
			// back to 0 line.. 
			if(min_value < 0 && max_value > 0) {
				ctx.line_to(graph_width, graph_height*((max_value)/range));
			}else{
				ctx.line_to(graph_width, graph_height*1);
			}
			// Close the path.
			ctx.close_path();
			ctx.stroke_preserve();

			ctx.set_source_rgba(0.6, 0.8, 0.9, 0.8);
			ctx.fill();

			if (this.show_points) {
				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw data points");
				/* Draw points */
				ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
				iter = track.points.first();
				while(iter.next != null)
				{
					weak List<Point?> ii = iter.next;
					double time_offset = (ii.data.get_time()-f.get_time());
					double speed = calculate_graph_point_smooth_value(ii)-min_value;


					if(ii.data.stopped) {
						ctx.set_source_rgba(1.0, 0.0, 0.0, 1.0);
					}else{
						ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
					}
                    ctx.rectangle(graph_width*(double)(time_offset/(double)elapsed_time)-1,
							graph_height*(double)(1.0-speed/(range))-1,2,2);
					ctx.stroke();

					iter = iter.next;
				}
			}

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw graph");

			if(this._mode == GraphMode.SPEED)
			{
				/* Draw average speed */
				var avg = track.get_track_average();
				ctx.move_to(0.0, graph_height*(1-avg/max_value));
				ctx.line_to(graph_width, graph_height*(1-avg/max_value));

				ctx.set_line_width(2.5);
				ctx.set_source_rgba(1.00,0.33, 0.00, 1.0);
				ctx.stroke();
				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw average speed line @ %.02f km/h", avg);

				/* Draw moving speed */
				time_t moving_time;
				avg = track.calculate_moving_average(this.track.points.first().data, this.track.points.last().data,out moving_time);
				ctx.set_source_rgba(0.7, 0.0, 0.0, 1.0);
				ctx.move_to(0.0, graph_height*(1-avg/max_value));
				ctx.line_to(graph_width, graph_height*(1-avg/max_value));
				ctx.stroke();

				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw moving average speed line @ %.02f km/h", avg);
			}

			/* Draw the title */
			int w,h;
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
			fd.set_absolute_size(12*1024);
			layout.set_font_description(fd);
			string mtext = "";
			if(_do_miles) {
				mtext = _(Graph.GraphModeMiles[this._mode]);
			}else{
				mtext = _(Graph.GraphModeName[this._mode]);
			}
			if(this._smooth_factor != 1)
			{
				var markup = _("%s <i>(smooth window: %i)</i>").printf(mtext,this._smooth_factor);
				layout.set_markup(markup,-1);
				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Set graph title: %s",
						markup);
			}
			else
			{
				layout.set_text(mtext,-1);
				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Set graph title: %s",
						mtext);
			}
			layout.get_pixel_size(out w, out h);
			ctx.move_to(graph_width/2-w/2, -20);
			Pango.cairo_layout_path(ctx, layout);
			ctx.fill();
		}
        ~Graph()
        {
            GLib.debug("Destroying graph");
        }
	}
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

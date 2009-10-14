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

using Gtk;
using Gpx;
using GLib;
using Config;

static const string LOG_DOMAIN="GPX_PARSER";
static const string unique_graph = Config.VERSION;
namespace Gpx
{
	public class Graph: Gtk.EventBox
	{
		private int _smooth_factor =4;
		public Gpx.Track track = null;
		private Pango.FontDescription fd = null; 
		private Cairo.Surface surf = null;
		private int LEFT_OFFSET=60;
		private int BOTTOM_OFFSET=30;
		private time_t highlight = 0;

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
			default = 4;
		}

		public Graph ()
		{
			this.fd = new Pango.FontDescription();
			fd.set_family("sans mono");
			this.app_paintable = true;
			this.visible_window = true;
			this.size_allocate.connect(size_allocate_cb);
			this.button_press_event.connect(button_press_event_cb);
			this.motion_notify_event.connect(motion_notify_event_cb);
			this.button_release_event.connect(button_release_event_cb);
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

		signal void point_clicked(Gpx.Point point);

		signal void selection_changed(Gpx.Track? track, Gpx.Point? start, Gpx.Point? stop);
		/**
		 * Private functions
		 */
		private Gpx.Point? get_point_from_position(double x, double y)
		{

			if(this.track == null) return null;
			if(x > LEFT_OFFSET && x < (this.allocation.width-10))
			{
				double elapsed_time = track.get_total_time();
				time_t time = (time_t)((x-LEFT_OFFSET)/(this.allocation.width-10-LEFT_OFFSET)*elapsed_time);
				weak List<Point?> iter = this.track.points.first();
				/* calculated time is offset from start time,  get real time */
				time += iter.data.get_time();
				while(iter.next != null)
				{
					if(time < iter.next.data.get_time() && (time == iter.data.get_time() || 
								time > iter.data.get_time()))
					{
						return iter.data;
					}
					iter = iter.next;
				}
			}
			return null;
		}
		private bool button_press_event_cb(Gdk.EventButton event)
		{
			if(this.track == null) return false;
			Gpx.Point *point = this.get_point_from_position(event.x, event.y); 
			if(point != null) {
				if(event.button == 1){
					this.start = point; 
				}else{ 
					this.start = null;
					point_clicked(point);
				}
			}
			return false;
		}

		private bool motion_notify_event_cb(Gdk.EventMotion event)
		{
			if(this.track == null) return false;
			if(this.start == null) return false;

			Gpx.Point *point = this.get_point_from_position(event.x, event.y); 
			if(point != null)
			{
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
						return false;
					}
				}
				selection_changed(this.track, this.track.points.first().data, this.track.points.last().data);
			}
			return false;
		}
		private bool button_release_event_cb(Gdk.EventButton event)
		{
			if(this.track == null) return false;
			Gpx.Point *point = this.get_point_from_position(event.x, event.y); 
			if(point != null)
			{
				if(event.button == 1)
					this.stop = point; 
				else this.stop = null;
				this.queue_draw();
				if(event.button == 1)
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
							return false;
						}
					}
					selection_changed(this.track, this.track.points.first().data, this.track.points.last().data);
				}
			}
			return false;
		}
		private void size_allocate_cb(Gdk.Rectangle alloc)
		{
			/* Invalidate the previous plot, so it is redrawn */
			this.surf = null;
		}

		private Gpx.Point start = null;
		private Gpx.Point stop = null;
		override bool expose_event(Gdk.EventExpose event)
		{
			var ctx = Gdk.cairo_create(this.window);
			/* If no valid surface, render it */
			if(surf == null)
				update_surface(this);

			/* Draw the actual surface on the widget */
			ctx.set_source_surface(this.surf, 0, 0);
			Gdk.cairo_region(ctx, event.region);
			ctx.clip();
			ctx.paint();

			if(highlight > 0 )
			{
				Gpx.Point f = this.track.points.first().data;
				double elapsed_time = track.get_total_time();
				double graph_width = this.allocation.width-LEFT_OFFSET-10;
				double graph_height = this.allocation.height-20-BOTTOM_OFFSET;

				double hl = (highlight-f.get_time())/elapsed_time*graph_width; 

				ctx.translate(LEFT_OFFSET,20);
				ctx.set_source_rgba(0.8, 0.2, 0.3, 0.8);
				ctx.move_to(hl, 0);
				ctx.line_to(hl,graph_height);

				ctx.stroke_preserve();
				ctx.fill();
			}
			/* Draw selection, if available */
			if(start != null && stop != null)
			{
				if(start.get_time() != stop.get_time())
				{
					Gpx.Point f = this.track.points.first().data;
					double elapsed_time = track.get_total_time();
					double graph_width = this.allocation.width-LEFT_OFFSET-10;
					double graph_height = this.allocation.height-20-BOTTOM_OFFSET;

					ctx.translate(LEFT_OFFSET,20);
					ctx.set_source_rgba(0.3, 0.2, 0.3, 0.8);
					ctx.rectangle((start.get_time()-f.get_time())/elapsed_time*graph_width, 0, 
							(stop.get_time()-start.get_time())/elapsed_time*graph_width, graph_height);
					ctx.stroke_preserve();
					ctx.fill();
				}

			}
			return false;
		}
		private void update_surface(Gtk.Widget win)
		{
			var ctx = Gdk.cairo_create(win.window);
			this.surf = new Cairo.Surface.similar(ctx.get_target(),
					Cairo.Content.COLOR_ALPHA,
					win.allocation.width, win.allocation.height); 
			ctx = new Cairo.Context(this.surf);

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Updating surface");
			/* Paint background white */
			ctx.set_source_rgba(1,1,1,1);
			ctx.paint();
			if(this.track == null) return;
			double max_speed = 0;
			if(this.smooth_factor != 1)
			{
				weak List<Point?> iter = this.track.points.first();
				while(iter.next != null)
				{
					weak List<Point?> ii = iter.next;
					double speed = 0;
					int i=0;
					int sf = this.smooth_factor;
					for(i=0;i<sf && ii.prev != null; i++)
					{
						speed += track.calculate_point_to_point_speed(ii.prev.data, ii.data);
						ii = ii.prev;
					}
					speed = speed/i;
					max_speed = (speed > max_speed)?speed:max_speed;
					iter = iter.next;
				}
			}
			else 
				max_speed = track.max_speed;
			double elapsed_time = track.get_total_time();


			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Max speed: %f, elapsed_time: %f",
					max_speed,
					elapsed_time);

			ctx.translate(LEFT_OFFSET,20);
			Point f = track.points.data;

			/* Draw Grid */
			double graph_width = win.allocation.width-LEFT_OFFSET-10;
			double graph_height = win.allocation.height-20-BOTTOM_OFFSET;

			var layout = Pango.cairo_create_layout(ctx);
			double j =0.0;
			double step_size = (graph_height)/8.0;
			ctx.set_source_rgba(0.2, 0.2, 0.2, 0.6);
			ctx.set_line_width(1);
			for(j=graph_height;j>0.0;j-=step_size){
				ctx.move_to(0.0,j);
				ctx.line_to(graph_width,j);
				ctx.stroke();
			}
			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw grid lines");
			/* Draw speed and ticks */
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
			double size = LEFT_OFFSET/("%.1f".printf(max_speed).length);
			if(size > step_size) size = step_size;
			fd.set_absolute_size(size*1024);
			layout.set_font_description(fd);
			for(j=0;j<graph_height;j+=step_size){
				double speed = max_speed*((graph_height-j)/graph_height);
				var text = "%.1f".printf(speed);
				int w,h;
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
				/* */
			}

			/* Draw axis */
			ctx.move_to(0.0,0.0);
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1);
			ctx.set_line_width(1.5);
			ctx.line_to(0.0, graph_height);
			ctx.line_to(graph_width, graph_height);
			ctx.stroke();

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw Axis"); 

			/* Draw the graph */
			ctx.set_source_rgba(0.1, 0.2, 0.3, 1);
			ctx.set_line_width(1);
			weak List<Point?> iter = track.points.first();
			ctx.move_to(0.0, graph_height);


			double pref_speed = 2f;
			while(iter.next != null)
			{
				double time_offset = (iter.data.get_time()-f.get_time());
				double speed = 0;
				weak List<Point?> ii = iter.next;
				int i=0;
				int sf = this.smooth_factor;
				for(i=0;i< sf && ii.prev != null; i++)
				{
					speed += track.calculate_point_to_point_speed(ii.prev.data, ii.data);
					ii = ii.prev;
				}
				speed = speed/i;
				if(pref_speed < 1) {
					ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),
							graph_height*(double)(1.0-0));

				}
				ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),
						graph_height*(double)(1.0-speed/max_speed));
				iter = iter.next;

				pref_speed = speed;
			}
			ctx.line_to(graph_width, graph_height);
			ctx.close_path();
			ctx.stroke_preserve();

			ctx.set_source_rgba(0.1, 0.2, 0.8, 0.5);
			ctx.fill();

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw data points"); 
			/* Draw points */
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
			iter = track.points.first();
			while(iter.next != null)
			{
				double time_offset = (iter.data.get_time()-f.get_time());
				double speed = 0;
				weak List<Point?> ii = iter.next;
				int i=0;
				int sf = this.smooth_factor;
				for(i=0;i< sf && ii.prev != null; i++)
				{
					speed += track.calculate_point_to_point_speed(ii.prev.data, ii.data);
					ii = ii.prev;
				}
				speed = speed/i;
				ctx.rectangle(graph_width*(double)(time_offset/(double)elapsed_time)-1,
						graph_height*(double)(1.0-speed/max_speed)-1,2,2);
				ctx.stroke();

				iter = iter.next;
			}

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw graph"); 

			iter = track.points.first();

			ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
			fd.set_absolute_size(12*1024);
			layout.set_font_description(fd);
			uint interval = (uint)elapsed_time/((uint)(graph_width/(5*12.0)));
			int current = 0;
			uint i;
			for(i=0; i < elapsed_time && interval > 0; i+= interval)
			{
				if(graph_width*(1-(i/elapsed_time)) > 2.5*12 ){
					int w,h;
					var text = "%02i:%02i".printf((int)i/3600, ((int)i%3600)/60);
					layout.set_text(text,-1);
					layout.get_pixel_size(out w, out h);
					ctx.move_to(graph_width*(double)(i/elapsed_time)-w/2.0, graph_height+10);
					Pango.cairo_layout_path(ctx, layout);
					ctx.fill();

					log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Set time tick: %s",
							text);

					ctx.move_to(graph_width*(double)(i/elapsed_time), graph_height);
					ctx.line_to(graph_width*(double)(i/elapsed_time), graph_height+5);
					ctx.stroke();
				}
				current++;
			}

			/* Draw average speed */
			var avg = track.get_track_average();
			ctx.set_source_rgba(0.0, 0.7, 0.0, 0.7);
			ctx.move_to(0.0, graph_height*(1-avg/max_speed));
			ctx.line_to(graph_width, graph_height*(1-avg/max_speed));
			ctx.stroke();
			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw average speed line @ %.02f km/h", avg);

			/* Draw moving speed */
			time_t moving_time;
			avg = track.calculate_moving_average(this.track.points.first().data, this.track.points.last().data,out moving_time);
			ctx.set_source_rgba(0.7, 0.0, 0.0, 0.7);
			ctx.move_to(0.0, graph_height*(1-avg/max_speed));
			ctx.line_to(graph_width, graph_height*(1-avg/max_speed));
			ctx.stroke();

			log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Draw moving average speed line @ %.02f km/h", avg);

			/* Draw the title */
			int w,h;
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
			fd.set_absolute_size(12*1024);
			layout.set_font_description(fd);
			if(this.smooth_factor != 1)
			{
				var markup = _("Speed (km/h) vs Time (HH:MM) <i>(smooth window: %i)</i>").printf(this.smooth_factor);
				layout.set_markup(markup,-1);
				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Set graph title: %s",
						markup);
			}
			else
			{
				var text = _("Speed (km/h) vs Time (HH:MM)");
				layout.set_text(text,-1);
				log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Set graph title: %s",
						text);
			}
			layout.get_pixel_size(out w, out h);
			ctx.move_to(graph_width/2-w/2, -20);
			Pango.cairo_layout_path(ctx, layout);
			ctx.fill();
		}
	}
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

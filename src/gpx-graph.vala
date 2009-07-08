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

static const string LOG_DOMAIN="GPX_PARSER";
namespace Gpx
{
	public class Graph: Gtk.EventBox
	{
		private int _smooth_factor =4;
		private Gpx.Track track = null;
		private Pango.FontDescription fd = null; 
		private Cairo.Surface surf = null;
		private int LEFT_OFFSET=60;
		private int BOTTOM_OFFSET=30;


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
			this.fd = new Pango.FontDescription();//from_string("sans mono"); 
			fd.set_family("sans mono");
			this.app_paintable = true;
			this.visible_window = true;
			this.size_allocate.connect(size_allocate_cb);
			this.button_press_event.connect(button_press_event_cb);
		}

		public void set_track(Gpx.Track? track)
		{
			this.track =track;
			/* Invalidate the previous plot, so it is redrawn */
			this.surf = null;
			/* Force a redraw */
			this.queue_draw();
		}

		signal void point_clicked(double lat_dec, double lon_dec);
		/**
		 * Private functions
		 */
		private bool button_press_event_cb(Gdk.EventButton event)
		{
			if(this.track == null) return false;
			if(event.x > LEFT_OFFSET && event.x < (this.allocation.width-10))
			{
				double elapsed_time = track.get_total_time();
				time_t time = (time_t)((event.x-LEFT_OFFSET)/(this.allocation.width-10-LEFT_OFFSET)*elapsed_time);


				weak List<Point?> iter = this.track.points.first();
				time += iter.data.get_time();
				while(iter.next != null)
				{
					if(time < iter.next.data.get_time() && (time == iter.data.get_time() || time > iter.data.get_time()))
					{
						point_clicked(iter.data.lat_dec, iter.data.lon_dec);

						return false;
					}

					iter = iter.next;
				}
			}
			return false;
		}
		private void size_allocate_cb(Gdk.Rectangle alloc)
		{
			/* Invalidate the previous plot, so it is redrawn */
			this.surf = null;
		}
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
			}
			/* Draw axis */
			ctx.move_to(0.0,0.0);
			ctx.set_source_rgba(0.0, 0.0, 0.0, 1);
			ctx.set_line_width(1.5);
			ctx.line_to(0.0, graph_height);
			ctx.line_to(graph_width, graph_height);
			ctx.stroke();


			/* Draw the graph */
			ctx.set_source_rgba(0.1, 0.2, 0.3, 1);
			ctx.set_line_width(1);
			weak List<Point?> iter = track.points.first();
			ctx.move_to(0.0, graph_height);
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
				ctx.line_to(graph_width*(double)(time_offset/(double)elapsed_time),graph_height*(double)(1.0-speed/max_speed));
				iter = iter.next;
			}
			ctx.line_to(graph_width, graph_height);
			ctx.close_path();
			ctx.stroke_preserve();

			ctx.set_source_rgba(0.1, 0.2, 0.8, 0.5);
			ctx.fill();


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


			/* Draw moving speed */
			time_t moving_time;
			avg = track.calculate_moving_average(out moving_time);
			ctx.set_source_rgba(0.7, 0.0, 0.0, 0.7);
			ctx.move_to(0.0, graph_height*(1-avg/max_speed));
			ctx.line_to(graph_width, graph_height*(1-avg/max_speed));
			ctx.stroke();

			{
				int w,h;
				ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
				fd.set_absolute_size(12*1024);
				layout.set_font_description(fd);
				if(this.smooth_factor != 1)
					layout.set_markup("Speed (km/h) vs Time (HH:MM) <i>(smooth window: %i)</i>".printf(this.smooth_factor),-1);
				else
					layout.set_text("Speed (km/h) vs Time (HH:MM)",-1);
				layout.get_pixel_size(out w, out h);
				ctx.move_to(graph_width/2-w/2, -20);
				Pango.cairo_layout_path(ctx, layout);
				ctx.fill();
			}
		}
	}
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

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
using GLib;
using Xml;


namespace Gpx {
	/**
	 * Represents a point in the track or a waypoint. 
	 */
	public class Point {
		/* Waypoint name */
		public string name = null; 
		/* Position, in radians and degrees, 1000 means not set */
		public double lat = 1000;
		public double lon = 1000;
		public double lat_dec = 1000;
		public double lon_dec = 1000;
		/* The distance from start of track. (only if part of track */
		public double distance =0;
		/* Elevation */
		public double elevation;
		/* Time */
		public string time;
		/* The speed (only if part of track */
		public double speed = 0; 


		private time_t utime  = 0;

		/* Sets the poistion in degrees, automagically calculates radians */
		public void set_position(double lat, double lon)
		{
			this.lat_dec = lat;
			this.lon_dec = lon;
			this.lat = (2*GLib.Math.PI*lat)/360.0;
			this.lon = (2*GLib.Math.PI*lon)/360.0;
		}
		/* Get the unix time. (calculated on first request, then cached ) */
		public time_t get_time()
		{
			if(this.time == null) return 0;
			if(this.utime > 0)
				return this.utime;
			Time ta = Time();
			ta.strptime(this.time, "%FT%T%z");
			this.utime = ta.mktime();
			return utime;
		}
	}

	/**
	 * This class represents a Track in a gpx file.
	 * The tracks contains the points connecting everything together. 
	 * Info like total distance, average speed, moving speed/time etc are available.
	 */
	public class Track : GLib.Object {
		/* make a property */
		public string name = null;

		/* usefull info gathered during walking the list */
		public double total_distance = 0.0;
		public double max_speed = 0;
		public List<Point> points = null;

		private Point? last = null;
		/* To get bounding box for view */
		public Point top = null;
		public Point bottom = null;

		public void add_point (Point point)
		{
			if(last != null)
			{
				var distance = calculate_distance(last, point); 
				this.total_distance += distance;
				point.distance = this.total_distance;


				if(last.time != null && point.time != null) {
					point.speed = calculate_point_to_point_speed(last, point);
					if((calculate_point_to_point_speed(this.points.first().data, point)*4) < point.speed) 
						point.speed = last.speed;
					var avg = point.speed; 
					if(avg > this.max_speed) this.max_speed = avg;
				}

				/* Update the 2 bounding box points */
				if(top == null || top.lat_dec ==  1000 || top.lat_dec > point.lat_dec) {
					if(top == null) top = new Point();
					top.lat_dec = point.lat_dec;
				}
				if(top == null || top.lon_dec == 1000 || top.lon_dec > point.lon_dec) {
					if(top == null) top = new Point();
					top.lon_dec = point.lon_dec;
				}
				if(bottom == null || bottom.lat_dec == 1000 || bottom.lat_dec < point.lat_dec) {
					if(bottom == null) bottom = new Point();
					bottom.lat_dec = point.lat_dec;
				}
				if(bottom == null || bottom.lon_dec == 1000 || bottom.lon_dec < point.lon_dec) {
					if(bottom == null) bottom = new Point();
					bottom.lon_dec = point.lon_dec;
				}

			}
			points.append(point);
			last = point; 
		}

		/* Private api */
		/**
		 * Calculate the speed of the full track
		 */

		public double get_track_average()
		{
			weak List<Point ?> first = this.points.first();
			weak List<Point ?> last = this.points.last();
			if(first != null && last != null) {
				return this.calculate_point_to_point_speed(first.data, last.data);
			}
			return 0;
		}

		/**
		 * Calculate the average speed between Point a and Point b on the track
		 */
		public double calculate_point_to_point_speed(Point a, Point b)
		{
			var dist = (b.distance - a.distance);
			if(a.time == null || b.time == null) return 0;
			var ta = a.get_time();
			var tb = b.get_time();
			if((tb -ta) == 0) return 0;
			return dist/((tb-ta)/(60.0*60.0));
		}
		public time_t get_total_time()
		{
			Point a, b;
			weak List<Point?> na = this.points.first();
			weak List<Point?>  nb = this.points.last();
			if(na == null || nb == null) return 0;
			a = na.data;
			b = nb.data;
			var time = b.get_time()-a.get_time();
			return time;
		}
		/**
		 * Try not to calculate time that we "stopped"  in average
		 */
		public double calculate_moving_average(out time_t moving_time)
		{
			double time = 1;
			double distance = 0;
			moving_time = 0;
			weak List<Point?> iter = this.points.first();
			if(iter == null) return 0;
			Point a  = iter.data;
			while((iter = iter.next)!= null)
			{
				Point b  = iter.data;
				if( (b.distance-a.distance) > 0.007){
					a = iter.prev.data;
					time += (b.get_time()-a.get_time());
					distance += b.distance-a.distance; 

					a = b; 
				}
			}
			moving_time = (time_t)time;	
			return distance/(time/(60.0*60.0));
		}

		/* Calculate distance between point a and point b using great circular distance method 
		 * Elevation is not taken into account.
		 */
		private double calculate_distance(Point a, Point b)
		{
			double retv =0;
			if(a.lat == b.lat && a.lon == b.lon) return 0;
			retv = 6378.7 * Math.acos(
					Math.sin(a.lat) *  Math.sin(b.lat) + 
					Math.cos(a.lat) * Math.cos(b.lat) * Math.cos(b.lon - a.lon)
					);
			if(GLib.Math.isnan(retv) == 1)
			{
				return 0;
			}
			return retv;
		}

	}

	
	/**
	 * This is the top level class representing the gpx file it self.
	 * This contains a list of tracks and waypoings.
	 */
	public class File : GLib.Object 
	{
		/* The filename */
		public string filename = null;
		/* A gpx file can contain multiple tracks, this supports it */
		public GLib.List<Gpx.Track> tracks = null;
		/* A gpx file can also contains a list of waypoints */
		public GLib.List<Gpx.Point> waypoints = null; 

		private void parse_track(Xml.Node *node)
		{
			/* Create new track here */
			Gpx.Track track = new Gpx.Track();

			var trkseg = node->children;
			/* iterretate over track segments */
			while(trkseg != null){
				if(trkseg->name == "trkseg") {
					var point = trkseg->children;
					while(point != null)
					{
						if(point->name == "trkpt")
						{
							var lat = point->get_prop("lat");
							var lon = point->get_prop("lon");
							if(lat != null && lon != null)
							{
								Point p = new Point();
								double flat = lat.to_double();
								double flon = lon.to_double();
								p.set_position(flat, flon);
								var info = point->children;
								while(info != null) {
									/* height */
									if(info->name == "ele") {
										var content = info->get_content();
										if(content != null)
											p.elevation = content.to_double();
									}else if (info->name == "time") {
										p.time = info->get_content();
									}
									info = info->next;
								}

								track.add_point(p);
							}else{
								GLib.message("Failed to get point: %s\n", point->name);
							}
						}
						point = point->next;
					}                                
				}
				if(trkseg->name == "name"){
					if(track.name == null){
						track.name = trkseg->get_content();
					}
					else{
						GLib.warning("Track name allready set: %s\n", track.name);
					}
				}

				trkseg = trkseg->next; 
			}
			this.tracks.append(track);
		}

		private void parse_waypoint(Xml.Node *node)
		{
			var lat = node->get_prop("lat");
			var lon = node->get_prop("lon");
			if(lat != null && lon != null)
			{
				Point p = new Point();
				double flat = lat.to_double();
				double flon = lon.to_double();
				p.set_position(flat, flon);
				var info = node->children;
				while(info != null) {
					if(info->name == "name"){
						if(p.name == null){
							p.name = info->get_content();
						}
						else{
							GLib.warning("Point name allready set: %s\n", p.name);
						}
					}
					info = info->next;
				}
				this.waypoints.append(p);
			}

		}
		/**
		 * Parse a file
		 */
		public File (string filename)
		{
			this.filename = filename;
			Xml.TextReader reader = new Xml.TextReader.for_file(this.filename, null, 0);
			if(reader != null)
			{
				/* Start parsing the xml file */
				var doc = reader.read(); 
				while(doc == 1)
				{
					var name = reader.const_name();
					if(name == "gpx")
					{
						int doc2 = reader.read();
						while(doc2 == 1)
						{
							var name2 = reader.const_name();
							/* Get the track element */
							if(name2 == "trk") {
								var node = reader.expand();
								this.parse_track(node);
							} else if (name2 == "wpt") {
								var node = reader.expand();
								this.parse_waypoint(node);
							}
							doc2 = reader.next();
						}
					}
					else
						doc = reader.read();
				}
			}else{
				/* Todo add error trower here http://www.vala-project.org/doc/vala-draft/errors.html#exceptionsexamples*/
				GLib.message("Failed to open file");
			}
		}
	}
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

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
    public struct HeartRateMonitorTrack
    {
        public uint32 calories;
    }

    /**
     * This class represents a Track in a gpx file.
     * The tracks contains the points connecting everything together.
     * Info like total distance, average speed, moving speed/time etc are available.
     */
    public class Track : GLib.Object
    {
        public HeartRateMonitorTrack hrmt = HeartRateMonitorTrack() { calories = 0 }; 
        /* make a property */
        public string name {get; set; default = null;}
        public string track_type {get; set; default = null;}
        public int? number {get; set; default = null;}
		public bool is_route {get; set; default = false;}
		/**  Number of points that the #filter_points()  function removed */
		public int filtered_points = 0; 


        /* usefull info gathered during walking the list */
        public double total_distance = 0.0;
        public double max_speed      = 0.0;
        public double max_elevation  = 0.0;
        public double min_elevation  = 0.0;
	
		/* All the Gpx.Points */
        public List<Point> points = null;
		/* Keeping a last pointer allows us to add points to the list faster. */
        /* No it doesn't :D */
        private Point? last = null;

        /* To get bounding box for view */
        /*  These are 2 fake points */
        public Point top = null;
        public Point bottom = null;
		

		public unowned Point? get_last()
		{
			return last;
		}

		/** This function will try to remove useless points */
		public void filter_points ()
		{
			unowned List<Point>? a = null;
			unowned List<Point>? b = null;
			unowned List<Point>? c = null;

			double davg = this.get_track_average();
			/* We take three points.  A-B-C.  If B lays on the same lineair line as remove it. */
			for(unowned List<Point> ?iter = this.points.first() ; iter != null;iter = iter.next)
			{
				if(b != null) c = b;
				if(a != null) b = a;
				a = iter;
				if(a != null && b != null && c != null) 
				{
					double elapsed_ca = (double)(a.data.get_time() - c.data.get_time());
					double elapsed_cb = (double)(a.data.get_time() - b.data.get_time());

					double lat_rico_ca = (a.data.lat_dec-c.data.lat_dec)/(double)elapsed_ca;
					double lon_rico_ca = (a.data.lon_dec-c.data.lon_dec)/(double)elapsed_ca;
					double lat_rico_cb = (b.data.lat_dec-c.data.lat_dec)/(double)elapsed_cb;
					double lon_rico_cb = (b.data.lon_dec-c.data.lon_dec)/(double)elapsed_cb;

					double elv_rico_ca = (a.data.elevation - c.data.elevation)/(double)elapsed_ca;
					double elv_rico_cb = (a.data.elevation - b.data.elevation)/(double)elapsed_cb;

					double l = Math.fabs(1.0-lat_rico_ca/lat_rico_cb ) ;
					double m = Math.fabs(1.0-lon_rico_ca/lon_rico_cb ) ;
					double e = Math.fabs(1.0-elv_rico_ca/elv_rico_cb ) ;

                    var abs_diff = Math.fabs(b.data.speed-a.data.speed)+Math.fabs(c.data.speed-b.data.speed);
                    var diff = Math.fabs((b.data.speed-a.data.speed)+(c.data.speed-b.data.speed));
                    if(  diff < 0.2*abs_diff &&  abs_diff > 3*davg) {
                        stdout.printf("----- %f %f filter points\n", diff, abs_diff);
                        points.remove_link(b); 
                    }
                    else
                        if( l <= 0.2) 
					{
						if(m <= 0.2 /*&& e <= 0.8*/) 
						{
							/*  TODO: this leaks memory */
							points.remove_link(b);
							this.filtered_points++;
							/* Make sure C is c again in the next run.  a becomes the new b, new point a */
							b = c;
						}
					}
				}
			}

			this.recalculate();
			double avg = this.get_track_average()/20;
			avg = (avg >  2)?2:avg;
			for(unowned List<Point> ?iter = this.points.first() ; iter != null;iter = iter.next)
			{
				weak Gpx.Point? p = iter.data;
				if(p.distance < 0.01 || p.speed  < avg || !p.has_position()) {
					p.stopped = true;
				}
			}
			GLib.debug("Removed %i points",this.filtered_points);
		}
		/**
		 * This will recalculates all speeds and distances. Call this when the list was modified.
		 */
		public void recalculate()
		{
			unowned List<Point> ?last = null;
			this.total_distance = 0;
			this.max_speed = 0;
			this.max_elevation = 0.0;
			this.min_elevation = 0.0;
            for(unowned List<Point> ?iter = this.points.first() ; iter != null;iter = iter.next)
			{
                unowned Gpx.Point point = iter.data;
                if(last != null) {
                    if(point.has_position()) {
                        total_distance += calculate_distance(last.data,point); 
                        point.distance = total_distance;
                        point.speed = calculate_point_to_point_speed(last.data,point); 
                        if(point.elevation > this.max_elevation) this.max_elevation = point.elevation;
                        if(point.elevation < this.min_elevation) this.min_elevation = point.elevation;
                        if(point.speed > this.max_speed) this.max_speed = point.speed;
                    }
                }else{
                    if(point.elevation > this.max_elevation) this.max_elevation = point.elevation;
                    if(point.elevation < this.min_elevation) this.min_elevation = point.elevation;
					point.distance = 0;
				}
				// radius in km
				double radius = 0.2;

				// add current point
				double elevation_value = point.elevation * radius;
				double weights = radius;

				// add previous points within the radius
				unowned List<Point> ?env_iter = iter;
				int i = 0;
				while ((env_iter = env_iter.prev) != null) {
					i++;
					double mydist = calculate_distance(env_iter.data, point);
					if (mydist < radius) {
						elevation_value += (env_iter.data.elevation * (radius - mydist));
						weights += (radius - mydist);
					} else break;
				}
				// add following points within the radius
				env_iter = iter;
				while ((env_iter = env_iter.prev) != null) {
					i++;
					double mydist = calculate_distance(env_iter.data, point);
					if (mydist < radius) {
						elevation_value += (env_iter.data.elevation * (radius - mydist));
						weights += (radius - mydist);
					} else break;
				}
				point.smooth_elevation = elevation_value / weights;
				//log(LOG_DOMAIN, LogLevelFlags.LEVEL_DEBUG, "Used %d points in radius of %f m", i, radius * 1000.0);
                if(point.has_position()) {
                    last = iter;
                }
            }
		}

        public void add_point (Point point)
        {
			/* Make sure this is 0 */
			point.speed = 0.0;

            if(last != null)
            {
                var distance = calculate_distance(last, point);
                this.total_distance += distance;
                point.distance = this.total_distance;
                /* Update the 2 bounding box points */
                if(top == null || top.lat_dec ==  1000 || top.lat_dec < point.lat_dec)
                {
                    if(top == null) top = new Point();
                    top.lat_dec = point.lat_dec;
                    top.lat = point.lat;
                }
                if(top == null || top.lon_dec == 1000 || top.lon_dec < point.lon_dec)
                {
                    if(top == null) top = new Point();
                    top.lon_dec = point.lon_dec;
					top.lon = point.lon;
                }
                if(bottom == null || bottom.lat_dec == 1000 || bottom.lat_dec > point.lat_dec)
                {
                    if(bottom == null) bottom = new Point();
                    bottom.lat_dec = point.lat_dec;
					bottom.lat = point.lat;
                }
                if(bottom == null || bottom.lon_dec == 1000 || bottom.lon_dec > point.lon_dec)
                {
                    if(bottom == null) bottom = new Point();
                    bottom.lon_dec = point.lon_dec;
                    bottom.lon = point.lon;
                }
                if(last.time != null && point.time != null && point.has_position())
                {
                    point.speed = calculate_point_to_point_speed(last, point);
                    if(point.speed > this.max_speed) this.max_speed = point.speed;
                    if(point.elevation > this.max_elevation) this.max_elevation = point.elevation;
                    if(point.elevation < this.min_elevation) this.min_elevation = point.elevation;

                }

            }
            else
            {
                this.max_elevation = point.elevation;
                this.min_elevation = point.elevation;
            }
            points.append(point);
            last = point;
        }

		public Track cleanup_speed()
		{
			Track retv = new Track(); 

			retv.name = this.name;

			var num_points = this.points.length();
			var mean =  this.get_track_average();
			var deviation = 0.0;


			List<weak Point > list_copy = this.points.copy();
			weak List<weak Point> iter = list_copy.first();
			while(iter != null)
			{
				var diff = iter.data.speed-mean;
				deviation += (diff*diff);
				iter = iter.next;
			}
			deviation /= num_points; 

			iter = list_copy.first();
			uint i =0;
			while(iter != null)
			{
				var pspeed = iter.data.speed;
				if(iter.next != null){
					pspeed = iter.next.data.speed;
				}
				var pdf =
				(1/Math.sqrt(2*Math.PI*deviation))*GLib.Math.exp(-((pspeed-mean)*(pspeed-mean))/(2*deviation));
				if((num_points*pdf) < 0.1 && !iter.data.stopped) {
					/* Remove point, fix speed off the next point, as it should */
					weak List<weak Point> temp = iter.prev;
					list_copy.remove_link(iter);	
					if(temp != null)
					{
						iter = temp;
						if(iter.next != null){
							iter.next.data.speed = calculate_point_to_point_speed(
									iter.data, iter.next.data);
						}
					}
					else
					{
						i =0;
						iter = list_copy.first();
						if(iter != null) {
							iter.data.speed = 0.0;
							iter.data.distance = 0.0;
							if(iter.next != null){
								iter.next.data.distance = calculate_distance(iter.data, iter.next.data);
								iter.next.data.speed = calculate_point_to_point_speed(iter.data, iter.next.data);
							}
						}
					}
				}else{
					/* Skip this */
					iter = iter.next;
					i++;
				}
			}
			/* Add remaining points to new track */
			iter = list_copy.first();
			while(iter != null)
			{
				retv.add_point(iter.data);
				iter = iter.next;
			}
			return retv; 
		}


        /**
         * Calculate the speed of the full track
         *
         * @returns the average speed of the full track in km/h.
         */

        public double get_track_average()
        {
            weak List<Point ?> first = this.points.first();
            weak List<Point ?> last = this.points.last();
            if(first != null && last != null)
            {
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
            double speed = (60.0*60.0)*dist/((tb-ta));
            return speed;
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
        public double calculate_moving_average(Gpx.Point start, Gpx.Point stop, out time_t moving_time)
        {
            double time = 0;
            double distance = 0;
            moving_time = 0;
            weak List<Point?> iter = this.points.find(start);
            if(iter == null) return 0;
            if((iter)!=null)
            {
                while((iter = iter.next) != null && iter.prev.data != stop)
                {
                    Point b  = iter.data;
//                    if(((b.distance-iter.prev.data.distance)*3600)/(b.get_time()-iter.prev.data.get_time()) > 1.0)
					if(!b.stopped)
                    {
                        time += (b.get_time()-(iter.prev.data).get_time());
                        distance += b.distance-iter.prev.data.distance;
                    }
                }
            }
            moving_time = (time_t)time;
            return distance/(time/(60.0*60.0));
        }

        public void calculate_total_elevation(Gpx.Point start, Gpx.Point stop, out double up, out double down)
        {
			up = 0.0;
			down = 0.0;
            weak List<Point?> iter = this.points.find(start);
            weak List<Point?> last = null;
            if(iter == null) return;
			do {
				if (last != null) {
					if(iter.data.smooth_elevation > last.data.smooth_elevation) {
						up += (iter.data.smooth_elevation - last.data.smooth_elevation);
					} else {
						down += (last.data.smooth_elevation - iter.data.smooth_elevation);
					}
				}
				last = iter;
			} while((iter = iter.next) != null && iter.prev.data != stop);
        }
		/**
		 * @param lon_a longitude in radians of point a
		 * @param lat_a latitude in radians of point a
		 * @param lon_b longitude in radians of point b
		 * @param lat_a latitude in radians of point b
         * Calculate distance between point a and point b using great circular distance method
         * Elevation is not taken into account.
         *
         * @returns distance in km.		 
		 */
        public static double calculate_distance_coords(double lon_a, double lat_a, double ele_a,  double lon_b, double lat_b, double ele_b)
        {
            double retv =0;
            retv = 6378.7 * Math.acos(
                Math.sin(lat_a) *  Math.sin(lat_b) +
                Math.cos(lat_a) * Math.cos(lat_b) * Math.cos(lon_b - lon_a)
                );
            if(GLib.Math.isnan(retv) == 1)
            {
                return 0;
            }
            double ele = (ele_b - ele_a)/1000;
            retv = Math.sqrt(Math.pow(retv, 2)+Math.pow(ele, 2));
            return GLib.Math.fabs(retv);
        }

        /**
         * @param a the first Gpx.Point
         * @param b the second Gpx.Point
         *
         * Calculate distance between point a and point b using great circular distance method
         * Elevation is not taken into account.
         *
         * @returns distance in km.
         */
        public static double calculate_distance(Point a, Point b)
        {
            if(a.lat == b.lat && a.lon == b.lon) return 0;
            return calculate_distance_coords(a.lon,a.lat,a.elevation, b.lon,b.lat,b.elevation);
        }


        public uint heartrate_avg(Point start, Point stop)
        {
            double total = 0;
            double total_time = 0.0;
            Point *prev = null;
            weak List<Point?> iter = this.points.find(start);
            if(iter == null) return 0;
            do {
                var p = iter.data;
                if(p.tpe.heartrate != 0) {
                    if(prev == null) {
                        prev = p;
                    }else{
                        double diff = (double)p.get_time()-(double)prev->get_time();
/*                        if(!p.stopped) */
                        {
                            total+= prev->tpe.heartrate*diff;
                            total_time+=diff;
                        }

                        prev = p;
                    }
                }
                iter = iter.next;
            } while(iter != null && iter.prev.data != stop);
            return (total_time > 0)?(uint)(total/total_time):0;
        }

        public uint get_burned_calories()
        {
            return this.hrmt.calories;
        }
        public void set_burned_calories(uint value)
        {
            this.hrmt.calories = value;
        }

        public signal void point_removed(Point point);

        public void remove_point(Point point, bool update=true)
        {
            points.remove(point);
            if (update) {
                recalculate();
                point_removed(point);
            }
        }

    }
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

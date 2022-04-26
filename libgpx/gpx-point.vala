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
    public struct HeartRateMonitorPoint 
    {
        public int heartrate;
    }

    /**
     * Represents a point in the track or a waypoint.
     */
	
    public class Point
    {
        /* Waypoint name */
        public string name {get; set; default=null;}
        /* Position, in radians and degrees, 1000 means not set */
        public double lat = 1000;
        public double lon = 1000;
        public double lat_dec = 1000;
        public double lon_dec = 1000;
        /* The distance from start of track. (only if part of track */
        public double distance =0;
        /* Elevation */
        public double elevation;
		public double smooth_elevation;
        /* Time */
        public string time;
        /* The speed (only if part of track */
        public double speed = 0;
		/* indicate if stopped */
		public bool stopped = false;

        private time_t utime  = 0;
        private DateTime datetime;

        public uint32 cadence = 0;


        public HeartRateMonitorPoint tpe = HeartRateMonitorPoint() {
            heartrate = 0
        };


        public bool has_position()
        {
            return !(lat == 1000 || lon == 1000);
        }

        /**
         * Make a clean copy off the point.
         * Only position and time is copied.
         */
        public Gpx.Point copy()
        {
            Gpx.Point p = new Gpx.Point();
            p.name = this.name;
            p.lat = this.lat;
            p.lon = this.lon;
            p.lat_dec = this.lat_dec;
            p.lon_dec = this.lon_dec;
            p.time = this.time;
            p.elevation = this.elevation;
			p.smooth_elevation = this.smooth_elevation;
            p.tpe = this.tpe;
            return p;
        }
        public void set_position_lat(double lat_d)
        {
            this.lat_dec = lat_d;
            this.lat = (2*GLib.Math.PI*lat_d)/360.0;
        }
        public void set_position_lon(double lon_d)
        {
            this.lon_dec = lon_d;
            this.lon = (2*GLib.Math.PI*lon_d)/360.0;
        }
        /* Sets the poistion in degrees, automagically calculates radians */
        public void set_position(double lat, double lon)
        {
            this.lat_dec = lat;
            this.lon_dec = lon;
            this.lat = (2*GLib.Math.PI*lat)/360.0;
            this.lon = (2*GLib.Math.PI*lon)/360.0;
        }
        /* Get the unix time. (calculated on first request, then cached ) */
        public void set_utime ( time_t ut )
        {
            this.utime = ut;
        }
        public time_t get_time()
        {
            if(this.utime > 0)
                return this.utime;
            if(this.time == null) return 0;
            Time ta = Time();
            ta.strptime(this.time, "%FT%T%z");
            this.utime = ta.mktime();
            return utime;
        }
        public DateTime get_datetime()
        {
            if (datetime == null)
                datetime = new DateTime.from_unix_utc(get_time());
            return datetime;
        }
    }
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

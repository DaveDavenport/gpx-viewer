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
    /**
     * This is the top level class representing the gpx file it self.
     * This contains a list of tracks and waypoints.
     */
    public abstract class FileBase : GLib.Object
    {

        /* A gpx file can contain multiple tracks, this supports it */
        public GLib.List<Gpx.Track> tracks = null;
        /* A gpx file can also contains a list of waypoints */
        public GLib.List<Gpx.Point> waypoints = null;
        /* A gpx file can also contains a list of Routes */
        public GLib.List<Gpx.Track> routes = null;

        /* The file behind it */
        protected GLib.File file = null;

        protected string creator;
        protected string name;
        protected string description;
        protected string time;
        protected string keywords;

        /**
         * Helpers
         */
        public string get_keywords()
        {
            return keywords;
        }

        public string get_time()
        {
            return time;
        }

        public string get_name()
        {
            return name;
        }

        public string get_description()
        {
            return description;
        }

        public string get_creator()
        {
            return creator;
        }

        public string get_uri()
        {
            return file.get_uri();
        }


        public string get_basename()
        {
            return file.get_basename();
        }

        /**
         * Accessors
         */
        public unowned List<Gpx.Track> get_tracks()
        {
            return tracks;
        }


        public unowned GLib.List <Gpx.Point> get_waypoints() 
        {
            return waypoints;
        }


        public unowned List<Gpx.Track> get_routes()
        {
            return routes;
        }
    }
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

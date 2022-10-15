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
    public class JsonFile : FileBase 
    {

        /**
         * Parse a file
         */

        /* IO Functions */
        /* Used for paring */
        private GLib.FileInputStream stream = null;
        private int read_file(uint8[] buffer)
        {
            try
            {
                var value = this.stream.read(buffer, null);
                return (int)value;
            }
            catch (GLib.Error e)
            {
                GLib.critical("error reading from stream: %s\n", e.message);
                return -1;
            }
        }
        private int close_file()
        {
            GLib.log("GPX PARSER", GLib.LogLevelFlags.LEVEL_DEBUG, "Close_file()");
            this.stream = null;
            return 0;
        }

        public JsonFile (GLib.File file)
        {
            this.file = file;
            try
            {
                this.stream = file.read(null);
                var dstream = new DataInputStream(this.stream);
                Gpx.Track track = new Gpx.Track();
                string line;
                while ((line = dstream.read_line (null)) != null)
                {
                    
                    debug("%s", line);
                    var parser = new Json.Parser ();
                    parser.load_from_data ((string) line, -1);

                    Point p = new Point();
                    double flat = parser.get_root().get_object().get_double_member("latitude"); 
                    double flon = parser.get_root().get_object().get_double_member("longitude"); 

                    var time = new DateTime.utc (
                            (int)parser.get_root().get_object().get_int_member("year"),
                            (int)parser.get_root().get_object().get_int_member("month"),
                            (int)parser.get_root().get_object().get_int_member("day"),
                            (int)parser.get_root().get_object().get_int_member("hour"),
                            (int)parser.get_root().get_object().get_int_member("minute"),
                             0);
                    print("%lld\r\n", time.to_unix());
                    p.set_utime((int32)time.to_unix());
                    p.set_position(flat, flon);
                    track.add_point(p);

                }
                track.filter_points();
                this.tracks.append(track);
            }
            catch (GLib.Error e)
            {
                GLib.critical("failed to open file: '%s' error: %s",this.file.get_uri(), e.message);
            }

        }
    }
}

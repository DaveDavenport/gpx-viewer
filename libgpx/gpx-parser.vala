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

    public struct HeartRateMonitorTrack
    {
        public uint32 calories;
    }


    public errordomain FileError {
        INVALID_FILE,
        IO_ERROR,

    }

    /**
     * @param file A GLib.File to open.
     * 
     * Tries to open the file.. check extension, if that fails, try it.
     *
     * @returns a file.
     * @throws a FileError
     */
    public FileBase? file_open(GLib.File path) throws FileError
    {
        try {
            // Test if fit file.
            if(path.get_uri().has_suffix("fit")) {
                return new Gpx.FitFile(path);
            }
            // Test if gpx file.
            if(path.get_uri().has_suffix("gpx")) {
                return new Gpx.XmlFile(path);
            }
            // Test if gpx file.
            if(path.get_uri().has_suffix("json")) {
                return new Gpx.JsonFile(path);
            }
            // Try, FIT first, it detects header.
            FileBase f = new Gpx.FitFile(path);
            return f;
        } catch (Error err) {
            return new Gpx.XmlFile(path);
        }
    }
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */

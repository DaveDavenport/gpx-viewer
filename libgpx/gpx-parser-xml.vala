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
    public class XmlFile : FileBase
    {

        /* IO Functions */
        /* Used for paring */
        private GLib.FileInputStream stream = null;

        private Gpx.Point parse_point(Xml.Node *point)
        {
            var lat = point->get_prop("lat");
            var lon = point->get_prop("lon");
            if(lat != null && lon != null)
            {
                Point p = new Point();
                // TODO: Move parsing into Point class.
                double flat = double.parse(lat);
                double flon = double.parse(lon);
                p.set_position(flat, flon);
                var pointNode = point->children;
                while(pointNode != null)
                {
                    /* height */
                    if(pointNode->name == "ele")
                    {
                        var content = pointNode->get_content();
                        if(content != null)
                            p.elevation = double.parse(content);
                    }
                    else if (pointNode->name == "name")
                    {
                        p.name = pointNode->get_content();
                    }
                    else if (pointNode->name == "desc")
                    {
                        p.description = pointNode->get_content();
                    }
                    else if (pointNode->name == "time")
                    {
                        p.time = pointNode->get_content();
                    }
                    else if (pointNode->name == "extensions")
                    {
                        var exts = pointNode->children;
                        for(; exts != null; exts = exts->next) {
                            if(exts->name == "TrackPointExtension") {
                                // Parse trackpoint
                                var ext = exts->children;
                                for(; ext != null; ext = ext->next)
                                {
                                    if(ext->name == "hr") {
                                        var val= ext->get_content();
                                        p.tpe.heartrate = int.parse(val);
                                    }
                                }
                            }
                        }
                    }
                    pointNode = pointNode->next;
                }

                if(p.time != null) {
                    return p;
                }
            }
            return null;
        }

        //TODO: Use dedicated metadata
        private void parse_metadata(Xml.Node *metadata)
        {
            var metadataChild = metadata->children;
            /* iterretate over track segments */
            while(metadataChild != null)
            {
                //TODO:
                //author> personType
                //link> linkType
                //copyright> copyrightType
                //bounds> boundsType
                //extensions> extensionsType
                if(metadataChild->name == "name")
                    name = metadataChild->get_content();
                if(metadataChild->name == "desc")
                    description = metadataChild->get_content();
                if(metadataChild->name == "time")
                    time = metadataChild->get_content();
                if(metadataChild->name == "keywords")
                    keywords = metadataChild->get_content();
                metadataChild = metadataChild->next;
            }
        }

        private Gpx.Track parse_track(Xml.Node *trk)
        {
            /* Create new track here */
            Gpx.Track track = new Gpx.Track();

            var trkChild = trk->children;
            /* iterretate over track segments */
            while(trkChild != null)
            {
                if(trkChild->name == "trkseg")
                {
                    var point = trkChild->children;
                    while(point != null)
                    {
                        if(point->name == "trkpt")
                        {
                            Point p = parse_point(point);
                            if (p != null) {
                                track.add_point(p);
                            }
                            else
                            {
                                GLib.message("Failed to get point: %s\n", point->name);
                            }
                        }
                        point = point->next;
                    }
                }
                else if(trkChild->name == "name")
                {
                    if(track.name == null)
                    {
                        track.name = trkChild->get_content();
                    }
                    else
                    {
                        GLib.warning("Track name allready set: %s\n", track.name);
                    }
                }
                else if(trkChild->name == "type")
                {
                    if(track.track_type == null)
                    {
                        track.track_type = trkChild->get_content();
                    }
                    else
                    {
                        GLib.warning("Track type allready set: %s\n", track.track_type);
                    }
                }
                else if(trkChild->name == "number")
                {
                    if(track.number == null)
                    {
                        track.number = int.parse(trkChild->get_content());
                    }
                    else
                    {
                        GLib.warning("Track number allready set: %d\n", track.number);
                    }
                }

                trkChild = trkChild->next;
            }
            return track;
        }

        private void parse_waypoint(Xml.Node *node)
        {
            Point p = parse_point(node);
            if (p != null) {
                this.waypoints.append(p);
            }
        }

        private Gpx.Track parse_route(Xml.Node *node)
        {
            /* Create new track here */
            Gpx.Track track = new Gpx.Track();
            track.is_route = true;

            var trkseg = node->children;
            /* iterretate over track segments */
            while(trkseg != null)
            {
                if(trkseg->name == "rtept")
                {
                    Point p = parse_point(trkseg);
                    if (p != null) {
                       track.add_point(p);
                    }
                    else
                    {
                        GLib.message("Failed to get rtept: %s\n", trkseg->name);
                    }
                }
                else if(trkseg->name == "name")
                {
                    if(track.name == null)
                    {
                        track.name = trkseg->get_content();
                    }
                    else
                    {
                        GLib.warning("Track name allready set: %s\n", track.name);
                    }
                }
                else if(trkseg->name == "type")
                {
                    if(track.track_type == null)
                    {
                        track.track_type = trkseg->get_content();
                    }
                    else
                    {
                        GLib.warning("Track type allready set: %s\n", track.track_type);
                    }
                }
                else if(trkseg->name == "number")
                {
                    if(track.number == null)
                    {
                        track.number = int.parse(trkseg->get_content());
                    }
                    else
                    {
                        GLib.warning("Track number allready set: %d\n", track.number);
                    }
                }
                trkseg = trkseg->next;
            }
            return track;
        }
        /**
         * Parse a file
         */

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

        public XmlFile (GLib.File file)
        {
            this.file = file;
            try
            {
                this.stream = file.read(null);
                Xml.TextReader reader = new Xml.TextReader.for_io(
                    (Xml.InputReadCallback)read_file,
                    (Xml.InputCloseCallback) close_file,this,
                    this.file.get_uri(), "", 0);
                if(reader != null)
                {
                    /* Start parsing the xml file */
                    var doc = reader.read();
                    while(doc == 1)
                    {
                        var name = reader.const_name();
                        if(name == "gpx")
                        {
                            var gpx = reader.expand();
                            this.creator = gpx->get_prop("creator");
                            int doc2 = reader.read();
                            while(doc2 == 1)
                            {
                                var name2 = reader.const_name();
                                 /* Get the track element */
                                if(name2 == "metadata")
                                {
                                    var metadata = reader.expand();
                                    parse_metadata(metadata);
                                }
                                else if(name2 == "trk")
                                {
                                    /* Track */
                                    var trk = reader.expand();
                                    var track = this.parse_track(trk);
                                    track.filter_points();
                                    this.tracks.append(track);
                                }
                                else if (name2 == "wpt")
                                {
                                    /* Waypoint */
                                    var node = reader.expand();
                                    this.parse_waypoint(node);
                                }
                                else if (name2 == "rte")
                                {
                                    var node = reader.expand();
                                    /* Route */
                                    var track = this.parse_route(node);
                                    this.routes.append(track);
                                }
                                doc2 = reader.next();
                            }
                        }
                        else
                            doc = reader.read();
                    }
                }
                else
                {
                    /* Todo add error trower here
                       http://www.vala-project.org/doc/vala-draft/errors.html#exceptionsexamples*/
                    GLib.message("Failed to open file");
                }
                reader.close();
            }
            catch (GLib.Error e)
            {
                GLib.critical("failed to open file: '%s' error: %s",this.file.get_uri(), e.message);
            }

        }
    }
}

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

#include "gpx-writer.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <locale.h>

static int write_point(xmlTextWriterPtr writer, GpxPoint *point, gchar *pointNodeName) {
    int rc;
    rc = xmlTextWriterStartElement(writer, BAD_CAST pointNodeName);
    if (rc < 0) {
        g_debug ("error adding %s\n", pointNodeName);
        return -1;
    }

    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "lat",
                                           "%.10g", point->lat_dec);
    if (rc < 0) {
        g_debug ("error adding lat\n");
        return -1;
    }

    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "lon",
                                           "%.10g", point->lon_dec);
    if (rc < 0) {
        g_debug ("error adding lon\n");
        return -1;
    }

    if (gpx_point_get_name(point) != NULL) {
        rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "name",
                                             "%s", gpx_point_get_name(point));
        if (rc < 0) {
            g_debug ("error adding name\n");
            return -1;
        }
    }

    if (gpx_point_get_description(point) != NULL) {
        rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "desc",
                                             "%s", gpx_point_get_description(point));
        if (rc < 0) {
            g_debug ("error adding description\n");
            return -1;
        }
    }
    if (point->elevation != 0) {
        rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ele",
                                             "%g", point->elevation);
        if (rc < 0) {
            g_debug ("error adding ele\n");
            return -1;
        }
    }

    rc = xmlTextWriterWriteElement(writer, BAD_CAST "time",
                                   BAD_CAST point->time);
    if (rc < 0) {
        g_debug ("error adding time\n");
        return -1;
    }

    //close trkpt
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        g_debug ("error closing %s\n", pointNodeName);
        return -1;
    }
    return 0;
}
static int write_track(xmlTextWriterPtr writer, GpxTrack *track) {
    int rc;
    GList *list;
    GList *iter;

    gboolean is_route = gpx_track_get_is_route(track);
    gchar * nodeName = "trk";
    gchar * pointNodeName = "trkpt";
    if (is_route) {
        nodeName = "rte";
        pointNodeName = "rtept";
    }

    rc = xmlTextWriterStartElement(writer, BAD_CAST nodeName);
    if (rc < 0) {
        g_debug ("error adding trk\n");
        return -1;
    }

    if (gpx_track_get_name(track) != NULL) {
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
                                       BAD_CAST gpx_track_get_name(track));
        if (rc < 0) {
            g_debug ("error adding name\n");
            return -1;
        }
    }

    if (gpx_track_get_track_type(track) != NULL) {
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "type",
                                       BAD_CAST gpx_track_get_track_type(track));
        if (rc < 0) {
            g_debug ("error adding type\n");
            return -1;
        }
    }

    if (gpx_track_get_number(track) != NULL) {
        rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "number",
                                       "%d", *(int*)gpx_track_get_number(track));
        if (rc < 0) {
            g_debug ("error adding type\n");
            return -1;
        }
    }

    if (!is_route) {
        rc = xmlTextWriterStartElement(writer, BAD_CAST "trkseg");
        if (rc < 0) {
            g_debug ("error adding trkseg\n");
            return -1;
        }
    }

    for(list = g_list_first(track->points); list != NULL; list = g_list_next(list))
    {
        GpxPoint *point = (GpxPoint *)list->data;

        rc = write_point(writer, point, pointNodeName);
        if (rc < 0) {
            g_debug ("error adding point\n");
            return -1;
        }

    }

    if (!is_route) {
        //close trkseg
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            g_debug ("error closing trkseg\n");
            return -1;
        }
    }

    //close trk
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        g_debug ("error closing trk\n");
        return -1;
    }
    return 0;
}

int gpx_write(GpxFileBase *file, gchar *filename) {

    GList *iter;
    xmlTextWriterPtr writer;
    xmlChar *tmp;
    xmlDocPtr doc;
    int rc;

    setlocale (LC_ALL, "C");

    writer = xmlNewTextWriterDoc(&doc, 0);
    if (writer == NULL) {
        g_debug("testXmlwriterDoc: Error creating the xml writer\n");
        return -1;
    }

    rc = xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
    if (rc < 0) {
        g_debug("error adding prolog\n");
        return -1;
    }

    rc = xmlTextWriterStartElement(writer, BAD_CAST "gpx");
    if (rc < 0) {
        g_debug
            ("error adding gpx\n");
        return -1;
    }

    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "creator",
                                     "%s", gpx_file_base_get_creator(file));
    if (rc < 0) {
        g_debug("error adding creator\n");
        return -1;
    }

    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "version",
                                     BAD_CAST "1.1");
    if (rc < 0) {
        g_debug("error adding version\n");
        return -1;
    }

    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:xsi",
                                     BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");
    if (rc < 0) {
        g_debug ("error adding xmlns:xsi attribute\n");
        return -1;
    }

    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xsi:schemaLocation",
                                     BAD_CAST "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");
    if (rc < 0) {
        g_debug ("error adding xsi:schemaLocation\n");
        return -1;
    }

    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns",
                                     BAD_CAST "http://www.topografix.com/GPX/1/1");
    if (rc < 0) {
        g_debug ("error adding xmlnsattribute\n");
        return -1;
    }

    rc = xmlTextWriterStartElement(writer, BAD_CAST "metadata");
    if (rc < 0) {
        g_debug ("error adding metadata\n");
        return -1;
    }

    if (gpx_file_base_get_name(file) != NULL) {
        rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "name",
                                       "%s", gpx_file_base_get_name(file));
        if (rc < 0) {
            g_debug ("error adding name metadata\n");
            return -1;
        }
    }

    if (gpx_file_base_get_time(file) != NULL) {
        rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "time",
                                       "%s", gpx_file_base_get_time(file));
        if (rc < 0) {
            g_debug ("error adding time metadata\n");
            return -1;
        }
    }

    //close metadata
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        g_debug ("error creating writter\n");
        return -1;
    }

    for (iter = g_list_first(gpx_file_base_get_waypoints(file)); iter; iter = g_list_next(iter))
    {
        GpxPoint *point = GPX_POINT(iter->data);
        rc = write_point(writer, point, "wpt");
        if (rc < 0) {
            g_debug ("error writing waypoint\n");
            return -1;
        }
    }

    for (iter = g_list_first(gpx_file_base_get_routes(file)); iter; iter = g_list_next(iter))
    {
        GpxTrack *track = GPX_TRACK(iter->data);
        rc = write_track(writer, track);
        if (rc < 0) {
            g_debug ("error writing routes\n");
            return -1;
        }
    }

    for (iter = g_list_first(gpx_file_base_get_tracks(file)); iter; iter = g_list_next(iter))
    {
        GpxTrack *track = GPX_TRACK(iter->data);
        rc = write_track(writer, track);
        if (rc < 0) {
            g_debug ("error writing track\n");
            return -1;
        }
    }

    //close gpx
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        g_debug ("error closing gpx\n");
        return -1;
    }

    xmlFreeTextWriter(writer);
    xmlSaveFormatFileEnc (filename, doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    setlocale (LC_ALL, "");
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=120: */

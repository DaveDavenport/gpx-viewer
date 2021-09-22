#ifndef __GPX_WRITER_H__
#define __GPX_WRITER_H__

#include <gtk/gtk.h>
#include "gpx.h"

/**
 * This structure holds all information related to
 * a track.
 * The file the track belongs to, the path on the map start/stop marker
 * and the visible state.
 */
typedef struct Route
{
    GpxFileBase *file;
    GpxTrack *track;
    GpxViewerPathLayer *path;
    ChamplainMarker *start;
    ChamplainMarker *stop;
    gboolean visible;
} Route;

int gpx_write(GpxFileBase *file, gchar * filename);
#endif

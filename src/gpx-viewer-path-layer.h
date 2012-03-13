/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2011 Jiri Techet <techet@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GPX_VIEWER_PATH_LAYER_H
#define GPX_VIEWER_PATH_LAYER_H

#include <champlain/champlain.h>
#include <glib-object.h>
#include <clutter/clutter.h>
#include "gpx.h"

G_BEGIN_DECLS

#define GPX_VIEWER_TYPE_PATH_LAYER gpx_viewer_path_layer_get_type ()

#define GPX_VIEWER_PATH_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPX_VIEWER_TYPE_PATH_LAYER, GpxViewerPathLayer))

#define GPX_VIEWER_PATH_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GPX_VIEWER_TYPE_PATH_LAYER, GpxViewerPathLayerClass))

#define GPX_VIEWER_IS_PATH_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPX_VIEWER_TYPE_PATH_LAYER))

#define GPX_VIEWER_IS_PATH_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPX_VIEWER_TYPE_PATH_LAYER))

#define GPX_VIEWER_PATH_LAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPX_VIEWER_TYPE_PATH_LAYER, GpxViewerPathLayerClass))

typedef struct _GpxViewerPathLayerPrivate GpxViewerPathLayerPrivate;

typedef struct _GpxViewerPathLayer GpxViewerPathLayer;
typedef struct _GpxViewerPathLayerClass GpxViewerPathLayerClass;


/**
 * GpxViewerPathLayer:
 *
 * The #GpxViewerPathLayer structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.10
 */
struct _GpxViewerPathLayer
{
  ChamplainLayer parent;

  GpxViewerPathLayerPrivate *priv;
};

struct _GpxViewerPathLayerClass
{
  ChamplainLayerClass parent_class;
};

GType gpx_viewer_path_layer_get_type (void);

GpxViewerPathLayer *gpx_viewer_path_layer_new (void);

void gpx_viewer_path_layer_set_track(GpxViewerPathLayer *layer,
    GpxTrack *track);

ClutterColor *gpx_viewer_path_layer_get_stroke_color (GpxViewerPathLayer *layer);
void gpx_viewer_path_layer_set_stroke_color (GpxViewerPathLayer *layer,
    const ClutterColor *color);

gdouble gpx_viewer_path_layer_get_stroke_width (GpxViewerPathLayer *layer);
void gpx_viewer_path_layer_set_stroke_width (GpxViewerPathLayer *layer,
    gdouble value);

gboolean gpx_viewer_path_layer_get_visible (GpxViewerPathLayer *layer);
void gpx_viewer_path_layer_set_visible (GpxViewerPathLayer *layer,
    gboolean value);

GList *gpx_viewer_path_layer_get_dash (GpxViewerPathLayer *layer);
void gpx_viewer_path_layer_set_dash (GpxViewerPathLayer *layer,
    GList *dash_pattern);

G_END_DECLS

#endif

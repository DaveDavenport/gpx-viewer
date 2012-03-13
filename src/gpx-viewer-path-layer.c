/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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
/**
 * TODO:
 * Accept GpxTrack, not points.
 * Then remove object type from point.
 */
/**
 * SECTION:champlain-path-layer
 * @short_description: A layer displaying line path between inserted #GpxPoint
 * objects
 *
 * This layer shows a connection between inserted objects implementing the
 * #GpxPoint interface. This means that both #ChamplainMarker
 * objects and #ChamplainCoordinate objects can be inserted into the layer.
 * Of course, custom objects implementing the #GpxPoint interface
 * can be used as well.
 */
#define CHAMPLAIN_PARAM_READABLE     \
  (G_PARAM_READABLE |     \
   G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB)

#define CHAMPLAIN_PARAM_READWRITE    \
  (G_PARAM_READABLE | G_PARAM_WRITABLE | \
   G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB)


#include "gpx-viewer-path-layer.h"
#include "gpx.h"
#include "config.h"
#include <math.h>

#include "gpx-viewer-path-layer.h"

#include <champlain/champlain.h>

#include <clutter/clutter.h>
#include <glib.h>

G_DEFINE_TYPE (GpxViewerPathLayer, gpx_viewer_path_layer, CHAMPLAIN_TYPE_LAYER)

#define GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GPX_VIEWER_TYPE_PATH_LAYER, GpxViewerPathLayerPrivate))

typedef struct _Color{
	double r;
	double g;
	double b;
}Color;

const int max_height_colors = 5;
const Color height_colors[] =
{
	{0.0,0.0,1.0},
	{0.0,1.0,1.0},
	{0.0,1.0,0.0},
	{1.0,0.8,0.0},
	{1.0,0.0,0.0}
};
enum
{
  /* normal signals */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_STROKE_WIDTH,
  PROP_STROKE_COLOR,
  PROP_VISIBLE,
};

static ClutterColor DEFAULT_STROKE_COLOR = { 0xa4, 0x00, 0x00, 0xff };

/* static guint signals[LAST_SIGNAL] = { 0, }; */

struct _GpxViewerPathLayerPrivate
{
  ChamplainView *view;

  ClutterColor *stroke_color;
  gboolean fill;
  ClutterColor *fill_color;
  gboolean stroke;
  gdouble stroke_width;
  gboolean visible;
  gdouble *dash;
  guint num_dashes;

  ClutterGroup *content_group;
  ClutterActor *path_actor;
  GpxTrack *track;
  gboolean redraw_scheduled;
};


static gboolean redraw_path (GpxViewerPathLayer *layer);
static void schedule_redraw (GpxViewerPathLayer *layer);

static void set_view (ChamplainLayer *layer,
    ChamplainView *view);

static ChamplainBoundingBox *get_bounding_box (ChamplainLayer *layer);


static void
gpx_viewer_path_layer_get_property (GObject *object,
    guint property_id,
    G_GNUC_UNUSED GValue *value,
    GParamSpec *pspec)
{
  GpxViewerPathLayer *self = GPX_VIEWER_PATH_LAYER (object);
  GpxViewerPathLayerPrivate *priv = self->priv;

  switch (property_id)
    {

    case PROP_STROKE_COLOR:
      clutter_value_set_color (value, priv->stroke_color);
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_double (value, priv->stroke_width);
      break;

    case PROP_VISIBLE:
      g_value_set_boolean (value, priv->visible);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
gpx_viewer_path_layer_set_property (GObject *object,
    guint property_id,
    G_GNUC_UNUSED const GValue *value,
    GParamSpec *pspec)
{
  switch (property_id)
    {


    case PROP_STROKE_COLOR:
      gpx_viewer_path_layer_set_stroke_color (GPX_VIEWER_PATH_LAYER (object),
          clutter_value_get_color (value));
      break;

    case PROP_STROKE_WIDTH:
      gpx_viewer_path_layer_set_stroke_width (GPX_VIEWER_PATH_LAYER (object),
          g_value_get_double (value));
      break;

    case PROP_VISIBLE:
      gpx_viewer_path_layer_set_visible (GPX_VIEWER_PATH_LAYER (object),
          g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
paint (ClutterActor *self)
{
  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (self);

  clutter_actor_paint (CLUTTER_ACTOR (priv->content_group));
}


static void
pick (ClutterActor *self,
    const ClutterColor *color)
{
  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (self);

  CLUTTER_ACTOR_CLASS (gpx_viewer_path_layer_parent_class)->pick (self, color);

  clutter_actor_paint (CLUTTER_ACTOR (priv->content_group));
}


static void
get_preferred_width (ClutterActor *self,
    gfloat for_height,
    gfloat *min_width_p,
    gfloat *natural_width_p)
{
  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (self);

  clutter_actor_get_preferred_width (CLUTTER_ACTOR (priv->content_group),
      for_height,
      min_width_p,
      natural_width_p);
}


static void
get_preferred_height (ClutterActor *self,
    gfloat for_width,
    gfloat *min_height_p,
    gfloat *natural_height_p)
{
  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (self);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->content_group),
      for_width,
      min_height_p,
      natural_height_p);
}


static void
allocate (ClutterActor *self,
    const ClutterActorBox *box,
    ClutterAllocationFlags flags)
{
  ClutterActorBox child_box;

  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (self);

  CLUTTER_ACTOR_CLASS (gpx_viewer_path_layer_parent_class)->allocate (self, box, flags);

  child_box.x1 = 0;
  child_box.x2 = box->x2 - box->x1;
  child_box.y1 = 0;
  child_box.y2 = box->y2 - box->y1;

  clutter_actor_allocate (CLUTTER_ACTOR (priv->content_group), &child_box, flags);
}


static void
map (ClutterActor *self)
{
  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (self);

  CLUTTER_ACTOR_CLASS (gpx_viewer_path_layer_parent_class)->map (self);

  clutter_actor_map (CLUTTER_ACTOR (priv->content_group));
}


static void
unmap (ClutterActor *self)
{
  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (self);

  CLUTTER_ACTOR_CLASS (gpx_viewer_path_layer_parent_class)->unmap (self);

  clutter_actor_unmap (CLUTTER_ACTOR (priv->content_group));
}


static void
gpx_viewer_path_layer_dispose (GObject *object)
{
  GpxViewerPathLayer *self = GPX_VIEWER_PATH_LAYER (object);
  GpxViewerPathLayerPrivate *priv = self->priv;

  if(priv->track)
  {
	g_object_unref(priv->track);
	priv->track = NULL;
  }

  if (priv->view != NULL)
    set_view (CHAMPLAIN_LAYER (self), NULL);

  if (priv->content_group)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->content_group));
      priv->content_group = NULL;
    }

  G_OBJECT_CLASS (gpx_viewer_path_layer_parent_class)->dispose (object);
}


static void
gpx_viewer_path_layer_finalize (GObject *object)
{
  GpxViewerPathLayer *self = GPX_VIEWER_PATH_LAYER (object);
  GpxViewerPathLayerPrivate *priv = self->priv;

  clutter_color_free (priv->stroke_color);
  g_free (priv->dash);

  G_OBJECT_CLASS (gpx_viewer_path_layer_parent_class)->finalize (object);
}


static void
gpx_viewer_path_layer_class_init (GpxViewerPathLayerClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ChamplainLayerClass *layer_class = CHAMPLAIN_LAYER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GpxViewerPathLayerPrivate));

  object_class->finalize = gpx_viewer_path_layer_finalize;
  object_class->dispose = gpx_viewer_path_layer_dispose;
  object_class->get_property = gpx_viewer_path_layer_get_property;
  object_class->set_property = gpx_viewer_path_layer_set_property;

  actor_class->get_preferred_width = get_preferred_width;
  actor_class->get_preferred_height = get_preferred_height;
  actor_class->allocate = allocate;
  actor_class->paint = paint;
  actor_class->pick = pick;
  actor_class->map = map;
  actor_class->unmap = unmap;

  layer_class->set_view = set_view;
  layer_class->get_bounding_box = get_bounding_box;


  /**
   * GpxViewerPathLayer:stroke-color:
   *
   * The path's stroke color
   *
   * Since: 0.10
   */
  g_object_class_install_property (object_class,
      PROP_STROKE_COLOR,
      clutter_param_spec_color ("stroke-color",
          "Stroke Color",
          "The path's stroke color",
          &DEFAULT_STROKE_COLOR,
          CHAMPLAIN_PARAM_READWRITE));


  /**
   * GpxViewerPathLayer:stroke-width:
   *
   * The path's stroke width (in pixels)
   *
   * Since: 0.10
   */
  g_object_class_install_property (object_class,
      PROP_STROKE_WIDTH,
      g_param_spec_double ("stroke-width",
          "Stroke Width",
          "The path's stroke width",
          0, 
          100.0,
          2.0,
          CHAMPLAIN_PARAM_READWRITE));

  /**
   * GpxViewerPathLayer:visible:
   *
   * Wether the path is visible
   *
   * Since: 0.10
   */
  g_object_class_install_property (object_class,
      PROP_VISIBLE,
      g_param_spec_boolean ("visible",
          "Visible",
          "The path's visibility",
          TRUE,
          CHAMPLAIN_PARAM_READWRITE));
}


static void
gpx_viewer_path_layer_init (GpxViewerPathLayer *self)
{
  GpxViewerPathLayerPrivate *priv;

  self->priv = GET_PRIVATE (self);
  priv = self->priv;
  priv->view = NULL;

  priv->visible = TRUE;
  priv->stroke_width = 2.0;
  priv->track = NULL;
  priv->redraw_scheduled = FALSE;
  priv->dash = NULL;
  priv->num_dashes = 0;

  priv->stroke_color = clutter_color_copy (&DEFAULT_STROKE_COLOR);

  priv->content_group = CLUTTER_GROUP (clutter_group_new ());
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->content_group), CLUTTER_ACTOR (self));

  priv->path_actor = clutter_cairo_texture_new (256, 256);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->content_group), priv->path_actor);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}


/**
 * gpx_viewer_path_layer_new:
 *
 * Creates a new instance of #GpxViewerPathLayer.
 *
 * Returns: a new instance of #GpxViewerPathLayer.
 *
 * Since: 0.10
 */
GpxViewerPathLayer *
gpx_viewer_path_layer_new ()
{
  return g_object_new (GPX_VIEWER_TYPE_PATH_LAYER, NULL);
}


static void
position_notify (GpxPoint *location,
    G_GNUC_UNUSED GParamSpec *pspec,
    GpxViewerPathLayer *layer)
{
  schedule_redraw (layer);
}

/**
 * gpx_viewer_path_layer_add_node:
 * @layer: a #GpxViewerPathLayer
 * @location: a #GpxPoint
 *
 * Adds a #GpxPoint object to the layer.
 *
 * Since: 0.10
 */
void
gpx_viewer_path_layer_set_track (GpxViewerPathLayer *layer,
    GpxTrack *track)
{
  GpxViewerPathLayerPrivate *priv;
  g_return_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer));
  g_return_if_fail (GPX_IS_TRACK(track));

  priv = layer->priv;
  if(priv->track) g_object_unref(priv->track);
  priv->track = g_object_ref_sink(track);

  schedule_redraw (layer);
}




static void
relocate_cb (G_GNUC_UNUSED GObject *gobject,
    GpxViewerPathLayer *layer)
{
  g_return_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer));

  schedule_redraw (layer);
}


static gboolean
redraw_path (GpxViewerPathLayer *layer)
{
  GpxViewerPathLayerPrivate *priv = layer->priv;
  cairo_t *cr;
  gfloat width, height;
  GList *elem;
  ChamplainView *view = priv->view;
  gint x, y;
  guint last_width, last_height;

  priv->redraw_scheduled = FALSE;

  /* layer not yet added to the view */
  if (view == NULL || !priv->content_group)
    return FALSE;

  clutter_actor_get_size (CLUTTER_ACTOR (view), &width, &height);

  if (!priv->visible || width == 0.0 || height == 0.0)
    return FALSE;

  clutter_cairo_texture_get_surface_size (CLUTTER_CAIRO_TEXTURE (priv->path_actor), &last_width, &last_height);

  if ((guint) width != last_width || (guint) height != last_height)
    clutter_cairo_texture_set_surface_size (CLUTTER_CAIRO_TEXTURE (priv->path_actor), width, height);

  champlain_view_get_viewport_origin (priv->view, &x, &y);
  clutter_actor_set_position (priv->path_actor, x, y);

  cr = clutter_cairo_texture_create (CLUTTER_CAIRO_TEXTURE (priv->path_actor));

  /* Clear the drawing area */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	// For colouring.. only > 50 meters difference.
  double min_elv = priv->track->min_elevation; 
  double max_elv = priv->track->max_elevation; 
  double range = MAX(250,max_elv-min_elv);


  cairo_set_line_width (cr, priv->stroke_width);
  cairo_set_dash(cr, priv->dash, priv->num_dashes, 0);
  double r,g,b=0;
  int old_val = -1;
  for (elem = g_list_first(priv->track->points); elem != NULL; elem = elem->next)
  {
	  GpxPoint *location = GPX_POINT (elem->data);
	  gfloat x, y;

	  x = champlain_view_longitude_to_x (view, location->lon_dec); 
	  y = champlain_view_latitude_to_y (view, location->lat_dec); 
	  int val = round((max_height_colors-1)*(location->elevation-min_elv)/(range));
	  if(val != old_val)
	  {		
		  cairo_line_to (cr, x, y);
		  cairo_stroke(cr);
		  cairo_set_source_rgb (cr,
				 height_colors[val].r, 
				 height_colors[val].g, 
				 height_colors[val].b);
		 old_val = (val);
	  }
	  cairo_line_to (cr, x, y);
  }

  cairo_stroke(cr);

  cairo_destroy (cr);

  return FALSE;
}


static void
schedule_redraw (GpxViewerPathLayer *layer)
{
  if (!layer->priv->redraw_scheduled)
    {
      layer->priv->redraw_scheduled = TRUE;
      g_idle_add_full (CLUTTER_PRIORITY_REDRAW,
          (GSourceFunc) redraw_path,
          g_object_ref (layer),
          (GDestroyNotify) g_object_unref);
    }
}


static void
redraw_path_cb (G_GNUC_UNUSED GObject *gobject,
    G_GNUC_UNUSED GParamSpec *arg1,
    GpxViewerPathLayer *layer)
{
  schedule_redraw (layer);
}


static void
set_view (ChamplainLayer *layer,
    ChamplainView *view)
{
  g_return_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer) && (CHAMPLAIN_IS_VIEW (view) || view == NULL));

  GpxViewerPathLayer *path_layer = GPX_VIEWER_PATH_LAYER (layer);

  if (path_layer->priv->view != NULL)
    {
      g_signal_handlers_disconnect_by_func (path_layer->priv->view,
          G_CALLBACK (relocate_cb), path_layer);

      g_signal_handlers_disconnect_by_func (path_layer->priv->view,
          G_CALLBACK (redraw_path_cb), path_layer);

      g_object_unref (path_layer->priv->view);
    }

  path_layer->priv->view = view;

  if (view != NULL)
    {
      g_object_ref (view);

      g_signal_connect (view, "layer-relocated",
          G_CALLBACK (relocate_cb), layer);

      g_signal_connect (view, "notify::latitude",
          G_CALLBACK (redraw_path_cb), layer);

      schedule_redraw (path_layer);
    }
}


static ChamplainBoundingBox *
get_bounding_box (ChamplainLayer *layer)
{
  GpxViewerPathLayerPrivate *priv = GET_PRIVATE (layer);
  GList *elem;
  ChamplainBoundingBox *bbox;

  bbox = champlain_bounding_box_new ();

  for (elem = priv->track->points; elem != NULL; elem = elem->next)
    {
      GpxPoint *location = GPX_POINT (elem->data);
      gdouble lat, lon;

      lat = location->lat_dec;
      lon = location->lon_dec;

      champlain_bounding_box_extend (bbox, lat, lon);
    }

  if (bbox->left == bbox->right)
    {
      bbox->left -= 0.0001;
      bbox->right += 0.0001;
    }

  if (bbox->bottom == bbox->top)
    {
      bbox->bottom -= 0.0001;
      bbox->top += 0.0001;
    }

  return bbox;
}



/**
 * gpx_viewer_path_layer_set_stroke_color:
 * @layer: a #GpxViewerPathLayer
 * @color: (allow-none): The path's stroke color or NULL to reset to the
 *         default color. The color parameter is copied.
 *
 * Set the path's stroke color.
 *
 * Since: 0.10
 */
void
gpx_viewer_path_layer_set_stroke_color (GpxViewerPathLayer *layer,
    const ClutterColor *color)
{
  g_return_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer));

  GpxViewerPathLayerPrivate *priv = layer->priv;

  if (priv->stroke_color != NULL)
    clutter_color_free (priv->stroke_color);

  if (color == NULL)
    color = &DEFAULT_STROKE_COLOR;

  priv->stroke_color = clutter_color_copy (color);
  g_object_notify (G_OBJECT (layer), "stroke-color");

  schedule_redraw (layer);
}


/**
 * gpx_viewer_path_layer_get_stroke_color:
 * @layer: a #GpxViewerPathLayer
 *
 * Gets the path's stroke color.
 *
 * Returns: the path's stroke color.
 *
 * Since: 0.10
 */
ClutterColor *
gpx_viewer_path_layer_get_stroke_color (GpxViewerPathLayer *layer)
{
  g_return_val_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer), NULL);

  return layer->priv->stroke_color;
}



/**
 * gpx_viewer_path_layer_set_stroke_width:
 * @layer: a #GpxViewerPathLayer
 * @value: the width of the stroke (in pixels)
 *
 * Sets the width of the stroke
 *
 * Since: 0.10
 */
void
gpx_viewer_path_layer_set_stroke_width (GpxViewerPathLayer *layer,
    gdouble value)
{
  g_return_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer));

  layer->priv->stroke_width = value;
  g_object_notify (G_OBJECT (layer), "stroke-width");

  schedule_redraw (layer);
}


/**
 * gpx_viewer_path_layer_get_stroke_width:
 * @layer: a #GpxViewerPathLayer
 *
 * Gets the width of the stroke.
 *
 * Returns: the width of the stroke
 *
 * Since: 0.10
 */
gdouble
gpx_viewer_path_layer_get_stroke_width (GpxViewerPathLayer *layer)
{
  g_return_val_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer), 0);

  return layer->priv->stroke_width;
}


/**
 * gpx_viewer_path_layer_set_visible:
 * @layer: a #GpxViewerPathLayer
 * @value: TRUE to make the path visible
 *
 * Sets path visibility.
 *
 * Since: 0.10
 */
void
gpx_viewer_path_layer_set_visible (GpxViewerPathLayer *layer,
    gboolean value)
{
  g_return_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer));

  layer->priv->visible = value;
  if (value)
    clutter_actor_show (CLUTTER_ACTOR (layer->priv->path_actor));
  else
    clutter_actor_hide (CLUTTER_ACTOR (layer->priv->path_actor));
  g_object_notify (G_OBJECT (layer), "visible");
}


/**
 * gpx_viewer_path_layer_get_visible:
 * @layer: a #GpxViewerPathLayer
 *
 * Gets path visibility.
 *
 * Returns: TRUE when the path is visible, FALSE otherwise
 *
 * Since: 0.10
 */
gboolean
gpx_viewer_path_layer_get_visible (GpxViewerPathLayer *layer)
{
  g_return_val_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer), FALSE);

  return layer->priv->visible;
}


/**
 * gpx_viewer_path_layer_set_dash:
 * @layer: a #GpxViewerPathLayer
 * @dash_pattern: (element-type guint): list of integer values representing lengths
 *     of dashes/spaces (see cairo documentation of cairo_set_dash())
 *
 * Sets dashed line pattern in a way similar to cairo_set_dash() of cairo. This 
 * method supports only integer values for segment lengths. The values have to be
 * passed inside the data pointer of the list (using the GUINT_TO_POINTER conversion)
 * 
 * Pass NULL to use solid line.
 * 
 * Since: 0.14
 */
void
gpx_viewer_path_layer_set_dash (GpxViewerPathLayer *layer,
    GList *dash_pattern)
{
  g_return_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer));

  GpxViewerPathLayerPrivate *priv = layer->priv;
  GList *iter;
  guint i;

  if (priv->dash)
    g_free (priv->dash);
  priv->dash = NULL;  

  priv->num_dashes = g_list_length (dash_pattern);

  if (dash_pattern == NULL) 
    return;

  priv->dash = g_new (gdouble, priv->num_dashes);
  for (iter = dash_pattern, i = 0; iter != NULL; iter = iter->next, i++)
    (priv->dash)[i] = (gdouble) GPOINTER_TO_UINT (iter->data);
}


/**
 * gpx_viewer_path_layer_get_dash:
 * @layer: a #GpxViewerPathLayer
 *
 * Returns the list of dash segment lengths.
 * 
 * Returns: (transfer container) (element-type guint): the list
 *
 * Since: 0.14
 */
GList *
gpx_viewer_path_layer_get_dash (GpxViewerPathLayer *layer)
{
  g_return_val_if_fail (GPX_VIEWER_IS_PATH_LAYER (layer), NULL);
  
  GpxViewerPathLayerPrivate *priv = layer->priv;
  GList *list = NULL;
  guint i;
  
  for (i = 0; i < priv->num_dashes; i++)
    list = g_list_append(list, GUINT_TO_POINTER((guint)(priv->dash)[i]));
  
  return list;
}

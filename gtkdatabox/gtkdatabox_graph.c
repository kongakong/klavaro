/* $Id: gtkdatabox_graph.c 4 2008-06-22 09:19:11Z rbock $ */
/* GtkDatabox - An extension to the gtk+ library
 * Copyright (C) 1998 - 2008  Dr. Roland Bock
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <gtkdatabox_graph.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE(GtkDataboxGraph, gtk_databox_graph,
	G_TYPE_OBJECT)

static void gtk_databox_graph_real_draw (GtkDataboxGraph * graph,
    GtkDatabox * draw);
static gint gtk_databox_graph_real_calculate_extrema (GtkDataboxGraph * graph,
    gfloat * min_x,
    gfloat * max_x,
    gfloat * min_y,
    gfloat * max_y);

/* IDs of properties */
enum
{
  GRAPH_COLOR = 1,
  GRAPH_SIZE,
  GRAPH_HIDE
};

/**
 * GtkDataboxGraphPrivate
 *
 * A private data structure used by the #GtkDataboxGraph. It shields all internal things
 * from developers who are just using the object.
 *
 **/
typedef struct _GtkDataboxGraphPrivate GtkDataboxGraphPrivate;

struct _GtkDataboxGraphPrivate
{
  GdkRGBA color;
  gint size;
  gboolean hide;
  GdkRGBA rgba;
};

static void
gtk_databox_graph_set_property (GObject * object,
                                guint property_id,
                                const GValue * value, GParamSpec * pspec)
{
  GtkDataboxGraph *graph = GTK_DATABOX_GRAPH (object);

  switch (property_id)
    {
    case GRAPH_COLOR:
    {
      gtk_databox_graph_set_color (graph,
                                   (GdkRGBA *)
                                   g_value_get_pointer (value));
    }
    break;
    case GRAPH_SIZE:
    {
      gtk_databox_graph_set_size (graph, g_value_get_int (value));
    }
    break;
    case GRAPH_HIDE:
    {
      gtk_databox_graph_set_hide (graph, g_value_get_boolean (value));
    }
    break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_databox_graph_get_property (GObject * object,
                                guint property_id,
                                GValue * value, GParamSpec * pspec)
{
  GtkDataboxGraph *graph = GTK_DATABOX_GRAPH (object);

  switch (property_id)
    {
    case GRAPH_COLOR:
    {
      g_value_set_pointer (value, gtk_databox_graph_get_color (graph));
    }
    break;
    case GRAPH_SIZE:
    {
      g_value_set_int (value, gtk_databox_graph_get_size (graph));
    }
    break;
    case GRAPH_HIDE:
    {
      g_value_set_boolean (value, gtk_databox_graph_get_hide (graph));
    }
    break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gtk_databox_graph_create_gc:
 * @graph: A #GtkDataboxGraph object
 * @box: A #GtkDatabox object
 *
 * Virtual function which creates a graphics context for the @graph.
 *
 * Typically called by derived graph objects when the graphics context is needed for the first time.
 *
 * Return value: The new graphics context.
 */
cairo_t*
gtk_databox_graph_create_gc (GtkDataboxGraph * graph,
                             GtkDatabox* box)
{
  return GTK_DATABOX_GRAPH_GET_CLASS (graph)->create_gc (graph, box);
}

static cairo_t *
gtk_databox_graph_real_create_gc (GtkDataboxGraph * graph,
                                  GtkDatabox* box)
{
  GtkDataboxGraphPrivate *priv = GTK_DATABOX_GRAPH_GET_PRIVATE(graph);
  cairo_t *cr;

  g_return_val_if_fail (GTK_DATABOX_IS_GRAPH (graph), NULL);

   cr = cairo_create (gtk_databox_get_backing_surface (box));
   gdk_cairo_set_source_rgba (cr, &priv->color);
   cairo_set_line_width (cr,  (priv->size > 1) ? priv->size : 1);

   return cr;
}

static void
gtk_databox_graph_class_init (GtkDataboxGraphClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *graph_param_spec;

  gobject_class->set_property = gtk_databox_graph_set_property;
  gobject_class->get_property = gtk_databox_graph_get_property;

  graph_param_spec = g_param_spec_pointer ("color",
                     "Graph color",
                     "Color of graph",
                     G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   GRAPH_COLOR, graph_param_spec);

  graph_param_spec = g_param_spec_int ("size", "Graph size", "Size of displayed items", G_MININT, G_MAXINT, 0,	/* default value */
                                       G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   GRAPH_SIZE, graph_param_spec);

  graph_param_spec = g_param_spec_boolean ("hide", "Graph hidden", "Determine if graph is hidden or not", FALSE,	/* default value */
                     G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   GRAPH_HIDE, graph_param_spec);

  klass->draw = gtk_databox_graph_real_draw;
  klass->calculate_extrema = gtk_databox_graph_real_calculate_extrema;
  klass->create_gc = gtk_databox_graph_real_create_gc;

  g_type_class_add_private (klass, sizeof (GtkDataboxGraphPrivate));
}

static void
gtk_databox_graph_init (GtkDataboxGraph *graph)
{
	graph = graph;
}

/**
 * gtk_databox_graph_draw:
 * @graph: A #GtkDataboxGraph object
 * @box: A #GtkDatabox object
 *
 * Virtual function which draws the #GtkDataboxGraph on the drawing area of the GtkDatabox object.
 *
 * Typically this function is called by #GtkDatabox objects.
 *
 */
void
gtk_databox_graph_draw (GtkDataboxGraph * graph, GtkDatabox* box)
{
  if (!GTK_DATABOX_GRAPH_GET_PRIVATE(graph)->hide)
    GTK_DATABOX_GRAPH_GET_CLASS (graph)->draw (graph, box);
}

/**
 * gtk_databox_graph_calculate_extrema:
 * @graph: A #GtkDataboxGraph object
 * @min_x: Will be filled with the lowest x value of the dataset
 * @max_x: Will be filled with the highest x value of the dataset
 * @min_y: Will be filled with the lowest y value of the dataset
 * @max_y: Will be filled with the highest y value of the dataset
 *
 * Virtual function which determines the minimum and maximum x and y values of the values of this
 * #GtkDataboxGraph object if applicable (there are graphs which do
 * not contain data).
 *
 * Return value: 0 on success,
 *          -1 if no data is available,
 *
 */
gint
gtk_databox_graph_calculate_extrema (GtkDataboxGraph * graph,
                                     gfloat * min_x, gfloat * max_x,
                                     gfloat * min_y, gfloat * max_y)
{
  return
    GTK_DATABOX_GRAPH_GET_CLASS (graph)->calculate_extrema (graph, min_x,
        max_x, min_y,
        max_y);
}

static void
gtk_databox_graph_real_draw (GtkDataboxGraph * graph,
                             GtkDatabox* box)
{
  g_return_if_fail (graph);
  g_return_if_fail (box);

  /* We have no data... */
  return;
}


static gint
gtk_databox_graph_real_calculate_extrema (GtkDataboxGraph * graph,
    gfloat * min_x, gfloat * max_x,
    gfloat * min_y, gfloat * max_y)
{
  g_return_val_if_fail (graph, -1);
  g_return_val_if_fail (min_x, -1);
  g_return_val_if_fail (max_x, -1);
  g_return_val_if_fail (min_y, -1);
  g_return_val_if_fail (max_y, -1);

  /* We have no data... */
  return -1;
}

/**
 * gtk_databox_graph_set_color:
 * @graph: A #GtkDataboxGraph object
 * @color: Color which is to be used by the graph object
 *
 * Sets the color which the #GtkDataboxGraph object is supposed to be using when drawing itself.
 *
 */
void
gtk_databox_graph_set_color (GtkDataboxGraph * graph, GdkRGBA * color)
{
  GtkDataboxGraphPrivate *priv = GTK_DATABOX_GRAPH_GET_PRIVATE(graph);

  g_return_if_fail (GTK_DATABOX_IS_GRAPH (graph));

  priv->color = *color;

  g_object_notify (G_OBJECT (graph), "color");
}

/**
 * gtk_databox_graph_get_color:
 * @graph: A #GtkDataboxGraph object
 *
 * Gets the current color of the graph elements (e.g. points).
 *
 * Return value: The color of the graph.
 *
 */
GdkRGBA *
gtk_databox_graph_get_color (GtkDataboxGraph * graph)
{
  return &GTK_DATABOX_GRAPH_GET_PRIVATE(graph)->color;
}

/**
 * gtk_databox_graph_set_size:
 * @graph: A #GtkDataboxGraph object
 * @size: Size of graph elements for the graph object
 *
 * Sets the size (e.g. line width) which the #GtkDataboxGraph object is supposed to be using when drawing itself.
 *
 */
void
gtk_databox_graph_set_size (GtkDataboxGraph * graph, gint size)
{
  g_return_if_fail (GTK_DATABOX_IS_GRAPH (graph));

  GTK_DATABOX_GRAPH_GET_PRIVATE(graph)->size = MAX (1, size);;

  g_object_notify (G_OBJECT (graph), "size");
}

/**
 * gtk_databox_graph_get_size:
 * @graph: A #GtkDataboxGraph object
 *
 * Gets the size of the graph elements (e.g. the line width).
 *
 * Return value: size of the graph elements
 *
 */
gint
gtk_databox_graph_get_size (GtkDataboxGraph * graph)
{
  g_return_val_if_fail (GTK_DATABOX_IS_GRAPH (graph), -1);

  return GTK_DATABOX_GRAPH_GET_PRIVATE(graph)->size;
}

/**
 * gtk_databox_graph_set_hide:
 * @graph: A #GtkDataboxGraph object
 * @hide: Declares whether should be hidden (true) or not (false).
 *
 * Hidden graphs are not shown, when the #GtkDatabox containing them is redrawn.
 *
 */
void
gtk_databox_graph_set_hide (GtkDataboxGraph * graph, gboolean hide)
{
  g_return_if_fail (GTK_DATABOX_IS_GRAPH (graph));

  GTK_DATABOX_GRAPH_GET_PRIVATE(graph)->hide = hide;

  g_object_notify (G_OBJECT (graph), "hide");
}

/**
 * gtk_databox_graph_get_hide:
 * @graph: A #GtkDataboxGraph object
 *
 * Gets the current "hide" status.
 *
 * Return value: Whether the graph is hidden (true) or not (false).
 *
 */
gboolean
gtk_databox_graph_get_hide (GtkDataboxGraph * graph)
{
  g_return_val_if_fail (GTK_DATABOX_IS_GRAPH (graph), -1);

  return GTK_DATABOX_GRAPH_GET_PRIVATE(graph)->hide;
}

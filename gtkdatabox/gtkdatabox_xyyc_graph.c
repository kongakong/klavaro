/* $Id: gtkdatabox_xyyc_graph.c 4 2008-06-22 09:19:11Z rbock $ */
/* GtkDatabox - An extension to the gtk+ library
 * Copyright (C) 1998 - 2008  Dr. Roland Bock
 * Copyright (C) 2012  Dr. Matt Flax <flatmax@>
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

#include <gtkdatabox_xyyc_graph.h>

G_DEFINE_TYPE(GtkDataboxXYYCGraph, gtk_databox_xyyc_graph,
	GTK_DATABOX_TYPE_GRAPH)

static gint gtk_databox_xyyc_graph_real_calculate_extrema (GtkDataboxGraph *
							  xyyc_graph,
							  gfloat * min_x,
							  gfloat * max_x,
							  gfloat * min_y,
							  gfloat * max_y);

/* IDs of properties */
enum
{
   PROP_X = 1,
   PROP_Y1,
   PROP_Y2,
   PROP_LEN,
   PROP_SIZE,
   PROP_XSTART,
   PROP_Y1START,
   PROP_Y2START,
   PROP_XSTRIDE,
   PROP_Y1STRIDE,
   PROP_Y2STRIDE,
   PROP_XTYPE,
   PROP_YTYPE
};

/**
 * GtkDataboxXYYCGraphPrivate
 *
 * A private data structure used by the #GtkDataboxXYYCGraph. It shields all internal things
 * from developers who are just using the object.
 *
 **/
typedef struct _GtkDataboxXYYCGraphPrivate GtkDataboxXYYCGraphPrivate;

struct _GtkDataboxXYYCGraphPrivate
{
   gfloat *X;
   gfloat *Y1;
   gfloat *Y2;
   guint len;
   guint maxlen;
   guint xstart;
   guint y1start;
   guint y2start;
   guint xstride;
   guint y1stride;
   guint y2stride;
   GType xtype;
   GType ytype;
};

static void
gtk_databox_xyyc_graph_set_X (GtkDataboxXYYCGraph * xyyc_graph, gfloat * X)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));
   g_return_if_fail (X);

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->X = X;

   g_object_notify (G_OBJECT (xyyc_graph), "X-Values");
}

static void
gtk_databox_xyyc_graph_set_Y1 (GtkDataboxXYYCGraph * xyyc_graph, gfloat * Y1)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));
   g_return_if_fail (Y1);

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->Y1 = Y1;

   g_object_notify (G_OBJECT (xyyc_graph), "Y1-Values");
}

static void
gtk_databox_xyyc_graph_set_Y2 (GtkDataboxXYYCGraph * xyyc_graph, gfloat * Y2)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));
   g_return_if_fail (Y2);

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->Y2 = Y2;

   g_object_notify (G_OBJECT (xyyc_graph), "Y2-Values");
}

static void
gtk_databox_xyyc_graph_set_length (GtkDataboxXYYCGraph * xyyc_graph, guint len)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));
   g_return_if_fail (len > 0);

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->len = len;

   g_object_notify (G_OBJECT (xyyc_graph), "length");
}

static void
gtk_databox_xyyc_graph_set_maxlen (GtkDataboxXYYCGraph * xyyc_graph, guint maxlen)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));
   g_return_if_fail (maxlen > 0);

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->maxlen = maxlen;

   g_object_notify (G_OBJECT (xyyc_graph), "maxlen");
}

static void
gtk_databox_xyyc_graph_set_xstart (GtkDataboxXYYCGraph * xyyc_graph, guint xstart)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->xstart = xstart;

   g_object_notify (G_OBJECT (xyyc_graph), "X-Values");
}

static void
gtk_databox_xyyc_graph_set_y1start (GtkDataboxXYYCGraph * xyyc_graph, guint y1start)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y1start = y1start;

   g_object_notify (G_OBJECT (xyyc_graph), "Y1-Values");
}

static void
gtk_databox_xyyc_graph_set_y2start (GtkDataboxXYYCGraph * xyyc_graph, guint y2start)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y2start = y2start;

   g_object_notify (G_OBJECT (xyyc_graph), "Y2-Values");
}

static void
gtk_databox_xyyc_graph_set_xstride (GtkDataboxXYYCGraph * xyyc_graph, guint xstride)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->xstride = xstride;

   g_object_notify (G_OBJECT (xyyc_graph), "X-Values");
}

static void
gtk_databox_xyyc_graph_set_y1stride (GtkDataboxXYYCGraph * xyyc_graph, guint y1stride)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y1stride = y1stride;

   g_object_notify (G_OBJECT (xyyc_graph), "Y1-Values");
}

static void
gtk_databox_xyyc_graph_set_y2stride (GtkDataboxXYYCGraph * xyyc_graph, guint y2stride)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y2stride = y2stride;

   g_object_notify (G_OBJECT (xyyc_graph), "Y2-Values");
}

static void
gtk_databox_xyyc_graph_set_xtype (GtkDataboxXYYCGraph * xyyc_graph, GType xtype)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->xtype = xtype;

   g_object_notify (G_OBJECT (xyyc_graph), "X-Values");
}

static void
gtk_databox_xyyc_graph_set_ytype (GtkDataboxXYYCGraph * xyyc_graph, GType ytype)
{
   g_return_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph));

   GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->ytype = ytype;

   g_object_notify (G_OBJECT (xyyc_graph), "Y1-Values");
   g_object_notify (G_OBJECT (xyyc_graph), "Y2-Values");
}

static void
gtk_databox_xyyc_graph_set_property (GObject * object,
				    guint property_id,
				    const GValue * value, GParamSpec * pspec)
{
   GtkDataboxXYYCGraph *xyyc_graph = GTK_DATABOX_XYYC_GRAPH (object);

   switch (property_id)
   {
   case PROP_X:
      gtk_databox_xyyc_graph_set_X (xyyc_graph, (gfloat *) g_value_get_pointer (value));
      break;
   case PROP_Y1:
      gtk_databox_xyyc_graph_set_Y1 (xyyc_graph, (gfloat *) g_value_get_pointer (value));
      break;
   case PROP_Y2:
      gtk_databox_xyyc_graph_set_Y2 (xyyc_graph, (gfloat *) g_value_get_pointer (value));
      break;
   case PROP_LEN:
      gtk_databox_xyyc_graph_set_length (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_SIZE:
      gtk_databox_xyyc_graph_set_maxlen (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_XSTART:
      gtk_databox_xyyc_graph_set_xstart (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_Y1START:
      gtk_databox_xyyc_graph_set_y1start (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_Y2START:
      gtk_databox_xyyc_graph_set_y2start (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_XSTRIDE:
      gtk_databox_xyyc_graph_set_xstride (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_Y1STRIDE:
      gtk_databox_xyyc_graph_set_y1stride (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_Y2STRIDE:
      gtk_databox_xyyc_graph_set_y2stride (xyyc_graph, g_value_get_int (value));
      break;
   case PROP_XTYPE:
      gtk_databox_xyyc_graph_set_xtype (xyyc_graph, g_value_get_gtype (value));
      break;
   case PROP_YTYPE:
      gtk_databox_xyyc_graph_set_ytype (xyyc_graph, g_value_get_gtype (value));
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}

/**
 * gtk_databox_xyyc_graph_get_X:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the X values of the @xzc_graph.
 *
 * Return value: Pointer to X values
 */
gfloat *
gtk_databox_xyyc_graph_get_X (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), NULL);

   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->X;
}

/**
 * gtk_databox_xyyc_graph_get_Y1:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the Y1 values of the @xzc_graph.
 *
 * Return value: Pointer to Y1 values
 */
gfloat *
gtk_databox_xyyc_graph_get_Y1 (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), NULL);

   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->Y1;
}

/**
 * gtk_databox_xyyc_graph_get_Y2:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the Y2 values of the @xzc_graph.
 *
 * Return value: Pointer to Y2 values
 */
gfloat *
gtk_databox_xyyc_graph_get_Y2 (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), NULL);

   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->Y2;
}

/**
 * gtk_databox_xyyc_graph_get_length:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the length of the X and Y values arrays.
 *
 * Return value: Length of X/Y arrays.
 */
guint
gtk_databox_xyyc_graph_get_length (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);

   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->len;
}

/**
 * gtk_databox_xyyc_graph_get_maxlen:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the maxlen of the X and Y values arrays.
 *
 * Return value: Size of X/Y arrays (size of the allocated storage).
 */
guint
gtk_databox_xyyc_graph_get_maxlen (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);

   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->maxlen;
}

/**
 * gtk_databox_xyyc_graph_get_xstart:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the start offset of the X values array.  This is the element in the array pointed to by X that will be the first element plotted.
 * If X is a pointer to a gfloat array, and xstart is 5, then x[5] will be the first data element.  If Xstride is 1, then x[6] will be the
 * second element.  x[5 + len - 1] will be last element.
 * Usually, xstart will be 0.  It can be nonzero to allow for interleaved X/Y samples, or if the data is stored as a matrix, then X can point
 * to the start of the matrix, xstart can be the column number, and xstride the number of columns.
 *
 * Return value: The xstart value.
 */
guint
gtk_databox_xyyc_graph_get_xstart (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->xstart;
}

/**
 * gtk_databox_xyyc_graph_get_y1start:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the start offset of the Y1 values array.  This is the element in the array pointed to by Y that will be the first element plotted.
 * If Y1 is a pointer to a gfloat array, and y1start is 5, then y1[5] will be the first data element.  If y1stride is 1, then y1[6] will be the
 * second element.  y1[5 + len - 1] will be last element.
 * Usually, y1start will be 0.  It can be nonzero to allow for interleaved X/Y1/Y2 samples, or if the data is stored as a matrix, then Y1 can point
 * to the start of the matrix, y1start can be the column number, and y1stride the number of columns.
 *
 * Return value: The y1start value.
 */
guint
gtk_databox_xyyc_graph_get_y1start (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y1start;
}

/**
 * gtk_databox_xyyc_graph_get_y2start:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the start offset of the Y2 values array.  This is the element in the array pointed to by Y that will be the first element plotted.
 * If Y2 is a pointer to a gfloat array, and y2start is 5, then y2[5] will be the first data element.  If y2stride is 1, then y2[6] will be the
 * second element.  y2[5 + len - 1] will be last element.
 * Usually, y2start will be 0.  It can be nonzero to allow for interleaved X/Y1/Y2 samples, or if the data is stored as a matrix, then Y2 can point
 * to the start of the matrix, y2start can be the column number, and y2stride the number of columns.
 *
 * Return value: The y2start value.
 */
guint
gtk_databox_xyyc_graph_get_y2start (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y2start;
}

/**
 * gtk_databox_xyyc_graph_get_xstride:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the stride offset of the X values array.  This is the element in the array pointed to by X that will be the first element plotted.
 * If X is a pointer to a gfloat array, and xstart is 5, then x[5] will be the first data element.  If Xstride is 1, then x[6] will be the
 * second element.  x[5 + len - 1] will be last element.
 * Usually, xstride will be 1.  It can be nonzero to allow for interleaved X/Y samples, or if the data is stored as a matrix, then X can point
 * to the start of the matrix, xstart can be the column number, and xstride the number of columns.
 *
 * Return value: The xstride value.
 */
guint
gtk_databox_xyyc_graph_get_xstride (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->xstride;
}

/**
 * gtk_databox_xyyc_graph_get_y1stride:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the stride offset of the Y1 values array.  This is the element in the array pointed to by Y1 that will be the first element plotted.
 * If Y1 is a pointer to a gfloat array, and y1start is 5, then y1[5] will be the first data element.  If y1stride is 1, then y1[6] will be the
 * second element.  y1[5 + len - 1] will be last element.
 * Usually, y1stride will be 1.  It can be nonzero to allow for interleaved X/Y1/Y2 samples, or if the data is stored as a matrix, then Y1 can point
 * to the start of the matrix, y1start can be the column number, and y1stride the number of columns.
 *
 * Return value: The y1stride value.
 */
guint
gtk_databox_xyyc_graph_get_y1stride (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y1stride;
}

/**
 * gtk_databox_xyyc_graph_get_y2stride:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the stride offset of the Y2 values array.  This is the element in the array pointed to by Y2 that will be the first element plotted.
 * If Y2 is a pointer to a gfloat array, and y2start is 5, then y2[5] will be the first data element.  If y2stride is 1, then y2[6] will be the
 * second element.  y2[5 + len - 1] will be last element.
 * Usually, y2stride will be 1.  It can be nonzero to allow for interleaved X/Y1/Y2 samples, or if the data is stored as a matrix, then Y2 can point
 * to the start of the matrix, y2start can be the column number, and y2stride the number of columns.
 *
 * Return value: The y2stride value.
 */
guint
gtk_databox_xyyc_graph_get_y2stride (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->y2stride;
}

/**
 * gtk_databox_xyyc_graph_get_xtype:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the GType of the X array elements.  This may be G_TYPE_FLOAT, G_TYPE_DOUBLE, or similar.
 *
 * Return value: A GType, usually this is G_TYPE_FLOAT.
 */
GType
gtk_databox_xyyc_graph_get_xtype (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->xtype;
}

/**
 * gtk_databox_xyyc_graph_get_ytype:
 * @xyyc_graph: A #GtkDataboxXYYCGraph object
 *
 * Gets the the GType of the Y1/Y2 array elements.  This may be G_TYPE_FLOAT, G_TYPE_DOUBLE, or similar.
 *
 * Return value: A GType, usually this is G_TYPE_FLOAT.
 */
GType
gtk_databox_xyyc_graph_get_ytype (GtkDataboxXYYCGraph * xyyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (xyyc_graph), 0);
   return GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE(xyyc_graph)->ytype;
}

static void
gtk_databox_xyyc_graph_get_property (GObject * object,
				    guint property_id,
				    GValue * value, GParamSpec * pspec)
{
   GtkDataboxXYYCGraph *xyyc_graph = GTK_DATABOX_XYYC_GRAPH (object);

   switch (property_id)
   {
   case PROP_X:
      g_value_set_pointer (value, gtk_databox_xyyc_graph_get_X (xyyc_graph));
      break;
   case PROP_Y1:
      g_value_set_pointer (value, gtk_databox_xyyc_graph_get_Y1 (xyyc_graph));
      break;
   case PROP_Y2:
      g_value_set_pointer (value, gtk_databox_xyyc_graph_get_Y2 (xyyc_graph));
      break;
   case PROP_LEN:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_length (xyyc_graph));
      break;
   case PROP_SIZE:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_maxlen (xyyc_graph));
      break;
   case PROP_XSTART:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_xstart (xyyc_graph));
      break;
   case PROP_Y1START:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_y1start (xyyc_graph));
      break;
   case PROP_Y2START:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_y2start (xyyc_graph));
      break;
   case PROP_XSTRIDE:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_xstride (xyyc_graph));
      break;
   case PROP_Y1STRIDE:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_y1stride (xyyc_graph));
      break;
   case PROP_Y2STRIDE:
      g_value_set_int (value, gtk_databox_xyyc_graph_get_y2stride (xyyc_graph));
      break;
   case PROP_XTYPE:
      g_value_set_gtype (value, gtk_databox_xyyc_graph_get_xtype (xyyc_graph));
      break;
   case PROP_YTYPE:
      g_value_set_gtype (value, gtk_databox_xyyc_graph_get_ytype (xyyc_graph));
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}

static void
gtk_databox_xyyc_graph_class_init (GtkDataboxXYYCGraphClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   GtkDataboxGraphClass *graph_class = GTK_DATABOX_GRAPH_CLASS (klass);
   GParamSpec *xyyc_graph_param_spec;

   gobject_class->set_property = gtk_databox_xyyc_graph_set_property;
   gobject_class->get_property = gtk_databox_xyyc_graph_get_property;

   xyyc_graph_param_spec = g_param_spec_pointer ("X-Values",
						"X coordinates",
						"X values of data",
						G_PARAM_CONSTRUCT_ONLY |
						G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_X, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_pointer ("Y1-Values",
						"Y1 coordinates",
						"Y1 values of data",
						G_PARAM_CONSTRUCT_ONLY |
						G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_Y1, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_pointer ("Y2-Values",
						"Y2 coordinates",
						"Y2 values of data",
						G_PARAM_CONSTRUCT_ONLY |
						G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_Y2, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("length", "length of X, Y1 and Y2", "number of data points", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_LEN, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("maxlen", "maxlen of X and Y", "maximal number of data points", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_SIZE, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("xstart", "array index of first X", "array index of first X", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_XSTART, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("y1start", "array index of first Y1", "array index of first Y1", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_Y1START, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("y2start", "array index of first Y2", "array index of first Y2", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_Y2START, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("xstride", "stride of X values", "stride of X values", G_MININT, G_MAXINT, 1,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_XSTRIDE, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("y1stride", "stride of Y1 values", "stride of Y1 values", G_MININT, G_MAXINT, 1,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_Y1STRIDE, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_int ("y2stride", "stride of Y2 values", "stride of Y2 values", G_MININT, G_MAXINT, 1,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_Y2STRIDE, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_gtype ("xtype", "GType of X elements", "GType of X elements", G_TYPE_NONE,
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_XTYPE, xyyc_graph_param_spec);

   xyyc_graph_param_spec = g_param_spec_gtype ("ytype", "GType of Y1/Y2 elements", "GType of Y1/Y2 elements", G_TYPE_NONE,
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_YTYPE, xyyc_graph_param_spec);

   graph_class->calculate_extrema =
      gtk_databox_xyyc_graph_real_calculate_extrema;

   g_type_class_add_private (klass, sizeof (GtkDataboxXYYCGraphPrivate));
}

static void
gtk_databox_xyyc_graph_init (GtkDataboxXYYCGraph * xyyc_graph)
{
	xyyc_graph = xyyc_graph;
}

static gint
gtk_databox_xyyc_graph_real_calculate_extrema (GtkDataboxGraph * graph,
					      gfloat * min_x, gfloat * max_x,
					      gfloat * min_y, gfloat * max_y)
{
   GtkDataboxXYYCGraphPrivate *priv = GTK_DATABOX_XYYC_GRAPH_GET_PRIVATE (graph);
   guint i, indx, len, maxlen, start, stride;
   void *values;
   GType vtype;
   gfloat fval = 0.0, minval = 0.0, maxval = 0.0;

   g_return_val_if_fail (GTK_DATABOX_IS_XYYC_GRAPH (graph), -1);
   g_return_val_if_fail (min_x, -1);
   g_return_val_if_fail (max_x, -1);
   g_return_val_if_fail (min_y, -1);
   g_return_val_if_fail (max_y, -1);
   g_return_val_if_fail (priv->len, -1);

   len = priv->len;
   maxlen = priv->maxlen;
   values = priv->X;
   vtype = priv->xtype;
   start = priv->xstart;
   stride = priv->xstride;

   indx = start * stride;
   i = 0;
   do {
		if (vtype == G_TYPE_FLOAT)
			fval = ((gfloat *)values)[indx];
		else if (vtype == G_TYPE_DOUBLE)
			fval = ((gdouble *)values)[indx];
		else if (vtype == G_TYPE_INT)
			fval = ((gint *)values)[indx];
		else if (vtype == G_TYPE_UINT)
			fval = ((guint *)values)[indx];
		else if (vtype == G_TYPE_LONG)
			fval = ((glong *)values)[indx];
		else if (vtype == G_TYPE_ULONG)
			fval = ((gulong *)values)[indx];
		else if (vtype == G_TYPE_INT64)
			fval = ((gint64 *)values)[indx];
		else if (vtype == G_TYPE_UINT64)
			fval = ((guint64 *)values)[indx];
		else if (vtype == G_TYPE_CHAR)
			fval = ((gchar *)values)[indx];
		else if (vtype == G_TYPE_UCHAR)
			fval = ((guchar *)values)[indx];

		if (i==0)
		{
			minval = maxval = fval;
		}
		else
		{
			if (fval < minval) minval = fval;
			if (fval > maxval) maxval = fval;
		}

		/* handle the wrap-around (ring buffer) issue using modulus.  for efficiency, don't do this for non-wraparound cases. */
		/* note this allows multiple wrap-arounds.  One could hold a single cycle of a sine wave, and plot a continuous wave */
		/* This can be optimized using pointers later */
		if (i + start > maxlen)
			indx = ((i + start) % maxlen) * stride;
		else
			indx += stride;
   } while (++i < len);

   *min_x = minval;
   *max_x = maxval;

   values = priv->Y1;
   vtype = priv->ytype;
   start = priv->y1start;
   stride = priv->y1stride;

   indx = start * stride;
   i = 0;
   do {
		if (vtype == G_TYPE_FLOAT)
			fval = ((gfloat *)values)[indx];
		else if (vtype == G_TYPE_DOUBLE)
			fval = ((gdouble *)values)[indx];
		else if (vtype == G_TYPE_INT)
			fval = ((gint *)values)[indx];
		else if (vtype == G_TYPE_UINT)
			fval = ((guint *)values)[indx];
		else if (vtype == G_TYPE_LONG)
			fval = ((glong *)values)[indx];
		else if (vtype == G_TYPE_ULONG)
			fval = ((gulong *)values)[indx];
		else if (vtype == G_TYPE_INT64)
			fval = ((gint64 *)values)[indx];
		else if (vtype == G_TYPE_UINT64)
			fval = ((guint64 *)values)[indx];
		else if (vtype == G_TYPE_CHAR)
			fval = ((gchar *)values)[indx];
		else if (vtype == G_TYPE_UCHAR)
			fval = ((guchar *)values)[indx];

		if (i==0) /* yes putting this check inside the loop is inefficient, but it makes the code simpler */
		{
			minval = maxval = fval;
		}
		else
		{
			if (fval < minval) minval = fval;
			if (fval > maxval) maxval = fval;
		}

		/* handle the wrap-around (ring buffer) issue using modulus.  for efficiency, don't do this for non-wraparound cases. */
		/* note this allows multiple wrap-arounds.  One could hold a single cycle of a sine wave, and plot a continuous wave */
		/* This can be optimized using pointers later */
		if (i + start > maxlen)
			indx = ((i + start) % maxlen) * stride;
		else
			indx += stride;
   } while (++i < len);

   values = priv->Y2;
   start = priv->y2start;
   stride = priv->y2stride;

   indx = start * stride;
   i = 0;
   do {
		if (vtype == G_TYPE_FLOAT)
			fval = ((gfloat *)values)[indx];
		else if (vtype == G_TYPE_DOUBLE)
			fval = ((gdouble *)values)[indx];
		else if (vtype == G_TYPE_INT)
			fval = ((gint *)values)[indx];
		else if (vtype == G_TYPE_UINT)
			fval = ((guint *)values)[indx];
		else if (vtype == G_TYPE_LONG)
			fval = ((glong *)values)[indx];
		else if (vtype == G_TYPE_ULONG)
			fval = ((gulong *)values)[indx];
		else if (vtype == G_TYPE_INT64)
			fval = ((gint64 *)values)[indx];
		else if (vtype == G_TYPE_UINT64)
			fval = ((guint64 *)values)[indx];
		else if (vtype == G_TYPE_CHAR)
			fval = ((gchar *)values)[indx];
		else if (vtype == G_TYPE_UCHAR)
			fval = ((guchar *)values)[indx];

		/* Note that this is different from where we checked Y1 */
		if (fval < minval) minval = fval;
		if (fval > maxval) maxval = fval;

		/* handle the wrap-around (ring buffer) issue using modulus.  for efficiency, don't do this for non-wraparound cases. */
		/* note this allows multiple wrap-arounds.  One could hold a single cycle of a sine wave, and plot a continuous wave */
		/* This can be optimized using pointers later */
		if (i + start > maxlen)
			indx = ((i + start) % maxlen) * stride;
		else
			indx += stride;
   } while (++i < len);

   *min_y = minval;
   *max_y = maxval;

   return 0;
}

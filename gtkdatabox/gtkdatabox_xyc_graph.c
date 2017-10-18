/* $Id: gtkdatabox_xyc_graph.c 4 2008-06-22 09:19:11Z rbock $ */
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

#include <gtkdatabox_xyc_graph.h>

G_DEFINE_TYPE(GtkDataboxXYCGraph, gtk_databox_xyc_graph,
	GTK_DATABOX_TYPE_GRAPH)

static gint gtk_databox_xyc_graph_real_calculate_extrema (GtkDataboxGraph *
							  xyc_graph,
							  gfloat * min_x,
							  gfloat * max_x,
							  gfloat * min_y,
							  gfloat * max_y);

/* IDs of properties */
enum
{
   PROP_X = 1,
   PROP_Y,
   PROP_LEN,
   PROP_MAXLEN,
   PROP_XSTART,
   PROP_YSTART,
   PROP_XSTRIDE,
   PROP_YSTRIDE,
   PROP_XTYPE,
   PROP_YTYPE
};

/**
 * GtkDataboxXYCGraphPrivate
 *
 * A private data structure used by the #GtkDataboxXYCGraph. It shields all internal things
 * from developers who are just using the object.
 *
 **/
typedef struct _GtkDataboxXYCGraphPrivate GtkDataboxXYCGraphPrivate;

struct _GtkDataboxXYCGraphPrivate
{
   gfloat *X;
   gfloat *Y;
   guint len;
   guint maxlen;
   guint xstart;
   guint ystart;
   guint xstride;
   guint ystride;
   GType xtype;
   GType ytype;
};

/*
static gpointer parent_class = NULL;
*/

void
gtk_databox_xyc_graph_set_X_Y_length(GtkDataboxXYCGraph * xyc_graph, gfloat * X, gfloat * Y, guint len)
{
   GtkDataboxXYCGraphPrivate *priv = GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph);
   priv->Y = Y;
   priv->X = X;
   priv->len = len;
}

static void
gtk_databox_xyc_graph_set_X (GtkDataboxXYCGraph * xyc_graph, gfloat * X)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));
   g_return_if_fail (X);

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->X = X;

   g_object_notify (G_OBJECT (xyc_graph), "X-Values");
}

static void
gtk_databox_xyc_graph_set_Y (GtkDataboxXYCGraph * xyc_graph, gfloat * Y)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));
   g_return_if_fail (Y);

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->Y = Y;

   g_object_notify (G_OBJECT (xyc_graph), "Y-Values");
}

static void
gtk_databox_xyc_graph_set_length (GtkDataboxXYCGraph * xyc_graph, guint len)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));
   g_return_if_fail (len > 0);

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->len = len;

   g_object_notify (G_OBJECT (xyc_graph), "length");
}

static void
gtk_databox_xyc_graph_set_maxlen (GtkDataboxXYCGraph * xyc_graph, guint maxlen)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));
   g_return_if_fail (maxlen > 0);

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->maxlen = maxlen;

   g_object_notify (G_OBJECT (xyc_graph), "maxlen");
}

static void
gtk_databox_xyc_graph_set_xstart (GtkDataboxXYCGraph * xyc_graph, guint xstart)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->xstart = xstart;

   g_object_notify (G_OBJECT (xyc_graph), "X-Values");
}

static void
gtk_databox_xyc_graph_set_ystart (GtkDataboxXYCGraph * xyc_graph, guint ystart)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->ystart = ystart;

   g_object_notify (G_OBJECT (xyc_graph), "Y-Values");
}

static void
gtk_databox_xyc_graph_set_xstride (GtkDataboxXYCGraph * xyc_graph, guint xstride)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->xstride = xstride;

   g_object_notify (G_OBJECT (xyc_graph), "X-Values");
}

static void
gtk_databox_xyc_graph_set_ystride (GtkDataboxXYCGraph * xyc_graph, guint ystride)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->ystride = ystride;

   g_object_notify (G_OBJECT (xyc_graph), "Y-Values");
}

static void
gtk_databox_xyc_graph_set_xtype (GtkDataboxXYCGraph * xyc_graph, GType xtype)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->xtype = xtype;

   g_object_notify (G_OBJECT (xyc_graph), "X-Values");
}

static void
gtk_databox_xyc_graph_set_ytype (GtkDataboxXYCGraph * xyc_graph, GType ytype)
{
   g_return_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph));

   GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->ytype = ytype;

   g_object_notify (G_OBJECT (xyc_graph), "Y-Values");
}

static void
gtk_databox_xyc_graph_set_property (GObject * object,
				    guint property_id,
				    const GValue * value, GParamSpec * pspec)
{
   GtkDataboxXYCGraph *xyc_graph = GTK_DATABOX_XYC_GRAPH (object);

   switch (property_id)
   {
   case PROP_X:
      gtk_databox_xyc_graph_set_X (xyc_graph, (gfloat *) g_value_get_pointer (value));
      break;
   case PROP_Y:
      gtk_databox_xyc_graph_set_Y (xyc_graph, (gfloat *) g_value_get_pointer (value));
      break;
   case PROP_LEN:
      gtk_databox_xyc_graph_set_length (xyc_graph, g_value_get_int (value));
      break;
   case PROP_MAXLEN:
      gtk_databox_xyc_graph_set_maxlen (xyc_graph, g_value_get_int (value));
      break;
   case PROP_XSTART:
      gtk_databox_xyc_graph_set_xstart (xyc_graph, g_value_get_int (value));
      break;
   case PROP_YSTART:
      gtk_databox_xyc_graph_set_ystart (xyc_graph, g_value_get_int (value));
      break;
   case PROP_XSTRIDE:
      gtk_databox_xyc_graph_set_xstride (xyc_graph, g_value_get_int (value));
      break;
   case PROP_YSTRIDE:
      gtk_databox_xyc_graph_set_ystride (xyc_graph, g_value_get_int (value));
      break;
   case PROP_XTYPE:
      gtk_databox_xyc_graph_set_xtype (xyc_graph, g_value_get_gtype (value));
      break;
   case PROP_YTYPE:
      gtk_databox_xyc_graph_set_ytype (xyc_graph, g_value_get_gtype (value));
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}

/**
 * gtk_databox_xyc_graph_get_X:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the X values of the @xzc_graph.
 *
 * Return value: Pointer to X values
 */
gfloat *
gtk_databox_xyc_graph_get_X (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), NULL);

   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->X;
}

/**
 * gtk_databox_xyc_graph_get_Y:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the Y values of the @xzc_graph.
 *
 * Return value: Pointer to Y values
 */
gfloat *
gtk_databox_xyc_graph_get_Y (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), NULL);

   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->Y;
}

/**
 * gtk_databox_xyc_graph_get_length:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the the length of the X and Y values arrays.
 *
 * Return value: Length of X/Y arrays.
 */
guint
gtk_databox_xyc_graph_get_length (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->len;
}

/**
 * gtk_databox_xyc_graph_get_maxlen:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the the maxlen of the X and Y values arrays.
 *
 * Return value: Size of X/Y arrays.
 */
guint
gtk_databox_xyc_graph_get_maxlen (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->maxlen;
}

/**
 * gtk_databox_xyc_graph_get_xstart:
 * @xyc_graph: A #GtkDataboxXYCGraph object
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
gtk_databox_xyc_graph_get_xstart (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->xstart;
}

/**
 * gtk_databox_xyc_graph_get_ystart:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the the start offset of the Y values array.  This is the element in the array pointed to by Y that will be the first element plotted.
 * If Y is a pointer to a gfloat array, and ystart is 5, then y[5] will be the first data element.  If Ystride is 1, then y[6] will be the
 * second element.  y[5 + len - 1] will be last element.
 * Usually, ystart will be 0.  It can be nonzero to allow for interleaved X/Y samples, or if the data is stored as a matrix, then Y can point
 * to the start of the matrix, ystart can be the column number, and ystride the number of columns.
 *
 * Return value: The ystart value.
 */
guint
gtk_databox_xyc_graph_get_ystart (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->ystart;
}

/**
 * gtk_databox_xyc_graph_get_xstride:
 * @xyc_graph: A #GtkDataboxXYCGraph object
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
gtk_databox_xyc_graph_get_xstride (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->xstride;
}

/**
 * gtk_databox_xyc_graph_get_ystride:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the the stride offset of the Y values array.  This is the element in the array pointed to by Y that will be the first element plotted.
 * If Y is a pointer to a gfloat array, and ystart is 5, then y[5] will be the first data element.  If Ystride is 1, then y[6] will be the
 * second element.  y[5 + len - 1] will be last element.
 * Usually, ystride will be 1.  It can be nonzero to allow for interleaved X/Y samples, or if the data is stored as a matrix, then Y can point
 * to the start of the matrix, ystart can be the column number, and ystride the number of columns.
 *
 * Return value: The ystride value.
 */
guint
gtk_databox_xyc_graph_get_ystride (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->ystride;
}

/**
 * gtk_databox_xyc_graph_get_xtype:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the the GType of the X array elements.  This may be G_TYPE_FLOAT, G_TYPE_DOUBLE, or similar.
 *
 * Return value: A GType, usually this is G_TYPE_FLOAT.
 */
GType
gtk_databox_xyc_graph_get_xtype (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->xtype;
}

/**
 * gtk_databox_xyc_graph_get_ytype:
 * @xyc_graph: A #GtkDataboxXYCGraph object
 *
 * Gets the the GType of the Y array elements.  This may be G_TYPE_FLOAT, G_TYPE_DOUBLE, or similar.
 *
 * Return value: A GType, usually this is G_TYPE_FLOAT.
 */
GType
gtk_databox_xyc_graph_get_ytype (GtkDataboxXYCGraph * xyc_graph)
{
   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (xyc_graph), 0);
   return GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph)->ytype;
}

static void
gtk_databox_xyc_graph_get_property (GObject * object,
				    guint property_id,
				    GValue * value, GParamSpec * pspec)
{
   GtkDataboxXYCGraph *xyc_graph = GTK_DATABOX_XYC_GRAPH (object);

   switch (property_id)
   {
   case PROP_X:
      g_value_set_pointer (value, gtk_databox_xyc_graph_get_X (xyc_graph));
      break;
   case PROP_Y:
      g_value_set_pointer (value, gtk_databox_xyc_graph_get_Y (xyc_graph));
      break;
   case PROP_LEN:
      g_value_set_int (value, gtk_databox_xyc_graph_get_length (xyc_graph));
      break;
   case PROP_MAXLEN:
      g_value_set_int (value, gtk_databox_xyc_graph_get_maxlen (xyc_graph));
      break;
   case PROP_XSTART:
      g_value_set_int (value, gtk_databox_xyc_graph_get_xstart (xyc_graph));
      break;
   case PROP_YSTART:
      g_value_set_int (value, gtk_databox_xyc_graph_get_ystart (xyc_graph));
      break;
   case PROP_XSTRIDE:
      g_value_set_int (value, gtk_databox_xyc_graph_get_xstride (xyc_graph));
      break;
   case PROP_YSTRIDE:
      g_value_set_int (value, gtk_databox_xyc_graph_get_ystride (xyc_graph));
      break;
   case PROP_XTYPE:
      g_value_set_gtype (value, gtk_databox_xyc_graph_get_xtype (xyc_graph));
      break;
   case PROP_YTYPE:
      g_value_set_gtype (value, gtk_databox_xyc_graph_get_ytype (xyc_graph));
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}

static void
gtk_databox_xyc_graph_class_init (GtkDataboxXYCGraphClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   GtkDataboxGraphClass *graph_class = GTK_DATABOX_GRAPH_CLASS (klass);
   GParamSpec *xyc_graph_param_spec;

   gobject_class->set_property = gtk_databox_xyc_graph_set_property;
   gobject_class->get_property = gtk_databox_xyc_graph_get_property;

   xyc_graph_param_spec = g_param_spec_pointer ("X-Values",
						"X coordinates",
						"X values of data",
						G_PARAM_CONSTRUCT_ONLY |
						G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_X, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_pointer ("Y-Values",
						"Y coordinates",
						"Y values of data",
						G_PARAM_CONSTRUCT_ONLY |
						G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_Y, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_int ("length", "length of X and Y", "number of data points", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_LEN, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_int ("maxlen", "maxlen of X and Y", "maximal number of data points", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_MAXLEN, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_int ("xstart", "array index of first X", "array index of first X", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_XSTART, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_int ("ystart", "array index of first Y", "array index of first Y", G_MININT, G_MAXINT, 0,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_YSTART, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_int ("xstride", "stride of X values", "stride of X values", G_MININT, G_MAXINT, 1,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_XSTRIDE, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_int ("ystride", "stride of Y values", "stride of Y values", G_MININT, G_MAXINT, 1,	/* default value */
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_YSTRIDE, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_gtype ("xtype", "GType of X elements", "GType of X elements", G_TYPE_NONE,
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_XTYPE, xyc_graph_param_spec);

   xyc_graph_param_spec = g_param_spec_gtype ("ytype", "GType of Y elements", "GType of Y elements", G_TYPE_NONE,
					    G_PARAM_CONSTRUCT_ONLY |
					    G_PARAM_READWRITE);
   g_object_class_install_property (gobject_class,
				    PROP_YTYPE, xyc_graph_param_spec);

   graph_class->calculate_extrema =
      gtk_databox_xyc_graph_real_calculate_extrema;

   g_type_class_add_private (klass, sizeof (GtkDataboxXYCGraphPrivate));
}

static void
gtk_databox_xyc_graph_init (GtkDataboxXYCGraph *xyc_graph)
{
	xyc_graph = xyc_graph;
}

static gint
gtk_databox_xyc_graph_real_calculate_extrema (GtkDataboxGraph * graph,
					      gfloat * min_x, gfloat * max_x,
					      gfloat * min_y, gfloat * max_y)
{
   GtkDataboxXYCGraph *xyc_graph = GTK_DATABOX_XYC_GRAPH (graph);
   GtkDataboxXYCGraphPrivate *priv = GTK_DATABOX_XYC_GRAPH_GET_PRIVATE(xyc_graph);
   guint i, indx, len, maxlen, start, stride;
   void *values;
   GType vtype;
   gfloat fval = 0.0, minval = 0.0, maxval = 0.0;

   g_return_val_if_fail (GTK_DATABOX_IS_XYC_GRAPH (graph), -1);
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

   values = priv->Y;
   vtype = priv->ytype;
   start = priv->ystart;
   stride = priv->ystride;

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

   *min_y = minval;
   *max_y = maxval;

   return 0;
}

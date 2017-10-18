/* $Id: gtkdatabox_offset_bars.c 4 2008-06-22 09:19:11Z rbock $ */
/* GtkDatabox - An extension to the gtk+ library
 * Copyright (C) 2011 - 2012  Dr. Matt Flax <flatmax@>
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

#include <gtkdatabox_offset_bars.h>

G_DEFINE_TYPE(GtkDataboxOffsetBars, gtk_databox_offset_bars,
	GTK_DATABOX_TYPE_XYYC_GRAPH)

static void gtk_databox_offset_bars_real_draw (GtkDataboxGraph * bars,
					GtkDatabox* box);

/**
 * GtkDataboxOffsetBarsPrivate
 *
 * A private data structure used by the #GtkDataboxOffsetBars. It shields all internal things
 * from developers who are just using the object.
 *
 **/
typedef struct _GtkDataboxOffsetBarsPrivate GtkDataboxOffsetBarsPrivate;

struct _GtkDataboxOffsetBarsPrivate
{
   gint16 *xpixels;
   gint16 *y1pixels;
   gint16 *y2pixels;
   guint pixelsalloc;
};

static void
bars_finalize (GObject * object)
{
   /*
   GtkDataboxOffsetBars *bars = GTK_DATABOX_OFFSET_BARS (object);
   */

   g_free (GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(object)->xpixels);
   g_free (GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(object)->y1pixels);
   g_free (GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(object)->y2pixels);

   /* Chain up to the parent class */
   G_OBJECT_CLASS (gtk_databox_offset_bars_parent_class)->finalize (object);
}

static void
gtk_databox_offset_bars_class_init (GtkDataboxOffsetBarsClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   GtkDataboxGraphClass *graph_class = GTK_DATABOX_GRAPH_CLASS (klass);

   gobject_class->finalize = bars_finalize;

   graph_class->draw = gtk_databox_offset_bars_real_draw;

   g_type_class_add_private (klass, sizeof (GtkDataboxOffsetBarsPrivate));
}

static void
gtk_databox_offset_bars_complete (GtkDataboxOffsetBars * bars)
{
   GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(bars)->xpixels = NULL;
   GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(bars)->y1pixels = NULL;
   GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(bars)->y2pixels = NULL;
   GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(bars)->pixelsalloc = 0;
}

static void
gtk_databox_offset_bars_init (GtkDataboxOffsetBars *bars)
{
   g_signal_connect (bars, "notify::length",
		     G_CALLBACK (gtk_databox_offset_bars_complete), NULL);
}

/**
 * gtk_databox_offset_bars_new:
 * @len: length of @X, @Y1  and @Y2
 * @X: array of horizontal position values of markers
 * @Y1: array of starting vertical position values of markers
 * @Y2: array of ending vertical position values of markers
 * @color: color of the markers
 * @size: marker size or line width (depending on the @type)
 *
 * Creates a new #GtkDataboxOffsetBars object which can be added to a #GtkDatabox widget
 *
 * Return value: A new #GtkDataboxOffsetBars object
 **/
GtkDataboxGraph *
gtk_databox_offset_bars_new (guint len, gfloat * X, gfloat * Y1, gfloat * Y2,
		      GdkRGBA * color, guint size)
{
   GtkDataboxOffsetBars *bars;
   g_return_val_if_fail (X, NULL);
   g_return_val_if_fail (Y1, NULL);
   g_return_val_if_fail (Y2, NULL);
   g_return_val_if_fail ((len > 0), NULL);

   bars = g_object_new (GTK_DATABOX_TYPE_OFFSET_BARS,
			"X-Values", X,
			"Y1-Values", Y1,
			"Y2-Values", Y2,
			"xstart", 0,
			"y1start", 0,
			"y2start", 0,
			"xstride", 1,
			"y1stride", 1,
			"y2stride", 1,
			"xtype", G_TYPE_FLOAT,
			"ytype", G_TYPE_FLOAT,
			"length", len,
			"maxlen", len,
			"color", color, "size", size, NULL);

   return GTK_DATABOX_GRAPH (bars);
}

/**
 * gtk_databox_offset_bars_new_full:
 * @maxlen: maximum length of @X and @Y
 * @len: actual number of @X and @Y values to plot
 * @X: array of horizontal position values of markers
 * @Y1: array of starting vertical position values of markers
 * @Y2: array of ending vertical position values of markers
 * @xstart: the first element in the X array to plot (usually 0)
 * @y1start: the first element in the Y1 array to plot (usually 0)
 * @y2start: the first element in the Y2 array to plot (usually 0)
 * @xstride: successive elements in the X array are separated by this much (1 if array, ncols if matrix)
 * @y1stride: successive elements in the Y1 array are separated by this much (1 if array, ncols if matrix)
 * @y2stride: successive elements in the Y2 array are separated by this much (1 if array, ncols if matrix)
 * @xtype: the GType of the X array elements.  G_TYPE_FLOAT, G_TYPE_DOUBLE, etc.
 * @ytype: the GType of the Y1/Y2 array elements.  G_TYPE_FLOAT, G_TYPE_DOUBLE, etc.
 * @color: color of the markers
 * @size: marker size or line width (depending on the @type)
 *
 * Creates a new #GtkDataboxOffsetBars object which can be added to a #GtkDatabox widget
 *
 * Return value: A new #GtkDataboxOffsetBars object
 **/
GtkDataboxGraph *
gtk_databox_offset_bars_new_full (guint maxlen, guint len,
			void * X, guint xstart, guint xstride, GType xtype,
			void * Y1, guint y1start, guint y1stride,
			void * Y2, guint y2start, guint y2stride, GType ytype,
		    GdkRGBA * color, guint size)
{
   GtkDataboxOffsetBars *bars;
   g_return_val_if_fail (X, NULL);
   g_return_val_if_fail (Y1, NULL);
   g_return_val_if_fail (Y2, NULL);
   g_return_val_if_fail ((len > 0), NULL);

   bars = g_object_new (GTK_DATABOX_TYPE_OFFSET_BARS,
			"X-Values", X,
			"Y1-Values", Y1,
			"Y2-Values", Y2,
			"xstart", xstart,
			"y1start", y1start,
			"y2start", y2start,
			"xstride", xstride,
			"y1stride", y1stride,
			"y2stride", y2stride,
			"xtype", xtype,
			"ytype", ytype,
			"length", len,
			"maxlen", maxlen,
			"color", color, "size", size, NULL);

   return GTK_DATABOX_GRAPH (bars);
}

static void
gtk_databox_offset_bars_real_draw (GtkDataboxGraph * graph,
			    GtkDatabox* box)
{
   GtkDataboxOffsetBars *bars = GTK_DATABOX_OFFSET_BARS (graph);
   GtkDataboxOffsetBarsPrivate *priv = GTK_DATABOX_OFFSET_BARS_GET_PRIVATE(graph);
   guint i = 0;
   void *X;
   void *Y1;
   void *Y2;
   guint len, maxlen;
   cairo_t *cr;
   gint16 *xpixels, *y1pixels, *y2pixels;
   guint xstart, xstride, y1start, y1stride, y2start, y2stride;
   GType xtype, ytype;

   g_return_if_fail (GTK_DATABOX_IS_OFFSET_BARS (bars));
   g_return_if_fail (GTK_IS_DATABOX (box));

   if (gtk_databox_get_scale_type_y (box) == GTK_DATABOX_SCALE_LOG)
      g_warning
	 ("gtk_databox_offset_bars do not work well with logarithmic scale in Y axis");

   len = gtk_databox_xyyc_graph_get_length (GTK_DATABOX_XYYC_GRAPH (graph));
   maxlen = gtk_databox_xyyc_graph_get_maxlen (GTK_DATABOX_XYYC_GRAPH (graph));

   if (priv->pixelsalloc < len)
   {
   	priv->pixelsalloc = len;
	priv->xpixels = (gint16 *)g_realloc(priv->xpixels, len * sizeof(gint16));
	priv->y1pixels = (gint16 *)g_realloc(priv->y1pixels, len * sizeof(gint16));
	priv->y2pixels = (gint16 *)g_realloc(priv->y2pixels, len * sizeof(gint16));
   }

   xpixels = priv->xpixels;
   y1pixels = priv->y1pixels;
   y2pixels = priv->y2pixels;

   X = gtk_databox_xyyc_graph_get_X (GTK_DATABOX_XYYC_GRAPH (graph));
   xstart = gtk_databox_xyyc_graph_get_xstart (GTK_DATABOX_XYYC_GRAPH (graph));
   xstride = gtk_databox_xyyc_graph_get_xstride (GTK_DATABOX_XYYC_GRAPH (graph));
   xtype = gtk_databox_xyyc_graph_get_xtype (GTK_DATABOX_XYYC_GRAPH (graph));
   gtk_databox_values_to_xpixels(box, xpixels, X, xtype, maxlen, xstart, xstride, len);

   ytype = gtk_databox_xyyc_graph_get_ytype (GTK_DATABOX_XYYC_GRAPH (graph));
   Y1 = gtk_databox_xyyc_graph_get_Y1 (GTK_DATABOX_XYYC_GRAPH (graph));
   y1start = gtk_databox_xyyc_graph_get_y1start (GTK_DATABOX_XYYC_GRAPH (graph));
   y1stride = gtk_databox_xyyc_graph_get_y1stride (GTK_DATABOX_XYYC_GRAPH (graph));
   gtk_databox_values_to_ypixels(box, y1pixels, Y1, ytype, maxlen, y1start, y1stride, len);

   Y2 = gtk_databox_xyyc_graph_get_Y2 (GTK_DATABOX_XYYC_GRAPH (graph));
   y2start = gtk_databox_xyyc_graph_get_y2start (GTK_DATABOX_XYYC_GRAPH (graph));
   y2stride = gtk_databox_xyyc_graph_get_y2stride (GTK_DATABOX_XYYC_GRAPH (graph));
   gtk_databox_values_to_ypixels(box, y2pixels, Y2, ytype, maxlen, y2start, y2stride, len);

   cr = gtk_databox_graph_create_gc (graph, box);

   for (i = 0; i < len; i++, xpixels++, y1pixels++, y2pixels++)
   {
      cairo_move_to (cr, *xpixels + 0.5, *y1pixels + 0.5);
      cairo_line_to (cr, *xpixels + 0.5, *y2pixels + 0.5);
   }
   cairo_stroke(cr);
   cairo_destroy(cr);

   return;
}

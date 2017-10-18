/* $Id: gtkdatabox_markers.c 4 2008-06-22 09:19:11Z rbock $ */
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

#include <gtkdatabox_markers.h>
#include <pango/pango.h>

G_DEFINE_TYPE(GtkDataboxMarkers, gtk_databox_markers,
	GTK_DATABOX_TYPE_XYC_GRAPH)

static void gtk_databox_markers_real_draw (GtkDataboxGraph * markers,
					  GtkDatabox* box);
static cairo_t* gtk_databox_markers_real_create_gc (GtkDataboxGraph * graph,
					       GtkDatabox* box);

/* IDs of properties */
enum
{
   PROP_TYPE = 1
};


typedef struct
{
   GtkDataboxMarkersPosition position;	/* relative to data point */
   gchar *text;
   PangoLayout *label;		/* the label for markers */
   GtkDataboxMarkersTextPosition label_position;	/* position relative to markers */
   gboolean boxed;		/* label in a box? */
}
GtkDataboxMarkersInfo;

/**
 * GtkDataboxMarkersPrivate:
 *
 * A private data structure used by the #GtkDataboxMarkers. It shields all internal things
 * from developers who are just using the object.
 *
 **/
typedef struct _GtkDataboxMarkersPrivate GtkDataboxMarkersPrivate;

struct _GtkDataboxMarkersPrivate
{
   GtkDataboxMarkersType type;
   GtkDataboxMarkersInfo *markers_info;
   gint16 *xpixels;
   gint16 *ypixels;
   guint pixelsalloc;
};

static void
gtk_databox_markers_set_mtype (GtkDataboxMarkers * markers, gint type)
{
   g_return_if_fail (GTK_DATABOX_IS_MARKERS (markers));

   GTK_DATABOX_MARKERS_GET_PRIVATE(markers)->type = type;

   g_object_notify (G_OBJECT (markers), "markers-type");
}

static void
gtk_databox_markers_set_property (GObject * object,
				 guint property_id,
				 const GValue * value, GParamSpec * pspec)
{
   GtkDataboxMarkers *markers = GTK_DATABOX_MARKERS (object);

   switch (property_id)
   {
   case PROP_TYPE:
      {
	 gtk_databox_markers_set_mtype (markers, g_value_get_int (value));
      }
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}

static gint
gtk_databox_markers_get_mtype (GtkDataboxMarkers * markers)
{
   g_return_val_if_fail (GTK_DATABOX_IS_MARKERS (markers), 0);

   return GTK_DATABOX_MARKERS_GET_PRIVATE(markers)->type;
}

static void
gtk_databox_markers_get_property (GObject * object,
				 guint property_id,
				 GValue * value, GParamSpec * pspec)
{
   GtkDataboxMarkers *markers = GTK_DATABOX_MARKERS (object);

   switch (property_id)
   {
   case PROP_TYPE:
      {
	 g_value_set_int (value, gtk_databox_markers_get_mtype (markers));
      }
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}

static void
markers_finalize (GObject * object)
{
   GtkDataboxMarkers *markers = GTK_DATABOX_MARKERS (object);
   GtkDataboxMarkersPrivate *priv = GTK_DATABOX_MARKERS_GET_PRIVATE (markers);
   int i;
   int len;

   len = gtk_databox_xyc_graph_get_length (GTK_DATABOX_XYC_GRAPH (markers));

   for (i = 0; i < len; ++i)
   {
      if (priv->markers_info[i].label)
	 g_object_unref (priv->markers_info[i].label);
      if (priv->markers_info[i].text)
	 g_free (priv->markers_info[i].text);
   }
   g_free (priv->markers_info);
   g_free (priv->xpixels);
   g_free (priv->ypixels);

   /* Chain up to the parent class */
   G_OBJECT_CLASS (gtk_databox_markers_parent_class)->finalize (object);
}

static cairo_t *
gtk_databox_markers_real_create_gc (GtkDataboxGraph * graph,
				   GtkDatabox* box)
{
   GtkDataboxMarkers *markers = GTK_DATABOX_MARKERS (graph);
   GtkDataboxMarkersPrivate *priv = GTK_DATABOX_MARKERS_GET_PRIVATE (markers);
   cairo_t *cr;
   static const double dash = 5.0f;

   g_return_val_if_fail (GTK_DATABOX_IS_MARKERS (graph), NULL);

   cr = GTK_DATABOX_GRAPH_CLASS (gtk_databox_markers_parent_class)->create_gc (graph, box);

   if (cr)
   {
      if (priv->type == GTK_DATABOX_MARKERS_DASHED_LINE)
         cairo_set_dash (cr, &dash, 1, 0.0);
   }

   return cr;
}

static void
gtk_databox_markers_class_init (GtkDataboxMarkersClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   GtkDataboxGraphClass *graph_class = GTK_DATABOX_GRAPH_CLASS (klass);
   GParamSpec *markers_param_spec;

   gobject_class->set_property = gtk_databox_markers_set_property;
   gobject_class->get_property = gtk_databox_markers_get_property;
   gobject_class->finalize = markers_finalize;

   markers_param_spec = g_param_spec_int ("markers-type", "Type of markers", "Type of markers for this graph, e.g. triangles or lines", G_MININT, G_MAXINT, 0,	/*  default value */
					 G_PARAM_CONSTRUCT |
					 G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    PROP_TYPE, markers_param_spec);
   graph_class->draw = gtk_databox_markers_real_draw;
   graph_class->create_gc = gtk_databox_markers_real_create_gc;

   g_type_class_add_private (klass, sizeof (GtkDataboxMarkersPrivate));
}

static void
complete (GtkDataboxMarkers * markers)
{
   GTK_DATABOX_MARKERS_GET_PRIVATE(markers)->markers_info =
      g_new0 (GtkDataboxMarkersInfo,
	      gtk_databox_xyc_graph_get_length
	      (GTK_DATABOX_XYC_GRAPH (markers)));

}

static void
gtk_databox_markers_init (GtkDataboxMarkers *markers)
{
   GtkDataboxMarkersPrivate *priv = GTK_DATABOX_MARKERS_GET_PRIVATE (markers);
   priv->markers_info = NULL;
   priv->xpixels = NULL;
   priv->ypixels = NULL;
   priv->pixelsalloc = 0;

   g_signal_connect (markers, "notify::length", G_CALLBACK (complete), NULL);
}

/**
 * gtk_databox_markers_new:
 * @len: length of @X and @Y
 * @X: array of horizontal position values of markers
 * @Y: array of vertical position values of markers
 * @color: color of the markers
 * @size: marker size or line width (depending on the @type)
 * @type: type of markers (e.g. triangle or circle)
 *
 * Creates a new #GtkDataboxMarkers object which can be added to a #GtkDatabox widget as nice decoration for other graphs.
 *
 * Return value: A new #GtkDataboxMarkers object
 **/
GtkDataboxGraph *
gtk_databox_markers_new (guint len, gfloat * X, gfloat * Y,
			GdkRGBA * color, guint size,
			GtkDataboxMarkersType type)
{
   GtkDataboxMarkers *markers;
   g_return_val_if_fail (X, NULL);
   g_return_val_if_fail (Y, NULL);
   g_return_val_if_fail ((len > 0), NULL);

   markers = g_object_new (GTK_DATABOX_TYPE_MARKERS,
			  "X-Values", X,
			  "Y-Values", Y,
			  "xstart", 0,
			  "ystart", 0,
			  "xstride", 1,
			  "ystride", 1,
			  "xtype", G_TYPE_FLOAT,
			  "ytype", G_TYPE_FLOAT,
			  "length", len,
			  "maxlen", len,
			  "color", color,
			  "size", size, "markers-type", type, NULL);

   return GTK_DATABOX_GRAPH (markers);
}

/**
 * gtk_databox_markers_new_full:
 * @maxlen: maximum length of @X and @Y
 * @len: actual number of @X and @Y values to plot
 * @X: array of horizontal position values of markers
 * @Y: array of vertical position values of markers
 * @xstart: the first element in the X array to plot (usually 0)
 * @ystart: the first element in the Y array to plot (usually 0)
 * @xstride: successive elements in the X array are separated by this much (1 if array, ncols if matrix)
 * @ystride: successive elements in the Y array are separated by this much (1 if array, ncols if matrix)
 * @xtype: the GType of the X array elements.  G_TYPE_FLOAT, G_TYPE_DOUBLE, etc.
 * @ytype: the GType of the Y array elements.  G_TYPE_FLOAT, G_TYPE_DOUBLE, etc.
 * @color: color of the markers
 * @size: marker size or line width (depending on the @type)
 * @type: type of markers (e.g. triangle or circle)
 *
 * Creates a new #GtkDataboxMarkers object which can be added to a #GtkDatabox widget as nice decoration for other graphs.
 *
 * Return value: A new #GtkDataboxMarkers object
 **/
GtkDataboxGraph *
gtk_databox_markers_new_full (guint maxlen, guint len,
			void * X, guint xstart, guint xstride, GType xtype,
			void * Y, guint ystart, guint ystride, GType ytype,
			GdkRGBA * color, guint size,
			GtkDataboxMarkersType type)
{
   GtkDataboxMarkers *markers;
   g_return_val_if_fail (X, NULL);
   g_return_val_if_fail (Y, NULL);
   g_return_val_if_fail ((len > 0), NULL);

   markers = g_object_new (GTK_DATABOX_TYPE_MARKERS,
			  "X-Values", X,
			  "Y-Values", Y,
			  "xstart", xstart,
			  "ystart", ystart,
			  "xstride", xstride,
			  "ystride", ystride,
			  "xtype", xtype,
			  "ytype", ytype,
			  "length", len,
			  "maxlen", maxlen,
			  "color", color,
			  "size", size, "markers-type", type, NULL);

   return GTK_DATABOX_GRAPH (markers);
}

static gint
gtk_databox_label_write_at (cairo_t *cr,
			    PangoLayout * pl,
			    GdkPoint coord,
			    GtkDataboxMarkersTextPosition position,
			    gint distance, gboolean boxed)
{
   gint hdist_text;
   gint vdist_text;
   gint hdist_box;
   gint vdist_box;

   gint width;
   gint height;

   gint offset = (boxed) ? 2 : 0;

   pango_layout_get_pixel_size (pl, &width, &height);

   switch (position)
   {
   case GTK_DATABOX_MARKERS_TEXT_N:
      hdist_text = -width / 2;
      vdist_text = -distance - offset - height;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   case GTK_DATABOX_MARKERS_TEXT_NE:
      hdist_text = +distance + offset;
      vdist_text = -distance - offset - height;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   case GTK_DATABOX_MARKERS_TEXT_E:
      hdist_text = +distance + offset;
      vdist_text = -height / 2;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   case GTK_DATABOX_MARKERS_TEXT_SE:
      hdist_text = +distance + offset;
      vdist_text = +distance + offset;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   case GTK_DATABOX_MARKERS_TEXT_S:
      hdist_text = -width / 2;
      vdist_text = +distance + offset;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   case GTK_DATABOX_MARKERS_TEXT_SW:
      hdist_text = -distance - offset - width;
      vdist_text = +distance + offset;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   case GTK_DATABOX_MARKERS_TEXT_W:
      hdist_text = -distance - offset - width;
      vdist_text = -height / 2;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   case GTK_DATABOX_MARKERS_TEXT_NW:
      hdist_text = -distance - offset - width;
      vdist_text = -distance - offset - height;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
      break;
   default:
      hdist_text = -width / 2;
      vdist_text = -height / 2;
      hdist_box = hdist_text - offset;
      vdist_box = vdist_text - offset;
   }


   cairo_move_to(cr, coord.x + hdist_text, coord.y + vdist_text);
   pango_cairo_show_layout(cr, pl);

   if (boxed) {
      cairo_save (cr);
      cairo_set_line_width (cr, 1.0);
      cairo_set_dash (cr, NULL, 0, 0.0);
      cairo_rectangle (cr, coord.x + hdist_box - 0.5,
			  coord.y + vdist_box - 0.5, width + 3.5, height + 3.5);
	  cairo_stroke(cr);
	  cairo_restore(cr);
	}

   return (0);
}

static void
gtk_databox_markers_real_draw (GtkDataboxGraph * graph,
			      GtkDatabox* box)
{
   GtkWidget *widget;
   GtkDataboxMarkers *markers = GTK_DATABOX_MARKERS (graph);
   GtkDataboxMarkersPrivate *priv = GTK_DATABOX_MARKERS_GET_PRIVATE (markers);
   GdkPoint points[3];
   PangoContext *context;
   void *X;
   void *Y;
   guint len, maxlen;
   gint16 x;
   gint16 y;
   gint16 widget_width;
   gint16 widget_height;
   GdkPoint coord;
   gint size;
   guint i;
   cairo_t *cr;
   GtkAllocation allocation;
   gint16 *xpixels, *ypixels;
   guint xstart, xstride, ystart, ystride;
   GType xtype, ytype;

   g_return_if_fail (GTK_DATABOX_IS_MARKERS (markers));
   g_return_if_fail (GTK_IS_DATABOX (box));

   widget = GTK_WIDGET(box);
   gtk_widget_get_allocation(widget, &allocation);

   context = gtk_widget_get_pango_context(widget);

   len = gtk_databox_xyc_graph_get_length (GTK_DATABOX_XYC_GRAPH (graph));
   maxlen = gtk_databox_xyc_graph_get_maxlen (GTK_DATABOX_XYC_GRAPH (graph));

   if (priv->pixelsalloc < len)
   {
   	priv->pixelsalloc = len;
	priv->xpixels = (gint16 *)g_realloc(priv->xpixels, len * sizeof(gint16));
	priv->ypixels = (gint16 *)g_realloc(priv->ypixels, len * sizeof(gint16));
   }

   xpixels = priv->xpixels;
   ypixels = priv->ypixels;

   X = gtk_databox_xyc_graph_get_X (GTK_DATABOX_XYC_GRAPH (graph));
   xstart = gtk_databox_xyc_graph_get_xstart (GTK_DATABOX_XYC_GRAPH (graph));
   xstride = gtk_databox_xyc_graph_get_xstride (GTK_DATABOX_XYC_GRAPH (graph));
   xtype = gtk_databox_xyc_graph_get_xtype (GTK_DATABOX_XYC_GRAPH (graph));
   gtk_databox_values_to_xpixels(box, xpixels, X, xtype, maxlen, xstart, xstride, len);

   Y = gtk_databox_xyc_graph_get_Y (GTK_DATABOX_XYC_GRAPH (graph));
   ystart = gtk_databox_xyc_graph_get_ystart (GTK_DATABOX_XYC_GRAPH (graph));
   ystride = gtk_databox_xyc_graph_get_ystride (GTK_DATABOX_XYC_GRAPH (graph));
   ytype = gtk_databox_xyc_graph_get_ytype (GTK_DATABOX_XYC_GRAPH (graph));
   gtk_databox_values_to_ypixels(box, ypixels, Y, ytype, maxlen, ystart, ystride, len);

   size = gtk_databox_graph_get_size (graph);

   widget_width = allocation.width;
   widget_height = allocation.height;

   cr = gtk_databox_graph_create_gc (graph, box);

	for (i = 0; i < len; ++i)
	{
		coord.x = x = xpixels[i];
		coord.y = y = ypixels[i];

		switch (priv->type)
		{
			case GTK_DATABOX_MARKERS_TRIANGLE:
				switch (priv->markers_info[i].position)
				{
					case GTK_DATABOX_MARKERS_C:
						y = y - size / 2;
						points[0].x = x;
						points[0].y = y;
						points[1].x = x - size / 2;
						points[1].y = y + size;
						points[2].x = x + size / 2;
						points[2].y = y + size;
						break;

					case GTK_DATABOX_MARKERS_N:
						coord.y = y - 2 - size / 2;
						y = y - 2;
						points[0].x = x;
						points[0].y = y;
						points[1].x = x - size / 2;
						points[1].y = y - size;
						points[2].x = x + size / 2;
						points[2].y = y - size;
						break;

					case GTK_DATABOX_MARKERS_E:
						coord.x = x + 2 + size / 2;
						x = x + 2;
						points[0].x = x;
						points[0].y = y;
						points[1].x = x + size;
						points[1].y = y + size / 2;
						points[2].x = x + size;
						points[2].y = y - size / 2;
						break;

					case GTK_DATABOX_MARKERS_S:
						coord.y = y + 2 + size / 2;
						y = y + 2;
						points[0].x = x;
						points[0].y = y;
						points[1].x = x - size / 2;
						points[1].y = y + size;
						points[2].x = x + size / 2;
						points[2].y = y + size;
						break;

					case GTK_DATABOX_MARKERS_W:
					default:
						coord.x = x - 2 - size / 2;
						x = x - 2;
						points[0].x = x;
						points[0].y = y;
						points[1].x = x - size;
						points[1].y = y + size / 2;
						points[2].x = x - size;
						points[2].y = y - size / 2;
						break;
				}
				cairo_move_to(cr, points[0].x + 0.5, points[0].y + 0.5);
				cairo_line_to(cr, points[1].x + 0.5, points[1].y + 0.5);
				cairo_line_to(cr, points[2].x + 0.5, points[2].y + 0.5);
				cairo_close_path  (cr);
				cairo_fill(cr);
				break;
				/* End of GTK_DATABOX_MARKERS_TRIANGLE */

				case GTK_DATABOX_MARKERS_SOLID_LINE:
				case GTK_DATABOX_MARKERS_DASHED_LINE:
				switch (priv->markers_info[i].position)
				{
					case GTK_DATABOX_MARKERS_C:
						points[0].x = x;
						points[0].y = 0;
						points[1].x = x;
						points[1].y = widget_height;
						break;

					case GTK_DATABOX_MARKERS_N:
						points[0].x = x;
						points[0].y = 0;
						points[1].x = x;
						points[1].y = widget_height;
						break;

					case GTK_DATABOX_MARKERS_E:
						points[0].x = 0;
						points[0].y = y;
						points[1].x = widget_width;
						points[1].y = y;
						break;

					case GTK_DATABOX_MARKERS_S:
						points[0].x = x;
						points[0].y = 0;
						points[1].x = x;
						points[1].y = widget_height;
						break;

					case GTK_DATABOX_MARKERS_W:
					default:
						points[0].x = 0;
						points[0].y = y;
						points[1].x = widget_width;
						points[1].y = y;
						break;
				}
				cairo_move_to(cr, points[0].x + 0.5, points[0].y + 0.5);
				cairo_line_to(cr, points[1].x + 0.5, points[1].y + 0.5);
				cairo_stroke(cr);

				break;
				/* End of GTK_DATABOX_MARKERS_LINE */

				case GTK_DATABOX_MARKERS_NONE:
				default:
					break;
		}

      if (priv->markers_info[i].text)
      {
		if (!priv->markers_info[i].label)
		{
			priv->markers_info[i].label =
			pango_layout_new (context);
			pango_layout_set_text (priv->markers_info[i].label,
			priv->markers_info[i].text, -1);
		}

	 if (priv->type == GTK_DATABOX_MARKERS_SOLID_LINE
	     || priv->type == GTK_DATABOX_MARKERS_DASHED_LINE)
	 {
	    gint width;
	    gint height;
	    pango_layout_get_pixel_size (priv->markers_info[i].label,
					 &width, &height);

	    width = (width + 1) / 2 + 2;
	    height = (height + 1) / 2 + 2;
	    size = 0;

	    switch (priv->markers_info[i].position)
	    {
	    case GTK_DATABOX_MARKERS_C:
	       break;
	    case GTK_DATABOX_MARKERS_N:
	       coord.y = height;
	       break;
	    case GTK_DATABOX_MARKERS_E:
	       coord.x = widget_width - width;
	       break;
	    case GTK_DATABOX_MARKERS_S:
	       coord.y = widget_height - height;
	       break;
	    case GTK_DATABOX_MARKERS_W:
	       coord.x = width;
	       break;
	    }
	 }

	 gtk_databox_label_write_at (cr,
				     priv->markers_info[i].label,
				     coord,
				     priv->markers_info[i].
				     label_position, (size + 1) / 2 + 2,
				     priv->markers_info[i].boxed);
      }
   }

   cairo_destroy(cr);
   return;
}

/**
 * gtk_databox_markers_set_position:
 * @markers: A #GtkDataboxMarkers object
 * @index: index within the array of X/Y values
 * @position: position of the marker (e.g. circle or triangle relative to their X/Y value
 *
 * Sets a position for one of the markers.
 **/
void
gtk_databox_markers_set_position (GtkDataboxMarkers * markers,
				 guint index,
				 GtkDataboxMarkersPosition position)
{
   GtkDataboxMarkersPrivate *priv = GTK_DATABOX_MARKERS_GET_PRIVATE (markers);
   guint len;

   g_return_if_fail (GTK_DATABOX_IS_MARKERS (markers));
   len = gtk_databox_xyc_graph_get_length (GTK_DATABOX_XYC_GRAPH (markers));
   g_return_if_fail (index < len);

   priv->markers_info[index].position = position;
}

/**
 * gtk_databox_markers_set_label:
 * @markers: A #GtkDataboxMarkers object
 * @index: index within the array of X/Y values
 * @label_position: position of the label relative to the marker
 * @text: text to be displayed in the label
 * @boxed: Whether the label is to be enclosed in a box (true) or not (false)
 *
 * Sets a label for one of the markers.
 **/
void
gtk_databox_markers_set_label (GtkDataboxMarkers * markers,
			      guint index,
			      GtkDataboxMarkersTextPosition label_position,
			      gchar * text, gboolean boxed)
{
   GtkDataboxMarkersPrivate *priv = GTK_DATABOX_MARKERS_GET_PRIVATE (markers);
   guint len;

   g_return_if_fail (GTK_DATABOX_IS_MARKERS (markers));
   len = gtk_databox_xyc_graph_get_length (GTK_DATABOX_XYC_GRAPH (markers));
   g_return_if_fail (index < len);

   priv->markers_info[index].label_position = label_position;
   if (priv->markers_info[index].text)
      g_free (priv->markers_info[index].text);
   priv->markers_info[index].text = g_strdup (text);
   priv->markers_info[index].boxed = boxed;

   if (priv->markers_info[index].label)
   {
      pango_layout_set_text (priv->markers_info[index].label,
			     priv->markers_info[index].text, -1);
   }
}

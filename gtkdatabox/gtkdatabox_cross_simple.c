/* $Id: gtkdatabox_cross_simple.c 4 2008-06-22 09:19:11Z rbock $ */
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

#include <gtkdatabox_cross_simple.h>

G_DEFINE_TYPE(GtkDataboxCrossSimple, gtk_databox_cross_simple,
	GTK_DATABOX_TYPE_MARKERS)

static void
cross_simple_finalize (GObject * object)
{
   gpointer pointer;

   pointer = gtk_databox_xyc_graph_get_X (GTK_DATABOX_XYC_GRAPH (object));
   if (pointer)
      g_free (pointer);

   pointer = gtk_databox_xyc_graph_get_Y (GTK_DATABOX_XYC_GRAPH (object));
   if (pointer)
      g_free (pointer);

   /* Chain up to the parent class */
   G_OBJECT_CLASS (gtk_databox_cross_simple_parent_class)->finalize (object);
}

static void
gtk_databox_cross_simple_class_init (GtkDataboxCrossSimpleClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   gobject_class->finalize = cross_simple_finalize;
}

static void gtk_databox_cross_simple_init(GtkDataboxCrossSimple *cross) { cross = cross; }

/**
 * gtk_databox_cross_simple_new:
 * @color: color of the markers
 * @size: marker size or line width (depending on the @type)
 *
 * Creates a new #GtkDataboxCrossSimple object which can be added to a #GtkDatabox widget as nice decoration for other graphs.
 *
 * Return value: A new #GtkDataboxCrossSimple object
 **/
GtkDataboxGraph *
gtk_databox_cross_simple_new (GdkRGBA * color, guint size)
{
   GtkDataboxCrossSimple *cross_simple;
   gfloat *X = g_new0 (gfloat, 2);
   gfloat *Y = g_new0 (gfloat, 2);
   gint len = 2;

   cross_simple = g_object_new (GTK_DATABOX_TYPE_CROSS_SIMPLE,
				"markers-type", GTK_DATABOX_MARKERS_SOLID_LINE,
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
				"color", color, "size", size, NULL);

   gtk_databox_markers_set_position (GTK_DATABOX_MARKERS (cross_simple), 0,
				    GTK_DATABOX_MARKERS_C);
   gtk_databox_markers_set_label (GTK_DATABOX_MARKERS (cross_simple), 0,
				 GTK_DATABOX_MARKERS_TEXT_SW, "0", FALSE);
   gtk_databox_markers_set_position (GTK_DATABOX_MARKERS (cross_simple), 1,
				    GTK_DATABOX_MARKERS_W);

   return GTK_DATABOX_GRAPH (cross_simple);
}

/* $Id: gtkdatabox_regions.h 4 2008-06-22 09:19:11Z rbock $ */
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

/**
 * SECTION:gtkdatabox_regions
 * @short_description: A #GtkDataboxGraph used for displaying xxyy-values (x1, x2, y1 and y2 values) as oblongs from x1 to y1, x2 to y2.
 * @include: gtkdatabox_regions.h
 * @see_also: #GtkDatabox, #GtkDataboxGraph, #GtkDataboxPoints, #GtkDataboxLines, #GtkDataboxMarkers, #GtkDataboxRegions
 *
 * #GtkDataboxRegions is a #GtkDataboxGraph class for displaying xxyy-values as oblongs.
 *
 */

#ifndef __GTK_DATABOX_REGIONS_H__
#define __GTK_DATABOX_REGIONS_H__

#include <gtkdatabox_xyyc_graph.h>

G_BEGIN_DECLS
#define GTK_DATABOX_TYPE_REGIONS		  (gtk_databox_regions_get_type ())
#define GTK_DATABOX_REGIONS(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GTK_DATABOX_TYPE_REGIONS, \
                                           GtkDataboxRegions))
#define GTK_DATABOX_REGIONS_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                           GTK_DATABOX_TYPE_REGIONS, \
                                           GtkDataboxRegionsClass))
#define GTK_DATABOX_IS_REGIONS(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           GTK_DATABOX_TYPE_REGIONS))
#define GTK_DATABOX_IS_REGIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                           GTK_DATABOX_TYPE_REGIONS))
#define GTK_DATABOX_REGIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                           GTK_DATABOX_TYPE_REGIONS, \
                                           GtkDataboxRegionsClass))
#define GTK_DATABOX_REGIONS_GET_PRIVATE(obj) \
	G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_DATABOX_TYPE_REGIONS, \
	GtkDataboxRegionsPrivate)

/**
 * GtkDataboxRegions:
 * @see_also: #GtkDatabox, #GtkDataboxGraph, #GtkDataboxPoints, #GtkDataboxLines, #GtkDataboxMarkers, #GtkDataboxBars, #GtkDataboxOffsetBars
 *
 * #GtkDataboxRegions is a #GtkDataboxGraph class for displaying xxyy-values as oblongs.
 *
 */
   typedef struct _GtkDataboxRegions GtkDataboxRegions;

   typedef struct _GtkDataboxRegionsClass GtkDataboxRegionsClass;

   struct _GtkDataboxRegions
   {
      /*< private >*/
      GtkDataboxXYYCGraph parent;
   };

   struct _GtkDataboxRegionsClass
   {
      GtkDataboxXYYCGraphClass parent_class;
   };

   GType gtk_databox_regions_get_type (void);

   GtkDataboxGraph *gtk_databox_regions_new (guint len, gfloat * X, gfloat * Y1, gfloat * Y2, GdkRGBA * color);
   GtkDataboxGraph *gtk_databox_regions_new_full (guint maxlen, guint len,
			void * X, guint xstart, guint xstride, GType xtype,
			void * Y1, guint y1start, guint y1stride,
			void * Y2, guint y2start, guint y2stride, GType ytype,
		    GdkRGBA * color);
G_END_DECLS
#endif				/* __GTK_DATABOX_REGIONS_H__ */

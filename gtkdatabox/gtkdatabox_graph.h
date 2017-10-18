/* $Id: gtkdatabox_graph.h 4 2008-06-22 09:19:11Z rbock $ */
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
 * SECTION:gtkdatabox_graph
 * @short_description: An abstract anchestor for all "real" graphs.
 * @include: gtkdatabox_graph.h
 * @see_also: #GtkDatabox, #GtkDataboxPoints, #GtkDataboxLines, #GtkDataboxBars, #GtkDataboxGrid, #GtkDataboxCrossSimple
 *
 * GtkDataboxGraphs can display data or other things in a #GtkDatabox widget.
 *
 * This class is just the basic interface. Other graph classes are derived from this
 * class and implement some real things.
 *
 */


#ifndef __GTK_DATABOX_GRAPH_H__
#define __GTK_DATABOX_GRAPH_H__

#include <gdk/gdk.h>
#include <pango/pango.h>
#include <gtkdatabox.h>

G_BEGIN_DECLS
#define GTK_DATABOX_TYPE_GRAPH		  (gtk_databox_graph_get_type ())
#define GTK_DATABOX_GRAPH(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           GTK_DATABOX_TYPE_GRAPH, \
                                           GtkDataboxGraph))
#define GTK_DATABOX_GRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                           GTK_DATABOX_TYPE_GRAPH, \
                                           GtkDataboxGraphClass))
#define GTK_DATABOX_IS_GRAPH(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           GTK_DATABOX_TYPE_GRAPH))
#define GTK_DATABOX_IS_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                           GTK_DATABOX_TYPE_GRAPH))
#define GTK_DATABOX_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                           GTK_DATABOX_TYPE_GRAPH, \
                                           GtkDataboxGraphClass))
#define GTK_DATABOX_GRAPH_GET_PRIVATE(obj) \
	G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_DATABOX_TYPE_GRAPH, GtkDataboxGraphPrivate)

   typedef struct _GtkDataboxGraphClass GtkDataboxGraphClass;

   /**
    * _GtkDataboxGraph
    * @parent: The parent object
    *
    * Implementation of #GtkDataboxGraph
    *
    **/
   struct _GtkDataboxGraph
   {
      /*< private >*/
      GObject parent;
   };

   struct _GtkDataboxGraphClass
   {
      GObjectClass parent_class;

      /*
       * public virtual drawing function
       */
      void (*draw) (GtkDataboxGraph * graph, GtkDatabox * box);

      gint (*calculate_extrema) (GtkDataboxGraph * graph,
                                 gfloat * min_x, gfloat * max_x,
				 gfloat * min_y, gfloat * max_y);
      cairo_t* (*create_gc) (GtkDataboxGraph * graph, GtkDatabox * box);
   };

   GType gtk_databox_graph_get_type (void);

   void gtk_databox_graph_set_hide (GtkDataboxGraph * graph, gboolean hide);
   gboolean gtk_databox_graph_get_hide (GtkDataboxGraph * graph);

   void gtk_databox_graph_set_color (GtkDataboxGraph * graph,
				     GdkRGBA * color);
   GdkRGBA *gtk_databox_graph_get_color (GtkDataboxGraph * graph);

   void gtk_databox_graph_set_rgba (GtkDataboxGraph * graph,
				     GdkRGBA * rgba);
   GdkRGBA *gtk_databox_graph_get_rgba (GtkDataboxGraph * graph);

   void gtk_databox_graph_set_size (GtkDataboxGraph * graph, gint size);
   gint gtk_databox_graph_get_size (GtkDataboxGraph * graph);

   gint gtk_databox_graph_calculate_extrema (GtkDataboxGraph * graph,
					     gfloat * min_x, gfloat * max_x,
					     gfloat * min_y, gfloat * max_y);
   /* This function is called by GtkDatabox */
   void gtk_databox_graph_draw (GtkDataboxGraph * graph, GtkDatabox * box);

   /* This function is called by derived graph classes */
   cairo_t* gtk_databox_graph_create_gc (GtkDataboxGraph * graph, GtkDatabox * box);

G_END_DECLS
#endif				/* __GTK_DATABOX_GRAPH_H__ */

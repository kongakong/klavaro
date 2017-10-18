/* $Id: gtkdatabox_grid.c 4 2008-06-22 09:19:11Z rbock $ */
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

#include <gtkdatabox_grid.h>

G_DEFINE_TYPE(GtkDataboxGrid, gtk_databox_grid,
	GTK_DATABOX_TYPE_GRAPH)

static void gtk_databox_grid_real_draw (GtkDataboxGraph * grid,
					GtkDatabox* box);
static cairo_t* gtk_databox_grid_real_create_gc (GtkDataboxGraph * graph,
					     GtkDatabox* box);

/* IDs of properties */
enum
{
   GRID_HLINES = 1,
   GRID_VLINES,
   GRID_HLINE_VALS,
   GRID_VLINE_VALS
};

/**
 * GtkDataboxGridPrivate
 *
 * A private data structure used by the #GtkDataboxGrid. It shields all internal things
 * from developers who are just using the object.
 *
 **/
typedef struct _GtkDataboxGridPrivate GtkDataboxGridPrivate;

struct _GtkDataboxGridPrivate
{
   gint hlines;
   gint vlines;
   gfloat *hline_vals;
   gfloat *vline_vals;
};

static void
gtk_databox_grid_set_property (GObject * object,
			       guint property_id,
			       const GValue * value, GParamSpec * pspec)
{
   GtkDataboxGrid *grid = GTK_DATABOX_GRID (object);

   switch (property_id)
   {
   case GRID_HLINES:
      {
	 gtk_databox_grid_set_hlines (grid, g_value_get_int (value));
      }
      break;
   case GRID_VLINES:
      {
	 gtk_databox_grid_set_vlines (grid, g_value_get_int (value));
      }
      break;
   case GRID_HLINE_VALS:
      {
	 gtk_databox_grid_set_hline_vals (grid, (gfloat *) g_value_get_pointer (value));
      }
      break;
   case GRID_VLINE_VALS:
      {
	 gtk_databox_grid_set_vline_vals (grid, (gfloat *) g_value_get_pointer (value));
      }
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}

static void
gtk_databox_grid_get_property (GObject * object,
			       guint property_id,
			       GValue * value, GParamSpec * pspec)
{
   GtkDataboxGrid *grid = GTK_DATABOX_GRID (object);

   switch (property_id)
   {
   case GRID_HLINES:
      {
	 g_value_set_int (value, gtk_databox_grid_get_hlines (grid));
      }
      break;
   case GRID_VLINES:
      {
	 g_value_set_int (value, gtk_databox_grid_get_vlines (grid));
      }
      break;
   case GRID_HLINE_VALS:
      {
    g_value_set_pointer (value, gtk_databox_grid_get_hline_vals (grid));
      }
      break;
   case GRID_VLINE_VALS:
      {
    g_value_set_pointer (value, gtk_databox_grid_get_vline_vals (grid));
      }
      break;
   default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
   }
}



static cairo_t*
gtk_databox_grid_real_create_gc (GtkDataboxGraph * graph,
				 GtkDatabox* box)
{
   cairo_t *cr;
   static const double grid_dash = 5.0;

   g_return_val_if_fail (GTK_DATABOX_IS_GRID (graph), NULL);

   cr = GTK_DATABOX_GRAPH_CLASS (gtk_databox_grid_parent_class)->create_gc (graph, box);

   if (cr)
      cairo_set_dash(cr, &grid_dash, 1, 0.0);

   return cr;
}

static void
grid_finalize (GObject * object)
{
  /*GtkDataboxGraph *graph = GTK_DATABOX_GRAPH (object);*/

  /* Chain up to the parent class */
  G_OBJECT_CLASS (gtk_databox_grid_parent_class)->finalize (object);
}

static void
gtk_databox_grid_class_init (GtkDataboxGridClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   GtkDataboxGraphClass *graph_class = GTK_DATABOX_GRAPH_CLASS (klass);
   GParamSpec *grid_param_spec;

   gobject_class->set_property = gtk_databox_grid_set_property;
   gobject_class->get_property = gtk_databox_grid_get_property;
   gobject_class->finalize = grid_finalize;

   grid_param_spec = g_param_spec_int ("grid-hlines", "grid-hlines", "Number of horizontal lines", G_MININT, G_MAXINT, 0,	/* default value */
				       G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    GRID_HLINES, grid_param_spec);

   grid_param_spec = g_param_spec_int ("grid-vlines", "grid-vlines", "Number of vertical lines", G_MININT, G_MAXINT, 0,	/* default value */
				       G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    GRID_VLINES, grid_param_spec);

   grid_param_spec = g_param_spec_pointer ("grid-hline-vals", "Grid Hline Vals", "The locations of each of the horizontal lines", G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    GRID_HLINE_VALS, grid_param_spec);

   grid_param_spec = g_param_spec_pointer ("grid-vline-vals", "Grid Vline Vals", "The locations of each of the vertical lines", G_PARAM_READWRITE);

   g_object_class_install_property (gobject_class,
				    GRID_VLINE_VALS, grid_param_spec);

   graph_class->draw = gtk_databox_grid_real_draw;
   graph_class->create_gc = gtk_databox_grid_real_create_gc;

   g_type_class_add_private (klass, sizeof (GtkDataboxGridPrivate));
}

static void
gtk_databox_grid_init (GtkDataboxGrid *grid)
{
	grid = grid;
}

/**
 * gtk_databox_grid_new:
 * @hlines: number of horizontal lines in the grid
 * @vlines: number of vertical lines in the grid
 * @color: color of the grid
 * @size: line width of the grid
 *
 * Creates a new #GtkDataboxGrid object which can be added to a #GtkDatabox widget as nice decoration for other graphs.
 *
 * Return value: A new #GtkDataboxGrid object
 **/
GtkDataboxGraph *
gtk_databox_grid_new (gint hlines, gint vlines, GdkRGBA * color, guint size)
{
   GtkDataboxGrid *grid;
   grid = g_object_new (GTK_DATABOX_TYPE_GRID,
			"color", color,
			"size", size,
			"grid-hlines", hlines, "grid-vlines", vlines, "grid-hline-vals",NULL, "grid-vline-vals", NULL, NULL);

   return GTK_DATABOX_GRAPH (grid);
}

/**
 * gtk_databox_grid_array_new:
 * @hlines: number of horizontal lines in the grid
 * @vlines: number of vertical lines in the grid
 * @hline_vals: a pointer to an array of gfloat horizontal grid coordinate
 * @vline_vals: a pointer to an array of gfloat vertical grid coordinate
 * @color: color of the grid
 * @size: line width of the grid
 *
 * Creates a new #GtkDataboxGrid object which can be added to a #GtkDatabox widget as nice decoration for other graphs.
 *
 * Return value: A new #GtkDataboxGrid object
 **/
GtkDataboxGraph *gtk_databox_grid_array_new (gint hlines, gint vlines, gfloat * local_hline_vals, gfloat * local_vline_vals,
					  GdkRGBA * color, guint size)
{
   GtkDataboxGrid *grid;

   grid = g_object_new (GTK_DATABOX_TYPE_GRID,
			"color", color,
			"size", size,
			"grid-hlines", hlines, "grid-vlines", vlines, "grid-hline-vals", local_hline_vals, "grid-vline-vals", local_vline_vals, NULL);

   return GTK_DATABOX_GRAPH (grid);
}

static void
gtk_databox_grid_real_draw (GtkDataboxGraph * graph,
			    GtkDatabox* box)
{
   GtkWidget *widget;
   GtkDataboxGrid *grid = GTK_DATABOX_GRID (graph);
   GtkDataboxGridPrivate *priv = GTK_DATABOX_GRID_GET_PRIVATE(grid);   gint i = 0;
   gfloat x;
   gfloat y;
   gint16 width;
   gint16 height;
   gfloat offset_x;
   gfloat offset_y;
   gfloat factor_x;
   gfloat factor_y;
   gint16 pixel_x;
   gint16 pixel_y;
   gfloat left, right, top, bottom;
   cairo_t *cr;
   GtkAllocation allocation;

   g_return_if_fail (GTK_DATABOX_IS_GRID (grid));
   g_return_if_fail (GTK_IS_DATABOX (box));

   widget = GTK_WIDGET(box);
   gtk_widget_get_allocation(widget, &allocation);

   gtk_databox_get_total_limits (box, &left, &right, &top, &bottom);

   cr = gtk_databox_graph_create_gc (graph, box);

   width = allocation.width;
   height = allocation.height;

   offset_x = left;
   factor_x =
      (right - left) / (priv->vlines + 1);

   offset_y = top;
   factor_y =
      (bottom - top) / (priv->hlines + 1);

   if (priv->hline_vals == NULL)
      for (i = 0; i < priv->hlines; i++)
      {
         y = offset_y + (i + 1) * factor_y;
         pixel_y = gtk_databox_value_to_pixel_y (box, y);
         cairo_move_to (cr, 0.0, pixel_y + 0.5);
         cairo_line_to (cr, width, pixel_y + 0.5);
      }
   else
      for (i = 0; i < priv->hlines; i++)
      {
         y = priv->hline_vals[i];
         pixel_y = gtk_databox_value_to_pixel_y (box, y);
         cairo_move_to (cr, 0.0, pixel_y + 0.5);
         cairo_line_to (cr, width, pixel_y + 0.5);
      }

   if (priv->vline_vals == NULL)
      for (i = 0; i < priv->vlines; i++)
      {
         x = offset_x + (i + 1) * factor_x;
         pixel_x = gtk_databox_value_to_pixel_x (box, x);
         cairo_move_to (cr, pixel_x + 0.5, 0.0);
         cairo_line_to (cr, pixel_x + 0.5, height);
      }
   else
      for (i = 0; i < priv->vlines; i++)
      {
         x = priv->vline_vals[i];
         pixel_x = gtk_databox_value_to_pixel_x (box, x);
         cairo_move_to (cr, pixel_x + 0.5, 0);
         cairo_line_to (cr, pixel_x + 0.5, height);
      }
   cairo_stroke(cr);
   cairo_destroy(cr);

   return;
}

/**
 * gtk_databox_grid_set_hlines:
 * @grid: a #GtkDataboxGrid graph object
 * @hlines: number of vertical lines in the grid
 *
 * Sets the number of horizontal lines in the @grid.
 **/
void
gtk_databox_grid_set_hlines (GtkDataboxGrid * grid, gint hlines)
{
   g_return_if_fail (GTK_DATABOX_IS_GRID (grid));

   GTK_DATABOX_GRID_GET_PRIVATE(grid)->hlines = MAX (1, hlines);

   g_object_notify (G_OBJECT (grid), "grid-hlines");
}

/**
 * gtk_databox_grid_get_hlines:
 * @grid: a #GtkDataboxGrid graph object
 *
 * Gets the number of horizontal lines in the @grid.
 *
 * Return value: Number of horizontal lines in the @grid.
 **/
gint
gtk_databox_grid_get_hlines (GtkDataboxGrid * grid)
{
   g_return_val_if_fail (GTK_DATABOX_IS_GRID (grid), -1);

   return GTK_DATABOX_GRID_GET_PRIVATE(grid)->hlines;
}

/**
 * gtk_databox_grid_set_vlines:
 * @grid: a #GtkDataboxGrid graph object
 * @vlines: number of vertical lines in the grid
 *
 * Sets the number of vertical lines in the @grid.
 **/
void
gtk_databox_grid_set_vlines (GtkDataboxGrid * grid, gint vlines)
{
   g_return_if_fail (GTK_DATABOX_IS_GRID (grid));

   GTK_DATABOX_GRID_GET_PRIVATE(grid)->vlines = MAX (1, vlines);

   g_object_notify (G_OBJECT (grid), "grid-vlines");
}

/**
 * gtk_databox_grid_get_vlines:
 * @grid: a #GtkDataboxGrid graph object
 *
 * Gets the number of vertical lines in the @grid.
 *
 * Return value: Number of vertical lines in the @grid.
 **/
gint
gtk_databox_grid_get_vlines (GtkDataboxGrid * grid)
{
   g_return_val_if_fail (GTK_DATABOX_IS_GRID (grid), -1);

   return GTK_DATABOX_GRID_GET_PRIVATE(grid)->vlines;
}

/**
 * gtk_databox_grid_set_hline_vals:
 * @grid: a #GtkDataboxGrid graph object
 * @hline_vals: sets the pointer to the hline values for the grid
 *
 * Sets the pointer to the horizontal lines in the @grid.
 **/
void
gtk_databox_grid_set_hline_vals (GtkDataboxGrid * grid, gfloat *hline_vals)
{
   g_return_if_fail (GTK_DATABOX_IS_GRID (grid));

   GTK_DATABOX_GRID_GET_PRIVATE(grid)->hline_vals = hline_vals;

   g_object_notify (G_OBJECT (grid), "grid-hline-vals");
}

/**
 * gtk_databox_grid_get_hline_vals:
 * @grid: a #GtkDataboxGrid graph object
 *
 * Gets the pointer to the horizontal line values for the @grid.
 *
 * Return value: Pointer to the horizontal line values for the @grid. (or NULL if error)
 **/
gfloat*
gtk_databox_grid_get_hline_vals (GtkDataboxGrid * grid)
{
   g_return_val_if_fail (GTK_DATABOX_IS_GRID (grid), NULL);

   return GTK_DATABOX_GRID_GET_PRIVATE(grid)->hline_vals;
}

/**
 * gtk_databox_grid_set_vline_vals:
 * @grid: a #GtkDataboxGrid graph object
 * @vline_vals: sets the pointer to the vline values for the grid
 *
 * Sets the pointer to the vertical lines in the @grid.
 **/
void
gtk_databox_grid_set_vline_vals (GtkDataboxGrid * grid, gfloat *vline_vals)
{
   g_return_if_fail (GTK_DATABOX_IS_GRID (grid));

   GTK_DATABOX_GRID_GET_PRIVATE(grid)->vline_vals = vline_vals;

   g_object_notify (G_OBJECT (grid), "grid-vline-vals");
}

/**
 * gtk_databox_grid_get_vline_vals:
 * @grid: a #GtkDataboxGrid graph object
 *
 * Gets the pointer to the vertical line values for the @grid.
 *
 * Return value: Pointer to the vertical line values for the @grid. (or NULL if error)
 **/
gfloat*
gtk_databox_grid_get_vline_vals (GtkDataboxGrid * grid)
{
   g_return_val_if_fail (GTK_DATABOX_IS_GRID (grid), NULL);

   return GTK_DATABOX_GRID_GET_PRIVATE(grid)->vline_vals;
}

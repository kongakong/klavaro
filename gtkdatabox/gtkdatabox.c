/* $Id: gtkdatabox.c 4 2008-06-22 09:19:11Z rbock $ */
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

#include <gtkdatabox.h>
#include <gtkdatabox_marshal.h>
#include <gtkdatabox_ruler.h>
#include <gtk/gtk.h>
#include <math.h>


static gint gtk_databox_button_press (GtkWidget * widget,
                                      GdkEventButton * event);
static gint gtk_databox_scroll_event (GtkWidget *widget,
                                      GdkEventScroll *event);
static gint gtk_databox_button_release (GtkWidget * widget,
                                        GdkEventButton * event);
static gint gtk_databox_motion_notify (GtkWidget * widget,
                                       GdkEventMotion * event);
static void gtk_databox_realize (GtkWidget * widget);
static void gtk_databox_unrealize (GtkWidget * widget);
static void gtk_databox_size_allocate (GtkWidget * widget,
                                       GtkAllocation * allocation);
static gint gtk_databox_draw (GtkWidget * widget,
                                cairo_t * cr);
static void gtk_databox_set_property (GObject * object,
                                      guint prop_id,
                                      const GValue * value,
                                      GParamSpec * pspec);
static void gtk_databox_get_property (GObject * object,
                                      guint prop_id,
                                      GValue * value,
                                      GParamSpec * pspec);

static gfloat gtk_databox_get_offset_x (GtkDatabox* box);
static gfloat gtk_databox_get_page_size_x (GtkDatabox* box);
static gfloat gtk_databox_get_offset_y (GtkDatabox* box);
static gfloat gtk_databox_get_page_size_y (GtkDatabox* box);
static void gtk_databox_calculate_visible_limits (GtkDatabox * box);
static void gtk_databox_create_backing_surface (GtkDatabox * box);
static void gtk_databox_calculate_selection_values (GtkDatabox * box);
static void gtk_databox_selection_cancel (GtkDatabox * box);
static void gtk_databox_zoomed (GtkDatabox * box);
static void gtk_databox_draw_selection (GtkDatabox * box, gboolean clear);
static void gtk_databox_adjustment_value_changed (GtkDatabox * box);
static void gtk_databox_ruler_update (GtkDatabox * box);

/* IDs of signals */
enum {
    ZOOMED_SIGNAL,
    SELECTION_STARTED_SIGNAL,
    SELECTION_CHANGED_SIGNAL,
    SELECTION_FINALIZED_SIGNAL,
    SELECTION_CANCELED_SIGNAL,
    LAST_SIGNAL
};

/* signals will be configured during class_init */
static gint gtk_databox_signals[LAST_SIGNAL] = { 0 };


/* IDs of properties */
enum {
    ENABLE_SELECTION = 1,
    ENABLE_ZOOM,
    ADJUSTMENT_X,
    ADJUSTMENT_Y,
    RULER_X,
    RULER_Y,
    SCALE_TYPE_X,
    SCALE_TYPE_Y,
    BOX_SHADOW,
    LAST_PROPERTY
};

/**
 * GtkDataboxPrivate
 *
 * A private data structure used by the #GtkDatabox. It shields all internal things
 * from developers who are just using the widget.
 *
 **/
typedef struct _GtkDataboxPrivate GtkDataboxPrivate;

struct _GtkDataboxPrivate {
    cairo_surface_t *backing_surface;
    gint old_width;
    gint old_height;

    /* Total and visible limits (values, not pixels) */
    gfloat total_left;
    gfloat total_right;
    gfloat total_top;
    gfloat total_bottom;
    gfloat visible_left;
    gfloat visible_right;
    gfloat visible_top;
    gfloat visible_bottom;

    /* Translation information between values and pixels */
    GtkDataboxScaleType scale_type_x;
    GtkDataboxScaleType scale_type_y;
    gfloat translation_factor_x;
    gfloat translation_factor_y;

    /* Properties */
    gboolean enable_selection;
    gboolean enable_zoom;
    GtkAdjustment *adj_x;
    GtkAdjustment *adj_y;
    GtkDataboxRuler *ruler_x;
    GtkDataboxRuler *ruler_y;

    /* Other private stuff */
    GList *graphs;
    GdkPoint marked;
    GdkPoint select;
    GtkDataboxValueRectangle selectionValues;
    gfloat zoom_limit;

    /* flags */
    gboolean selection_active;
    gboolean selection_finalized;

    GtkShadowType box_shadow; /* The type of shadow drawn on the pixmap edge */
};

/**
 * gtk_databox_get_type
 *
 * Determines the #GType of the GtkDatabox widget type.
 *
 * Return value: The #GType of the GtkDatabox widget type.
 *
 */
G_DEFINE_TYPE (GtkDatabox, gtk_databox, GTK_TYPE_WIDGET)

static void
gtk_databox_class_init (GtkDataboxClass * class) {
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS (class);
    widget_class = (GtkWidgetClass *) class;

    gobject_class->set_property = gtk_databox_set_property;
    gobject_class->get_property = gtk_databox_get_property;

    widget_class->realize = gtk_databox_realize;
    widget_class->unrealize = gtk_databox_unrealize;
    widget_class->size_allocate = gtk_databox_size_allocate;
    widget_class->draw = gtk_databox_draw;
    widget_class->motion_notify_event = gtk_databox_motion_notify;
    widget_class->button_press_event = gtk_databox_button_press;
    widget_class->button_release_event = gtk_databox_button_release;
    widget_class->scroll_event = gtk_databox_scroll_event;

    /**
     * GtkDatabox:enable-selection:
     *
     * Defines whether the user can select
     * rectangular areas with the mouse (#TRUE) or not (#FALSE).
     *
     */
    g_object_class_install_property (gobject_class,
                                     ENABLE_SELECTION,
                                     g_param_spec_boolean ("enable-selection",
                                             "Enable Selection",
                                             "Enable selection of areas via mouse (TRUE/FALSE)",
                                             TRUE,       /* default value */
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

    /**
     * GtkDatabox:enable-zoom:
     *
     * Defines whether the user can use the mouse to zoom in or out (#TRUE) or not (#FALSE).
     *
     */
    g_object_class_install_property (gobject_class,
                                     ENABLE_ZOOM,
                                     g_param_spec_boolean ("enable-zoom",
                                             "Enable Zoom",
                                             "Enable zooming in or out via mouse click (TRUE/FALSE)",
                                             TRUE,       /* default value */
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

    /**
     * GtkDatabox:adjustment_x:
     *
     * The #GtkAdjustment associated with the horizontal scrollbar. The #GtkDatabox widget
     * creates a GtkAdjustment itself. Normally there is no need for you to create another
     * GtkAdjustment. You could simply use the one you get via gtk_databox_get_adjustment_x().
     *
     */
    g_object_class_install_property (gobject_class,
                                     ADJUSTMENT_X,
                                     g_param_spec_object ("adjustment-x",
                                             "Horizontal Adjustment",
                                             "GtkAdjustment for horizontal scrolling",
                                             GTK_TYPE_ADJUSTMENT,
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

    /**
     * GtkDatabox:adjustment_y:
     *
     * The #GtkAdjustment associated with the vertical scrollbar. The #GtkDatabox widget
     * creates a GtkAdjustment itself. Normally there is no need for you to create another
     * GtkAdjustment. You could simply use the one you get via gtk_databox_get_adjustment_y().
     *
     */
    g_object_class_install_property (gobject_class,
                                     ADJUSTMENT_Y,
                                     g_param_spec_object ("adjustment-y",
                                             "Vertical Adjustment",
                                             "GtkAdjustment for vertical scrolling",
                                             GTK_TYPE_ADJUSTMENT,
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

    /**
     * GtkDatabox:ruler_x:
     *
     * The horizontal %GtkDataboxRuler (or NULL).
     *
     */
    g_object_class_install_property (gobject_class,
                                     RULER_X,
                                     g_param_spec_object ("ruler-x",
                                             "Horizontal Ruler",
                                             "A horizontal GtkDataboxRuler or NULL",
                                             GTK_DATABOX_TYPE_RULER,
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

    /**
     * GtkDatabox:ruler_y:
     *
     * The vertical %GtkDataboxRuler (or NULL).
     *
     */
    g_object_class_install_property (gobject_class,
                                     RULER_Y,
                                     g_param_spec_object ("ruler-y",
                                             "Vertical Ruler",
                                             "A vertical GtkDataboxRuler or NULL",
                                             GTK_DATABOX_TYPE_RULER,
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

    /**
     * GtkDatabox:scale_type_x:
     *
     * The horizontal scale type (linear or lograrithmic).
     */
    g_object_class_install_property (gobject_class,
                                     SCALE_TYPE_X,
                                     g_param_spec_enum ("scale-type-x",
                                             "Horizontal scale type",
                                             "Horizontal scale type (linear or logarithmic)",
                                             gtk_databox_scale_type_get_type (),
                                             GTK_DATABOX_SCALE_LINEAR,
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

    /**
     * GtkDatabox:scale_type_y:
     *
     * The vertical scale type (linear or lograrithmic).
     */
    g_object_class_install_property (gobject_class,
                                     SCALE_TYPE_Y,
                                     g_param_spec_enum ("scale-type-y",
                                             "Vertical scale type",
                                             "Vertical scale type (linear or logarithmic)",
                                             gtk_databox_scale_type_get_type (),
                                             GTK_DATABOX_SCALE_LINEAR,
                                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE));


    g_object_class_install_property (gobject_class,
                                     BOX_SHADOW,
                                     g_param_spec_uint ("box-shadow",
                                             "Box Shadow",
                                             "Style of the box shadow: GTK_SHADOW_NONE, GTK_SHADOW_IN, GTK_SHADOW_OUT, GTK_SHADOW_ETCHED_IN, GTK_SHADOW_ETCHED_OUT",
                                             GTK_SHADOW_NONE,
                                             GTK_SHADOW_ETCHED_OUT,
                                             GTK_SHADOW_NONE,
                                             G_PARAM_READWRITE));

    /**
     * GtkDatabox::zoomed:
     * @box: The #GtkDatabox widget which zoomed in or out.
     *
     * This signal is emitted each time the zoom of the widget is changed, see for example
     * gtk_databox_zoom_to_selection(), gtk_databox_set_visible_limits().
     */
    gtk_databox_signals[ZOOMED_SIGNAL] =
        g_signal_new ("zoomed",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GtkDataboxClass, zoomed),
                      NULL,	/* accumulator */
                      NULL,	/* accumulator_data */
                      gtk_databox_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    /**
     * GtkDatabox::selection-started:
     * @box: The #GtkDatabox widget in which the selection has been started.
     * @selection_values: The corners of the selection rectangle.
     *
     * This signal is emitted when the mouse is firstmoved
     * with the left button pressed after the mouse-down (and the #GtkDatabox:enable-selection property
     * is set). The corners of the selection rectangle are stored in @selection_values.
     *
     * @see_also: #GtkDatabox::selection-changed
     */
    gtk_databox_signals[SELECTION_STARTED_SIGNAL] =
        g_signal_new ("selection-started",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GtkDataboxClass, selection_started),
                      NULL,	/* accumulator */
                      NULL,	/* accumulator_data */
                      gtk_databox_marshal_VOID__POINTER,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_POINTER);

    /**
     * GtkDatabox::selection-changed:
     * @box: The #GtkDatabox widget in which the selection was changed.
     * @selection_values: The corners of the selection rectangle.
     *
     * This signal is emitted when the mouse is moved
     * with the left button pressed (and the #GtkDatabox:enable-selection property
     * is set). The corners of the selection rectangle are stored in @selection_values.
     */
    gtk_databox_signals[SELECTION_CHANGED_SIGNAL] =
        g_signal_new ("selection-changed",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GtkDataboxClass, selection_changed),
                      NULL,	/* accumulator */
                      NULL,	/* accumulator_data */
                      gtk_databox_marshal_VOID__POINTER,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_POINTER);

    /**
      * GtkDatabox::selection-finalized:
      * @box: The #GtkDatabox widget in which the selection has been stopped.
      * @selection_values: The corners of the selection rectangle.
      *
      * This signal is emitted when the left mouse button
      * is released after a selection was started before.
      *
      * @see_also: #GtkDatabox::selection-changed
      */
    gtk_databox_signals[SELECTION_FINALIZED_SIGNAL] =
        g_signal_new ("selection-finalized",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GtkDataboxClass, selection_finalized),
                      NULL,	/* accumulator */
                      NULL,	/* accumulator_data */
                      gtk_databox_marshal_VOID__POINTER,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_POINTER);

    /**
      * GtkDatabox::selection-canceled:
      * @box: The #GtkDatabox widget which zoomed in or out.
      *
      * This signal is emitted after a right click outside
      * a selection rectangle.
      */
    gtk_databox_signals[SELECTION_CANCELED_SIGNAL] =
        g_signal_new ("selection-canceled",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GtkDataboxClass, selection_canceled),
                      NULL,	/* accumulator */
                      NULL,	/* accumulator_data */
                      gtk_databox_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    class->zoomed = NULL;
    class->selection_started = NULL;
    class->selection_changed = NULL;
    class->selection_finalized = NULL;
    class->selection_canceled = NULL;

    g_type_class_add_private (class, sizeof (GtkDataboxPrivate));
}

static void
gtk_databox_init (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    priv->backing_surface = NULL;
    priv->scale_type_x = GTK_DATABOX_SCALE_LINEAR;
    priv->scale_type_y = GTK_DATABOX_SCALE_LINEAR;
    priv->translation_factor_x = 0;
    priv->translation_factor_y = 0;
    priv->enable_selection = TRUE;
    priv->enable_zoom = TRUE;
    priv->ruler_x = NULL;
    priv->ruler_y = NULL;
    priv->graphs = NULL;
    priv->zoom_limit = 0.01;
    priv->selection_active = FALSE;
    priv->selection_finalized = FALSE;
    priv->box_shadow=GTK_SHADOW_NONE;

    /* gtk_databox_set_total_limits(box, -1., 1., 1., -1.); */
	priv->visible_left = priv->total_left = -1.0;
	priv->visible_right = priv->total_right = 1.0;
	priv->visible_top = priv->total_top = 1.0;
	priv->visible_bottom = priv->total_bottom = -1.0;
    gtk_databox_set_adjustment_x (box, NULL);
    gtk_databox_set_adjustment_y (box, NULL);
    /*gtk_databox_set_total_limits(box, -1., 1., 1., -1.);*/
	g_object_set(GTK_WIDGET(box), "expand", TRUE, NULL);
}

/**
 * gtk_databox_new
 *
 * Creates a new #GtkDatabox widget.
 *
 * Return value: The new #GtkDatabox widget.
 *
 */
GtkWidget *
gtk_databox_new (void) {
    return g_object_new (GTK_TYPE_DATABOX, NULL);
}

/**
 * gtk_databox_get_graphs:
 * @box: A #GtkDatabox widget
 *
 * Return a list of graphs that were previously added to @box
 *
 * Return value: A #GList that contains all graphs
 */
GList *
gtk_databox_get_graphs (GtkDatabox * box) 
{
    g_return_val_if_fail (GTK_IS_DATABOX (box), NULL);

    return GTK_DATABOX_GET_PRIVATE(box)->graphs;
}

static gint
gtk_databox_motion_notify (GtkWidget * widget, GdkEventMotion * event) {
    GtkDatabox *box = GTK_DATABOX (widget);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GdkModifierType state;
    gint x;
    gint y;

    if (event->is_hint) {
        gdk_window_get_device_position (gtk_widget_get_window(widget), event->device, &x, &y, &state);
    } else {
        state = event->state;
        x = event->x;
        y = event->y;
    }

    if (state & GDK_BUTTON1_MASK
            && priv->enable_selection && !priv->selection_finalized) {
        GdkRectangle rect;
        gint width;
        gint height;

        width = gdk_window_get_width(gtk_widget_get_window(widget));
        height = gdk_window_get_height(gtk_widget_get_window(widget));
        x = MAX (0, MIN (width - 1, x));
        y = MAX (0, MIN (height - 1, y));

        if (priv->selection_active) {
            /* Clear current selection from backing_surface */
            gtk_databox_draw_selection (box, TRUE);
        } else {
            priv->selection_active = TRUE;
            priv->marked.x = x;
            priv->marked.y = y;
            priv->select.x = x;
            priv->select.y = y;
            gtk_databox_calculate_selection_values (box);
            g_signal_emit (G_OBJECT (box),
                           gtk_databox_signals[SELECTION_STARTED_SIGNAL], 0,
                           &priv->selectionValues);
        }

        /* Determine the exposure rectangle (covering old selection and new) */
        rect.x = MIN (MIN (priv->marked.x, priv->select.x), x);
        rect.y = MIN (MIN (priv->marked.y, priv->select.y), y);
        rect.width = MAX (MAX (priv->marked.x, priv->select.x), x)
                     - rect.x + 1;
        rect.height = MAX (MAX (priv->marked.y, priv->select.y), y)
                      - rect.y + 1;

        priv->select.x = x;
        priv->select.y = y;

        /* Draw new selection */
        gtk_databox_draw_selection (box, FALSE);

        gtk_databox_calculate_selection_values (box);
        g_signal_emit (G_OBJECT (box),
                       gtk_databox_signals[SELECTION_CHANGED_SIGNAL],
                       0, &priv->selectionValues);
    }

    return FALSE;
}

static void
gtk_databox_set_property (GObject * object,
                          guint property_id,
                          const GValue * value, GParamSpec * pspec) {
    GtkDatabox *box = GTK_DATABOX (object);
    switch (property_id) {
    case ENABLE_SELECTION:
        gtk_databox_set_enable_selection (box, g_value_get_boolean (value));
        break;
    case ENABLE_ZOOM:
        gtk_databox_set_enable_zoom (box, g_value_get_boolean (value));
        break;
    case ADJUSTMENT_X:
        gtk_databox_set_adjustment_x (box, g_value_get_object (value));
        break;
    case ADJUSTMENT_Y:
        gtk_databox_set_adjustment_y (box, g_value_get_object (value));
        break;
    case RULER_X:
        gtk_databox_set_ruler_x (box, g_value_get_object (value));
        break;
    case RULER_Y:
        gtk_databox_set_ruler_y (box, g_value_get_object (value));
        break;
    case SCALE_TYPE_X:
        gtk_databox_set_scale_type_x (box, g_value_get_enum (value));
        break;
    case SCALE_TYPE_Y:
        gtk_databox_set_scale_type_y (box, g_value_get_enum (value));
        break;
    case BOX_SHADOW:
        gtk_databox_set_box_shadow (box, (GtkShadowType) g_value_get_uint (value));
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gtk_databox_get_property (GObject * object,
                          guint property_id,
                          GValue * value, GParamSpec * pspec) {
    GtkDatabox *box = GTK_DATABOX (object);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    switch (property_id) {
    case ENABLE_SELECTION:
        g_value_set_boolean (value, gtk_databox_get_enable_selection (box));
        break;
    case ENABLE_ZOOM:
        g_value_set_boolean (value, gtk_databox_get_enable_zoom (box));
        break;
    case ADJUSTMENT_X:
        g_value_set_object (value, G_OBJECT (gtk_databox_get_adjustment_x (box)));
        break;
    case ADJUSTMENT_Y:
        g_value_set_object (value, G_OBJECT (gtk_databox_get_adjustment_y (box)));
        break;
    case RULER_X:
        g_value_set_object (value, G_OBJECT (gtk_databox_get_ruler_x (box)));
        break;
    case RULER_Y:
        g_value_set_object (value, G_OBJECT (gtk_databox_get_ruler_y (box)));
        break;
    case SCALE_TYPE_X:
        g_value_set_enum (value, gtk_databox_get_scale_type_x (box));
        break;
    case SCALE_TYPE_Y:
        g_value_set_enum (value, gtk_databox_get_scale_type_y (box));
        break;
    case BOX_SHADOW:
        g_value_set_uint (value, priv->box_shadow);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
gtk_databox_realize (GtkWidget * widget) {
    GtkDatabox *box;
    GdkWindowAttr attributes;
    gint attributes_mask;
	GtkAllocation allocation;
	GtkStyleContext *stylecontext;

    box = GTK_DATABOX (widget);
    gtk_widget_set_realized(widget, TRUE);
	gtk_widget_get_allocation(widget, &allocation);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.event_mask = gtk_widget_get_events (widget);
    attributes.event_mask |= (GDK_EXPOSURE_MASK |
                              GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK |
                              GDK_POINTER_MOTION_HINT_MASK);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    gtk_widget_set_window(widget,
        gdk_window_new (gtk_widget_get_parent_window (widget), &attributes,
                        attributes_mask));
    gdk_window_set_user_data (gtk_widget_get_window(widget), box);

	stylecontext = gtk_widget_get_style_context(widget);

    gtk_style_context_add_class(stylecontext, GTK_STYLE_CLASS_BACKGROUND);

	gtk_style_context_set_background(stylecontext, gtk_widget_get_window(widget));

    gtk_databox_create_backing_surface (box);
}

static void
gtk_databox_unrealize (GtkWidget * widget) {
    GtkDatabox *box = GTK_DATABOX (widget);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    gtk_widget_set_realized(widget, FALSE);

    if (priv->backing_surface)
		cairo_surface_destroy (priv->backing_surface);
    priv->backing_surface=NULL;
    if (priv->adj_x)
        g_object_unref (priv->adj_x);
    priv->adj_x=NULL;
    if (priv->adj_y)
        g_object_unref (priv->adj_y);

    g_list_free (priv->graphs);
    priv->graphs=NULL;

    if (GTK_WIDGET_CLASS (gtk_databox_parent_class)->unrealize)
        (*GTK_WIDGET_CLASS (gtk_databox_parent_class)->unrealize) (widget);

}


/**
 * gtk_databox_set_enable_selection
 * @box: A #GtkDatabox widget
 * @enable: Whether selection via mouse is enabled or not.
 *
 * Setter function for the #GtkDatabox:enable-selection property.
 *
 */
void
gtk_databox_set_enable_selection (GtkDatabox * box, gboolean enable) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_if_fail (GTK_IS_DATABOX (box));

    priv->enable_selection = enable;
    if (priv->selection_active) {
        gtk_databox_selection_cancel (box);
    }

    g_object_notify (G_OBJECT (box), "enable-selection");
}

/**
 * gtk_databox_set_enable_zoom
 * @box: A #GtkDatabox widget
 * @enable: Whether zoom via mouse is enabled or not.
 *
 * Setter function for the #GtkDatabox:enable-zoom property.
 *
 */
void
gtk_databox_set_enable_zoom (GtkDatabox * box, gboolean enable) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_if_fail (GTK_IS_DATABOX (box));

    priv->enable_zoom = enable;

    g_object_notify (G_OBJECT (box), "enable-zoom");
}

/**
 * gtk_databox_set_adjustment_x
 * @box: A #GtkDatabox widget
 * @adj: A #GtkAdjustment object
 *
 * Setter function for the #GtkDatabox:adjustment-x property. Normally, it should not be
 * required to use this function, see property documentation.
 *
 */
void
gtk_databox_set_adjustment_x (GtkDatabox * box, GtkAdjustment * adj) {
	gdouble page_size_x;
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    g_return_if_fail (GTK_IS_DATABOX (box));

    if (!adj)
        adj = GTK_ADJUSTMENT(gtk_adjustment_new (0, 0, 0, 0, 0, 0));

    g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

    if (priv->adj_x) {
        /* @@@ Do we need to disconnect from the signals here? */
        g_object_unref (priv->adj_x);
        if (g_object_is_floating(G_OBJECT(priv->adj_x)))
            g_object_ref_sink (priv->adj_x);
    }

    priv->adj_x = adj;
    g_object_ref (priv->adj_x);

    /* We always scroll from 0 to 1.0 */
	if (priv->total_left != priv->total_right)
	{
		page_size_x = gtk_databox_get_page_size_x(box);
	} else {
		page_size_x = 1.0;
	}

	gtk_adjustment_configure(priv->adj_x,
		gtk_databox_get_offset_x (box), /* value */
		0.0, /* lower */
		1.0, /* upper */
		page_size_x / 20, /* step_increment */
		page_size_x * 0.9, /* page_increment */
		page_size_x); /* page_size */

    g_signal_connect_swapped (G_OBJECT (priv->adj_x), "value_changed",
                              G_CALLBACK
                              (gtk_databox_adjustment_value_changed), box);

    g_object_notify (G_OBJECT (box), "adjustment-x");
}

/**
 * gtk_databox_set_adjustment_y
 * @box: A #GtkDatabox widget
 * @adj: A #GtkAdjustment object
 *
 * Setter function for the #GtkDatabox:adjustment-y property. Normally, it should not be
 * required to use this function, see property documentation.
 *
 */
void
gtk_databox_set_adjustment_y (GtkDatabox * box, GtkAdjustment * adj) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    gdouble page_size_y;

    g_return_if_fail (GTK_IS_DATABOX (box));
    if (!adj)
        adj = GTK_ADJUSTMENT(gtk_adjustment_new (0, 0, 0, 0, 0, 0));

    g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

    if (priv->adj_y) {
        /* @@@ Do we need to disconnect from the signals here? */
        g_object_unref (priv->adj_y);
        if (g_object_is_floating(G_OBJECT(priv->adj_y)))
            g_object_ref_sink (priv->adj_y);
    }

    priv->adj_y = adj;
    g_object_ref (priv->adj_y);

    /* We always scroll from 0 to 1.0 */
	if (priv->total_top != priv->total_bottom)
	{
		page_size_y = gtk_databox_get_page_size_y(box);
	} else {
		page_size_y = 1.0;
	}

	gtk_adjustment_configure(priv->adj_y,
		gtk_databox_get_offset_y (box), /* value */
		0.0, /* lower */
		1.0, /* upper */
		page_size_y / 20, /* step_increment */
		page_size_y * 0.9, /* page_increment */
		page_size_y); /* page_size */

    g_signal_connect_swapped (G_OBJECT (priv->adj_y), "value_changed",
                              G_CALLBACK
                              (gtk_databox_adjustment_value_changed), box);

    g_object_notify (G_OBJECT (box), "adjustment-y");
}

/**
 * gtk_databox_set_ruler_x
 * @box: A #GtkDatabox widget
 * @ruler: A #GtkDataboxRuler object
 *
 * Setter function for the #GtkDatabox:ruler-x property.
 *
 */
void
gtk_databox_set_ruler_x (GtkDatabox * box, GtkDataboxRuler * ruler) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    g_return_if_fail (GTK_IS_DATABOX (box));
    g_return_if_fail (ruler == NULL || GTK_DATABOX_IS_RULER (ruler));
    g_return_if_fail (ruler == NULL || gtk_databox_ruler_get_orientation(ruler) == GTK_ORIENTATION_HORIZONTAL);

    if (priv->ruler_x) {
        /* @@@ Do we need to disconnect the signals here? */
        /* @@@ Do we need to call object_ref and object_unref here and for adjustments? */
    }

    priv->ruler_x = ruler;

    if (GTK_DATABOX_IS_RULER (ruler)) {
        gtk_databox_ruler_set_scale_type (ruler, priv->scale_type_x);

        gtk_databox_ruler_update (box);
        g_signal_connect_swapped (box, "motion_notify_event",
                                  G_CALLBACK (GTK_WIDGET_GET_CLASS
                                              (priv->ruler_x)->
                                              motion_notify_event),
                                  G_OBJECT (priv->ruler_x));
    }

    g_object_notify (G_OBJECT (box), "ruler-x");
}

/**
 * gtk_databox_set_ruler_y
 * @box: A #GtkDatabox widget
 * @ruler: An #GtkDataboxRuler object
 *
 * Setter function for the #GtkDatabox:ruler-y property.
 *
 */
void
gtk_databox_set_ruler_y (GtkDatabox * box, GtkDataboxRuler * ruler) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    g_return_if_fail (GTK_IS_DATABOX (box));
    g_return_if_fail (ruler == NULL || GTK_DATABOX_IS_RULER (ruler));
    g_return_if_fail (ruler == NULL || gtk_databox_ruler_get_orientation(ruler) == GTK_ORIENTATION_VERTICAL);

    if (priv->ruler_y) {
        /* @@@ Do we need to disconnect the signals here? */
        /* @@@ Do we need to call object_ref and object_unref here and for adjustments? */
    }

    priv->ruler_y = ruler;

    if (GTK_DATABOX_IS_RULER (ruler)) {
        gtk_databox_ruler_set_scale_type (ruler,
                                          priv->scale_type_y);

        gtk_databox_ruler_update (box);
        g_signal_connect_swapped (box, "motion_notify_event",
                                  G_CALLBACK (GTK_WIDGET_GET_CLASS
                                              (priv->ruler_y)->
                                              motion_notify_event),
                                  G_OBJECT (priv->ruler_y));
    }

    g_object_notify (G_OBJECT (box), "ruler-y");
}

/**
 * gtk_databox_set_scale_type_x
 * @box: A #GtkDatabox widget
 * @scale_type: An #GtkDataboxScaleType (linear or logarithmic)
 *
 * Setter function for the #GtkDatabox:scale-type-x property.
 *
 */
void
gtk_databox_set_scale_type_x (GtkDatabox * box,
                              GtkDataboxScaleType scale_type) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    priv->scale_type_x = scale_type;

    if (priv->ruler_x)
        gtk_databox_ruler_set_scale_type (priv->ruler_x, scale_type);

    g_object_notify (G_OBJECT (box), "scale-type-x");
}

/**
 * gtk_databox_set_scale_type_y
 * @box: A #GtkDatabox widget
 * @scale_type: An #GtkDataboxScaleType (linear or logarithmic)
 *
 * Setter function for the #GtkDatabox:scale-type-y property.
 *
 */
void
gtk_databox_set_scale_type_y (GtkDatabox * box,
                              GtkDataboxScaleType scale_type) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    priv->scale_type_y = scale_type;

    if (priv->ruler_y)
        gtk_databox_ruler_set_scale_type (priv->ruler_y, scale_type);

    g_object_notify (G_OBJECT (box), "scale-type-y");
}

/**
 * gtk_databox_set_box_shadow:
 * @box: a #GtkDatabox widget.
 * @which_shadow: How to render the box shadow on the GtkDatabox edges.
 *
 * Sets the shadow type when using gtk_paint_box. This will draw the desired edge shadow.
 **/
void
gtk_databox_set_box_shadow(GtkDatabox * box, GtkShadowType which_shadow) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    g_return_if_fail (GTK_IS_DATABOX (box));
    g_return_if_fail (which_shadow<=GTK_SHADOW_ETCHED_OUT);

    if (priv->box_shadow!=which_shadow) {
        priv->box_shadow=which_shadow;
        if (gtk_widget_is_drawable (GTK_WIDGET (box)))
            gtk_widget_queue_draw (GTK_WIDGET (box));
    }
}


/**
 * gtk_databox_get_enable_selection
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:enable-selection property.
 *
 * Return value: The #GtkDatabox:enable-selection property value.
 *
 */
gboolean
gtk_databox_get_enable_selection (GtkDatabox * box) {
    g_return_val_if_fail (GTK_IS_DATABOX (box), FALSE);

    return GTK_DATABOX_GET_PRIVATE(box)->enable_selection;
}

/**
 * gtk_databox_get_enable_zoom
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:enable-zoom property.
 *
 * Return value: The #GtkDatabox:enable-zoom property value.
 *
 */
gboolean
gtk_databox_get_enable_zoom (GtkDatabox * box) {
    g_return_val_if_fail (GTK_IS_DATABOX (box), FALSE);

    return GTK_DATABOX_GET_PRIVATE(box)->enable_zoom;
}

/**
 * gtk_databox_get_adjustment_x
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:adjustment-x property.
 *
 * Return value: The #GtkDatabox:adjustment-x property value.
 *
 */
GtkAdjustment *
gtk_databox_get_adjustment_x (GtkDatabox * box) {
    g_return_val_if_fail (GTK_IS_DATABOX (box), NULL);

    return GTK_DATABOX_GET_PRIVATE(box)->adj_x;
}

/**
 * gtk_databox_get_adjustment_y
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:adjustment-y property.
 *
 * Return value: The #GtkDatabox:adjustment-y property value.
 *
 */
GtkAdjustment *
gtk_databox_get_adjustment_y (GtkDatabox * box) {
    g_return_val_if_fail (GTK_IS_DATABOX (box), NULL);

    return GTK_DATABOX_GET_PRIVATE(box)->adj_y;
}

/**
 * gtk_databox_get_ruler_x
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:ruler-x property.
 *
 * Return value: The #GtkDatabox:ruler-x property value.
 *
 */
GtkDataboxRuler *
gtk_databox_get_ruler_x (GtkDatabox * box) {
    g_return_val_if_fail (GTK_IS_DATABOX (box), NULL);

    return GTK_DATABOX_GET_PRIVATE(box)->ruler_x;
}

/**
 * gtk_databox_get_ruler_y
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:ruler-y property.
 *
 * Return value: The #GtkDatabox:ruler-y property value.
 *
 */
GtkDataboxRuler *
gtk_databox_get_ruler_y (GtkDatabox * box) {
    g_return_val_if_fail (GTK_IS_DATABOX (box), NULL);

    return GTK_DATABOX_GET_PRIVATE(box)->ruler_y;
}

/**
 * gtk_databox_get_scale_type_x
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:scale-type-x property.
 *
 * Return value: The #GtkDatabox:scale-type-x property value.
 *
 */
GtkDataboxScaleType
gtk_databox_get_scale_type_x (GtkDatabox * box) {
    return GTK_DATABOX_GET_PRIVATE(box)->scale_type_x;
}

/**
 * gtk_databox_get_scale_type_y
 * @box: A #GtkDatabox widget.
 *
 * Getter function for the #GtkDatabox:scale-type-y property.
 *
 * Return value: The #GtkDatabox:scale-type-y property value.
 *
 */
GtkDataboxScaleType
gtk_databox_get_scale_type_y (GtkDatabox * box) {
    return GTK_DATABOX_GET_PRIVATE(box)->scale_type_y;
}

/**
 * gtk_databox_get_box_shadow:
 * @box: a #GtkDatabox widget
 *
 * Gets the type of shadow being rendered to the @box (GTK_SHADOW_NONE, GTK_SHADOW_IN, GTK_SHADOW_OUT, GTK_SHADOW_ETCHED_IN, GTK_SHADOW_ETCHED_OUT).
 *
 * Return value: The currently used shadow type of the @box, -1 on failure.
 **/
GtkShadowType
gtk_databox_get_box_shadow(GtkDatabox * box) {

    g_return_val_if_fail (GTK_IS_DATABOX (box), -1);

    return GTK_DATABOX_GET_PRIVATE(box)->box_shadow;
}

static void
gtk_databox_calculate_translation_factors (GtkDatabox * box) {
    /* @@@ Check for all external functions, if type checks are implemented! */
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GtkWidget *widget = GTK_WIDGET(box);
	GtkAllocation allocation;

	gtk_widget_get_allocation(widget, &allocation);
    if (priv->scale_type_x == GTK_DATABOX_SCALE_LINEAR)
        priv->translation_factor_x =
            (gfloat)allocation.width / (priv->visible_right -
                                        priv->visible_left);
    else if (priv->scale_type_x == GTK_DATABOX_SCALE_LOG2)
        priv->translation_factor_x =
            (gfloat)allocation.width / log2 (priv->visible_right /
                                             priv->visible_left);
    else
        priv->translation_factor_x =
            (gfloat)allocation.width / log10 (priv->visible_right /
                                              priv->visible_left);

    if (priv->scale_type_y == GTK_DATABOX_SCALE_LINEAR)
        priv->translation_factor_y =
            (gfloat)allocation.height / (priv->visible_bottom -
                                         priv->visible_top);
    else if (priv->scale_type_y == GTK_DATABOX_SCALE_LOG2)
        priv->translation_factor_y =
            (gfloat)allocation.height / log2 (priv->visible_bottom /
                                              priv->visible_top);
    else
        priv->translation_factor_y =
            (gfloat)allocation.height / log10 (priv->visible_bottom /
                                               priv->visible_top);
}

static void
gtk_databox_create_backing_surface(GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GtkAllocation allocation;
    GtkWidget *widget;
	cairo_t *cr;
    gint width;
    gint height;

    widget = GTK_WIDGET (box);
    gtk_widget_get_allocation(widget, &allocation);
    width = allocation.width;
    height = allocation.height;
    if (priv->backing_surface) {
       if ((width == priv->old_width) &&
          (height == priv->old_height))
          return;
       cairo_surface_destroy (priv->backing_surface);
   }

   priv->old_width = width;
   priv->old_height = height;

    cr = gdk_cairo_create (gtk_widget_get_window (widget));

   priv->backing_surface = cairo_surface_create_similar(
                                cairo_get_target (cr),
                                CAIRO_CONTENT_COLOR,
                                width, height);

}

/**
 * gtk_databox_get_backing_surface:
 * @box: A #GtkDatabox widget
 *
 * This function returns the surface which is used by @box and its #GtkDataboxGraph objects
 * for drawing operations before copying the result to the screen.
 *
 * The function is typically called by the #GtkDataboxGraph objects.
 *
 * Return value: Backing surface
 */
cairo_surface_t *
gtk_databox_get_backing_surface(GtkDatabox * box) {
    g_return_val_if_fail (GTK_IS_DATABOX (box), NULL);

    return GTK_DATABOX_GET_PRIVATE(box)->backing_surface;
}

static void
gtk_databox_size_allocate (GtkWidget * widget, GtkAllocation * allocation) {
    GtkDatabox *box = GTK_DATABOX (widget);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    gtk_widget_set_allocation(widget, allocation);

	if (gtk_widget_get_window(widget))	/* don't move_resize an unrealized window */
	{
		gdk_window_move_resize (gtk_widget_get_window(widget),
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);
	}

    if (priv->selection_active) {
        gtk_databox_selection_cancel (box);
    }

    gtk_databox_calculate_translation_factors (box);
}

static gint
gtk_databox_draw (GtkWidget * widget, cairo_t * cr) {
    GtkDatabox *box = GTK_DATABOX (widget);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GList *list;
    cairo_t *cr2;
    GtkStyleContext *stylecontext = gtk_widget_get_style_context(widget);
    GdkRGBA bg_color;

    gtk_databox_create_backing_surface (box);

    cr2 = cairo_create(priv->backing_surface);
    gtk_style_context_get_background_color(stylecontext, GTK_STATE_FLAG_NORMAL, &bg_color);
    gdk_cairo_set_source_rgba (cr2, &bg_color);
    cairo_paint(cr2);
    cairo_destroy(cr2);

    list = g_list_last (priv->graphs);
    while (list) {
        if (list->data)
            gtk_databox_graph_draw (GTK_DATABOX_GRAPH (list->data), box);
        list = g_list_previous (list);
    }

    if (priv->selection_active)
        gtk_databox_draw_selection (box, TRUE);

    cairo_set_source_surface (cr, priv->backing_surface, 0, 0);
    cairo_paint(cr);
	/* the following was removed - unsure if it creates problems */
    /*gtk_databox_draw_selection (box, FALSE);*/

    return FALSE;
}

static void
gtk_databox_calculate_selection_values (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    priv->selectionValues.x1 =
        gtk_databox_pixel_to_value_x (box, priv->marked.x);
    priv->selectionValues.x2 =
        gtk_databox_pixel_to_value_x (box, priv->select.x);
    priv->selectionValues.y1 =
        gtk_databox_pixel_to_value_y (box, priv->marked.y);
    priv->selectionValues.y2 =
        gtk_databox_pixel_to_value_y (box, priv->select.y);
}

static gint
gtk_databox_button_press (GtkWidget * widget, GdkEventButton * event) {
    GtkDatabox *box = GTK_DATABOX (widget);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(widget);

    if (event->type != GDK_BUTTON_PRESS && event->type != GDK_2BUTTON_PRESS)
        return FALSE;

    priv->selection_finalized = FALSE;
    if ((event->button == 1 || event->button == 2) & !(event->type==GDK_2BUTTON_PRESS)) {
        if (priv->selection_active) {
            if (event->x > MIN (priv->marked.x, priv->select.x)
                    && event->x < MAX (priv->marked.x, priv->select.x)
                    && event->y > MIN (priv->marked.y, priv->select.y)
                    && event->y < MAX (priv->marked.y, priv->select.y)) {
                gtk_databox_zoom_to_selection (box);
            } else {
                gtk_databox_selection_cancel (box);
            }
            priv->marked.x = priv->select.x = event->x;
            priv->marked.y = priv->select.y = event->y;
            gtk_databox_calculate_selection_values (box);
        }
    }

    if ((event->button == 3) || (event->button == 1 && event->type==GDK_2BUTTON_PRESS)) {
        if (event->state & GDK_SHIFT_MASK) {
            gtk_databox_zoom_home (box);
        } else {
            gtk_databox_zoom_out (box);
        }
    }

    return FALSE;
}

static gint
gtk_databox_button_release (GtkWidget * widget, GdkEventButton * event) {
    GtkDatabox *box = GTK_DATABOX (widget);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(widget);

    if (event->type != GDK_BUTTON_RELEASE)
        return FALSE;

    if (priv->selection_active) {
        priv->selection_finalized = TRUE;

        g_signal_emit (G_OBJECT (box),
                       gtk_databox_signals[SELECTION_FINALIZED_SIGNAL],
                       0, &priv->selectionValues);
    }

    return FALSE;
}

static gint
gtk_databox_scroll_event (GtkWidget *widget, GdkEventScroll *event) {
    GtkDatabox  *box = GTK_DATABOX (widget);
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(widget);

    if (event->state & GDK_CONTROL_MASK && priv->enable_zoom) {
        if (event->direction == GDK_SCROLL_DOWN) {
            gtk_databox_zoom_out(box);
        } else if (event->direction == GDK_SCROLL_UP &&
                   (gtk_adjustment_get_page_size(priv->adj_x) / 2) >= priv->zoom_limit &&
                   (gtk_adjustment_get_page_size(priv->adj_y) / 2) >= priv->zoom_limit) {
            gdouble       x_val, y_val;
            gdouble       x_proportion, y_proportion;

            x_val = gtk_databox_pixel_to_value_x(box, event->x);
            y_val = gtk_databox_pixel_to_value_y(box, event->y);

            if (priv->scale_type_x == GTK_DATABOX_SCALE_LINEAR) {
                x_proportion = (x_val - priv->total_left) /
                               (priv->total_right - priv->total_left);
            } else {
                x_proportion = log(x_val/priv->total_left) /
                               log(priv->total_right / priv->total_left);
            }

            if (priv->scale_type_y == GTK_DATABOX_SCALE_LINEAR) {
                y_proportion = (y_val - priv->total_top) /
                               (priv->total_bottom - priv->total_top);
            } else {
                y_proportion = log(y_val/priv->total_top) /
                               log(priv->total_bottom / priv->total_top);
            }

			g_object_freeze_notify(G_OBJECT(priv->adj_x));
            gtk_adjustment_set_page_size(priv->adj_x, gtk_adjustment_get_page_size(priv->adj_x) / 2);
            gtk_adjustment_set_value(priv->adj_x, (x_proportion +
                                       gtk_adjustment_get_value(priv->adj_x)) / 2);
			g_object_thaw_notify(G_OBJECT(priv->adj_x));

			g_object_freeze_notify(G_OBJECT(priv->adj_y));
            gtk_adjustment_set_page_size(priv->adj_y, gtk_adjustment_get_page_size(priv->adj_y) / 2);
            gtk_adjustment_set_value(priv->adj_y, (y_proportion +
                                       gtk_adjustment_get_value(priv->adj_y)) / 2);
			g_object_thaw_notify(G_OBJECT(priv->adj_y));

            gtk_databox_calculate_visible_limits(box);
            gtk_databox_zoomed (box);
        }
    } else {
        GtkAdjustment *adj;
        gdouble delta = 0.0, new_value;

        if ((event->direction == GDK_SCROLL_UP ||
                event->direction == GDK_SCROLL_DOWN) &&
                !(event->state & GDK_MOD1_MASK)) {
            adj = priv->adj_y;
        } else {
            adj = priv->adj_x;
        }

        switch (event->direction) {
        case GDK_SCROLL_UP:
        case GDK_SCROLL_SMOOTH:
        case GDK_SCROLL_LEFT:
            delta = 0 - gtk_adjustment_get_step_increment(adj);
            break;
        case GDK_SCROLL_DOWN:
        case GDK_SCROLL_RIGHT:
            delta = gtk_adjustment_get_step_increment(adj);
            break;
        }

        new_value = CLAMP (gtk_adjustment_get_value(adj) + delta, gtk_adjustment_get_lower(adj),
                           gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
        gtk_adjustment_set_value(adj, new_value);
    }

    return FALSE;
}

static void
gtk_databox_selection_cancel (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    /* There is no active selection after cancellation */
    priv->selection_active = FALSE;

    /* Only active selections can be stopped */
    priv->selection_finalized = FALSE;

    /* Remove selection box */
    gtk_databox_draw_selection (box, TRUE);

    /* Let everyone know that the selection has been canceled */
    g_signal_emit (G_OBJECT (box),
                   gtk_databox_signals[SELECTION_CANCELED_SIGNAL], 0);
}


static void
gtk_databox_zoomed (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    g_return_if_fail(GTK_IS_DATABOX(box));
    g_return_if_fail(GTK_IS_ADJUSTMENT(priv->adj_x));
    g_return_if_fail(GTK_IS_ADJUSTMENT(priv->adj_y));

    priv->selection_active = FALSE;
    priv->selection_finalized = FALSE;

    gtk_adjustment_changed (priv->adj_x);
    gtk_adjustment_changed (priv->adj_y);

    gtk_widget_queue_draw (GTK_WIDGET(box));

    g_signal_emit (G_OBJECT (box),
                   gtk_databox_signals[ZOOMED_SIGNAL], 0, NULL);
}

/**
 * gtk_databox_zoom_to_selection:
 * @box: A #GtkDatabox widget
 *
 * This is equivalent to left-clicking into the selected area.
 *
 * This function works, if the attribute #enable-zoom is set to #TRUE. Calling the function
 * then zooms to the area selected with the mouse.
 *
 * Side effect: The @box emits #GtkDatabox::zoomed.
 */
void
gtk_databox_zoom_to_selection (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GtkWidget *widget;
	GtkAllocation allocation;
	gdouble temp_value, temp_page_size;

    g_return_if_fail(GTK_IS_DATABOX(box));

    widget = GTK_WIDGET (box);
	gtk_widget_get_allocation(widget, &allocation);

    if (!priv->enable_zoom) {
        gtk_databox_selection_cancel (box);
        return;
    }

	g_object_freeze_notify(G_OBJECT(priv->adj_x));
	g_object_freeze_notify(G_OBJECT(priv->adj_y));

	temp_value = gtk_adjustment_get_value(priv->adj_x);
    temp_value += (gdouble) (MIN (priv->marked.x, priv->select.x))
                               * gtk_adjustment_get_page_size(priv->adj_x)
                               / allocation.width;
	temp_page_size = gtk_adjustment_get_page_size(priv->adj_x);
    temp_page_size *=
        (gdouble) (ABS (priv->marked.x - priv->select.x) + 1)
        / allocation.width;

	gtk_adjustment_set_page_size(priv->adj_x, temp_page_size);
	gtk_adjustment_set_value(priv->adj_x, temp_value);

	temp_value = gtk_adjustment_get_value(priv->adj_y);
    temp_value += (gdouble) (MIN (priv->marked.y, priv->select.y))
                               * gtk_adjustment_get_page_size(priv->adj_y)
                               / allocation.height;
	temp_page_size = gtk_adjustment_get_page_size(priv->adj_y);
    temp_page_size *=
        (gfloat) (ABS (priv->marked.y - priv->select.y) + 1)
        / allocation.height;

	gtk_adjustment_set_page_size(priv->adj_y, temp_page_size);
	gtk_adjustment_set_value(priv->adj_y, temp_value);

    /* If we zoom too far into the data, we will get funny results, because
     * of overflow effects. Therefore zooming is limited to zoom_limit.
     */
    if (gtk_adjustment_get_page_size(priv->adj_x) < priv->zoom_limit) {
        temp_value = (gfloat) MAX (0, gtk_adjustment_get_value(priv->adj_x)
							- (priv->zoom_limit -
							gtk_adjustment_get_page_size(priv->adj_x)) /
							2.0);
		gtk_adjustment_set_page_size(priv->adj_x, priv->zoom_limit);
		gtk_adjustment_set_value(priv->adj_x, temp_value);
    }

    if (gtk_adjustment_get_page_size(priv->adj_y) < priv->zoom_limit) {
        temp_value = (gfloat) MAX (0, gtk_adjustment_get_value(priv->adj_y)
							- (priv->zoom_limit -
							gtk_adjustment_get_page_size(priv->adj_y)) /
							2.0);
		gtk_adjustment_set_page_size(priv->adj_y, priv->zoom_limit);
		gtk_adjustment_set_value(priv->adj_y, temp_value);
    }
	g_object_thaw_notify(G_OBJECT(priv->adj_y));
	g_object_thaw_notify(G_OBJECT(priv->adj_x));

    gtk_databox_calculate_visible_limits(box);
    gtk_databox_zoomed (box);
}

/**
 * gtk_databox_zoom_out:
 * @box: A #GtkDatabox widget
 *
 * This is equivalent to right-clicking into the @box.
 *
 * This function works, if the attribute #enable-zoom is set to #TRUE. Calling the function
 * then zooms out by a factor of 2 in both dimensions (the maximum is defined by the total
 * limits, see gtk_databox_set_total_limits()).
 *
 * Side effect: The @box emits #GtkDatabox::zoomed.
 */
void
gtk_databox_zoom_out (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    if (!priv->enable_zoom) {
        return;
    }

	g_object_freeze_notify(G_OBJECT(priv->adj_x));
	g_object_freeze_notify(G_OBJECT(priv->adj_y));
    gtk_adjustment_set_page_size(priv->adj_x, MIN (1.0, gtk_adjustment_get_page_size(priv->adj_x) * 2));
    gtk_adjustment_set_page_size(priv->adj_y, MIN (1.0, gtk_adjustment_get_page_size(priv->adj_y) * 2));
    gtk_adjustment_set_value(priv->adj_x,
        (gtk_adjustment_get_page_size(priv->adj_x) == 1.0)
        ? 0
        : MAX (0, MIN (gtk_adjustment_get_value(priv->adj_x) - gtk_adjustment_get_page_size(priv->adj_x) / 4,
                       1.0 - gtk_adjustment_get_page_size(priv->adj_x))));
    gtk_adjustment_set_value(priv->adj_y,
        (gtk_adjustment_get_page_size(priv->adj_y) == 1.0)
        ? 0
        : MAX (0, MIN (gtk_adjustment_get_value(priv->adj_y) - gtk_adjustment_get_page_size(priv->adj_y) / 4,
                       1.0 - gtk_adjustment_get_page_size(priv->adj_y))));
	g_object_thaw_notify(G_OBJECT(priv->adj_y));
	g_object_thaw_notify(G_OBJECT(priv->adj_x));

    gtk_databox_calculate_visible_limits(box);
    gtk_databox_zoomed (box);
}

/**
 * gtk_databox_zoom_home:
 * @box: A #GtkDatabox widget
 *
 * This is equivalent to shift right-clicking into the @box.
 *
 * This function works, if the attribute #enable-zoom is set to #TRUE. It is equivalent to
 * calling the gtk_databox_set_visible_limits() with the total limits.
 *
 */
void
gtk_databox_zoom_home (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    if (!priv->enable_zoom) {
        return;
    }

    gtk_databox_set_visible_limits (box,
                                    priv->total_left, priv->total_right,
                                    priv->total_top, priv->total_bottom);
}

static void
gtk_databox_draw_selection (GtkDatabox * box, gboolean clear) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GtkWidget *widget = GTK_WIDGET (box);
	cairo_t *cr;

	cr = gdk_cairo_create (gtk_widget_get_window (widget));
    cairo_rectangle (cr,
                     MIN (priv->marked.x, priv->select.x) - 0.5,
                     MIN (priv->marked.y, priv->select.y) - 0.5,
                     ABS (priv->marked.x - priv->select.x) + 1.0,
                     ABS (priv->marked.y - priv->select.y) + 1.0);

   if (clear) {
      cairo_set_source_surface (cr, priv->backing_surface, 0, 0);
      cairo_paint(cr);
      cairo_set_line_width (cr, 2.0);
    } else {
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
      cairo_set_line_width (cr, 1.0);
	}
	cairo_stroke(cr);

	cairo_destroy(cr);
}

static void
gtk_databox_adjustment_value_changed (GtkDatabox * box) {
    gtk_databox_calculate_visible_limits (box);

    gtk_widget_queue_draw (GTK_WIDGET(box));
}

static void
gtk_databox_ruler_update (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    if (priv->ruler_x) {
        gtk_databox_ruler_set_range (
            GTK_DATABOX_RULER (priv->ruler_x),
            priv->visible_left,
            priv->visible_right,
            0.5 * (priv->visible_left + priv->visible_right));
    }

    if (priv->ruler_y) {
        gtk_databox_ruler_set_range (
            GTK_DATABOX_RULER (priv->ruler_y),
            priv->visible_top,
            priv->visible_bottom,
            0.5 * (priv->visible_top + priv->visible_bottom));
    }
}

/**
 * gtk_databox_auto_rescale:
 * @box: A #GtkDatabox widget
 * @border: Relative border width (e.g. 0.1 means that the border on each side is 10% of the data area).
 *
 * This function is similar to gtk_databox_set_total_limits(). It sets the total limits
 * to match the data extrema (see gtk_databox_calculate_extrema()). If you do not like data pixels exactly at the
 * widget's border, you can add modify the limits using the border parameter: The limits are extended by
 * @border*(max-min) if max!=min. If max==min, they are extended by @border*max (otherwise the data could not be
 * scaled to the pixel realm).
 *
 * After calling this function, x values grow from left to right, y values grow from bottom to top.
 *
 * Return value: 0 on success,
 *          -1 if @box is no GtkDatabox widget,
 *          -2 if no datasets are available
 */
gint
gtk_databox_auto_rescale (GtkDatabox * box, gfloat border) {
    gfloat min_x;
    gfloat max_x;
    gfloat min_y;
    gfloat max_y;
    gint extrema_success = gtk_databox_calculate_extrema (box, &min_x, &max_x,
                           &min_y, &max_y);
    if (extrema_success)
        return extrema_success;
    else {
        gfloat width = max_x - min_x;
        gfloat height = max_y - min_y;

        if (width == 0) width = max_x;
        if (height == 0) height = max_y;

        min_x -= border * width;
        max_x += border * width;
        min_y -= border * height;
        max_y += border * height;
    }

    gtk_databox_set_total_limits (GTK_DATABOX (box), min_x, max_x, max_y,
                                  min_y);

    return 0;
}


/**
 * gtk_databox_calculate_extrema:
 * @box: A #GtkDatabox widget
 * @min_x: Will be filled with the lowest x value of all datasets
 * @max_x: Will be filled with the highest x value of all datasets
 * @min_y: Will be filled with the lowest y value of all datasets
 * @max_y: Will be filled with the highest y value of all datasets
 *
 * Determines the minimum and maximum x and y values of all
 * #GtkDataboxGraph objects which have been added to the #GtkDatabox widget via gtk_databox_graph_add().
 *
 * Return value: 0 on success,
 *          -1 if @box is no GtkDatabox widget,
 *          -2 if no datasets are available
 */
gint
gtk_databox_calculate_extrema (GtkDatabox * box,
                               gfloat * min_x, gfloat * max_x, gfloat * min_y,
                               gfloat * max_y) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GList *list;
    gint return_val = -2;
    gboolean first = TRUE;

    g_return_val_if_fail (GTK_IS_DATABOX (box), -1);

    list = g_list_last (priv->graphs);
    while (list) {
        gfloat graph_min_x;
        gfloat graph_max_x;
        gfloat graph_min_y;
        gfloat graph_max_y;
        gint value = -1;

        if (list->data) {
            value =
                gtk_databox_graph_calculate_extrema (GTK_DATABOX_GRAPH
                        (list->data), &graph_min_x,
                        &graph_max_x, &graph_min_y,
                        &graph_max_y);
        } else {
            /* Do nothing if data == NULL */
        }

        if (value >= 0) {
            return_val = 0;

            if (first) {
                /* The min and max values need to be initialized with the
                 * first valid values from the graph
                 */
                *min_x = graph_min_x;
                *max_x = graph_max_x;
                *min_y = graph_min_y;
                *max_y = graph_max_y;

                first = FALSE;
            } else {
                *min_x = MIN (*min_x, graph_min_x);
                *min_y = MIN (*min_y, graph_min_y);
                *max_x = MAX (*max_x, graph_max_x);
                *max_y = MAX (*max_y, graph_max_y);
            }
        }

        list = g_list_previous (list);
    }
    return return_val;
}

static gfloat
gtk_databox_get_offset_x (GtkDatabox* box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_x == GTK_DATABOX_SCALE_LINEAR)
        return (priv->visible_left - priv->total_left)
               / (priv->total_right - priv->total_left);
    else if (priv->scale_type_x == GTK_DATABOX_SCALE_LOG2)
        return log2 (priv->visible_left / priv->total_left)
               / log2 (priv->total_right / priv->total_left);
    else
        return log10 (priv->visible_left / priv->total_left)
               / log10 (priv->total_right / priv->total_left);
}

static gfloat
gtk_databox_get_page_size_x (GtkDatabox* box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_x == GTK_DATABOX_SCALE_LINEAR)
        return (priv->visible_left - priv->visible_right)
               / (priv->total_left - priv->total_right);
    else if (priv->scale_type_x == GTK_DATABOX_SCALE_LOG2)
        return log2 (priv->visible_left / priv->visible_right)
               / log2 (priv->total_left / priv->total_right);
    else
        return log10 (priv->visible_left / priv->visible_right)
               / log10 (priv->total_left / priv->total_right);
}

static gfloat
gtk_databox_get_offset_y (GtkDatabox* box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_y == GTK_DATABOX_SCALE_LINEAR)
        return (priv->visible_top - priv->total_top)
               / (priv->total_bottom - priv->total_top);
    else if (priv->scale_type_y == GTK_DATABOX_SCALE_LOG2)
        return log2 (priv->visible_top / priv->total_top)
               / log2 (priv->total_bottom / priv->total_top);
    else
        return log10 (priv->visible_top / priv->total_top)
               / log10 (priv->total_bottom / priv->total_top);
}

static gfloat
gtk_databox_get_page_size_y (GtkDatabox* box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_y == GTK_DATABOX_SCALE_LINEAR)
        return (priv->visible_top - priv->visible_bottom)
               / (priv->total_top - priv->total_bottom);
    else if (priv->scale_type_y == GTK_DATABOX_SCALE_LOG2)
        return log2 (priv->visible_top / priv->visible_bottom)
               / log2 (priv->total_top / priv->total_bottom);
    else
        return log10 (priv->visible_top / priv->visible_bottom)
               / log10 (priv->total_top / priv->total_bottom);
}

/**
 * gtk_databox_set_total_limits:
 * @box: A #GtkDatabox widget
 * @left: Left total limit
 * @right: Right total limit
 * @top: Top total limit
 * @bottom: Bottom total limit
 *
 * This function is used to set the limits of the total
 * display area of @box.
 * This function can be used to invert the orientation of the displayed graphs,
 * e.g. @top=-1000 and  @bottom=0.
 *
 * Side effect: The @box also internally calls gtk_databox_set_visible_limits() with the same values.
 *
 */
void
gtk_databox_set_total_limits (GtkDatabox * box,
                              gfloat left, gfloat right,
                              gfloat top, gfloat bottom) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_if_fail (GTK_IS_DATABOX (box));
    g_return_if_fail (left != right);
    g_return_if_fail (top != bottom);

    priv->total_left = left;
    priv->total_right = right;
    priv->total_top = top;
    priv->total_bottom = bottom;

    gtk_databox_set_visible_limits(box, left, right, top, bottom);
}

/**
 * gtk_databox_set_visible_limits:
 * @box: A #GtkDatabox widget
 * @left: Left visible limit
 * @right: Right visible limit
 * @top: Top visible limit
 * @bottom: Bottom visible limit
 *
 * This function is used to set the limits of the visible
 * display area of @box. The visible display area can be section of the total
 * area, i.e. the @box zooms in, showing only a part of the complete picture.
 *
 * The orientation of the values have to be the same as in gtk_databox_set_total_limits() and
 * the visible limits have to be within the total limits. The
 * values will not be used otherwise.
 *
 * Side effect: The @box emits #GtkDatabox::zoomed.
 *
 */
void
gtk_databox_set_visible_limits (GtkDatabox * box,
                                gfloat left, gfloat right,
                                gfloat top, gfloat bottom) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    gboolean visible_inside_total = FALSE;

    g_return_if_fail (GTK_IS_DATABOX (box));

    visible_inside_total =
        ((priv->total_left <= left && left < right
          && right <= priv->total_right)
         || (priv->total_left >= left && left > right
             && right >= priv->total_right))
        &&
        ((priv->total_bottom <= bottom && bottom < top
          && top <= priv->total_top)
         || (priv->total_bottom >= bottom && bottom > top
             && top >= priv->total_top));

    g_return_if_fail (visible_inside_total);

    priv->visible_left = left;
    priv->visible_right = right;
    priv->visible_top = top;
    priv->visible_bottom = bottom;

    gtk_databox_calculate_translation_factors (box);

	g_object_freeze_notify(G_OBJECT(priv->adj_x));
	g_object_freeze_notify(G_OBJECT(priv->adj_y));

    gtk_adjustment_set_value(priv->adj_x, gtk_databox_get_offset_x (box));
    gtk_adjustment_set_page_size(priv->adj_x, gtk_databox_get_page_size_x (box));
    gtk_adjustment_set_value(priv->adj_y, gtk_databox_get_offset_y (box));
    gtk_adjustment_set_page_size(priv->adj_y, gtk_databox_get_page_size_y (box));

	g_object_thaw_notify(G_OBJECT(priv->adj_y));
	g_object_thaw_notify(G_OBJECT(priv->adj_x));

    /* Update rulers */
    gtk_databox_ruler_update(box);

    gtk_databox_calculate_translation_factors (box);

    gtk_databox_zoomed (box);
}

/**
 * gtk_databox_calculate_visible_limits:
 * @box: A #GtkDatabox widget
 *
 * Calculates the visible limits based on the adjustment values and page sizes
 * and calls gtk_databox_set_visible_limits();
 */
static void
gtk_databox_calculate_visible_limits (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    if (!gtk_widget_get_visible (GTK_WIDGET(box)))
        return;

    if (priv->scale_type_x == GTK_DATABOX_SCALE_LINEAR) {
        priv->visible_left =
            priv->total_left
            + (priv->total_right - priv->total_left)
            * gtk_adjustment_get_value(priv->adj_x);
        priv->visible_right =
            priv->total_left
            + (priv->total_right - priv->total_left)
            * (gtk_adjustment_get_value(priv->adj_x) + gtk_adjustment_get_page_size(priv->adj_x));
    } else {
        priv->visible_left =
            priv->total_left
            * pow (priv->total_right / priv->total_left,
                   gtk_adjustment_get_value(priv->adj_x));
        priv->visible_right =
            priv->total_left
            * pow (priv->total_right / priv->total_left,
                   gtk_adjustment_get_value(priv->adj_x) + gtk_adjustment_get_page_size(priv->adj_x));
    }

    if (priv->scale_type_y == GTK_DATABOX_SCALE_LINEAR) {
        priv->visible_top =
            priv->total_top
            + (priv->total_bottom - priv->total_top)
            * gtk_adjustment_get_value(priv->adj_y);
        priv->visible_bottom =
            priv->total_top
            + (priv->total_bottom - priv->total_top)
            * (gtk_adjustment_get_value(priv->adj_y) + gtk_adjustment_get_page_size(priv->adj_y));
    } else {
        priv->visible_top =
            priv->total_top
            * pow (priv->total_bottom / priv->total_top,
                   gtk_adjustment_get_value(priv->adj_y)),
            priv->visible_bottom =
                priv->total_top
                * pow (priv->total_bottom / priv->total_top,
                       gtk_adjustment_get_value(priv->adj_y) + gtk_adjustment_get_page_size(priv->adj_y));
    }

    /* Adjustments are the basis for the calculations in this function
     * so they do not need to be updated
     */

    /* Update rulers */
    gtk_databox_ruler_update(box);

    gtk_databox_calculate_translation_factors (box);
}

/**
 * gtk_databox_get_total_limits:
 * @box: A #GtkDatabox widget
 * @left: Space for total left value or #NULL
 * @right: Space for total right value or #NULL
 * @top: Space for total top value or #NULL
 * @bottom: Space for total bottom value or #NULL
 *
 * Gives the total limits (as set by gtk_databox_auto_rescale() or gtk_databox_set_total_limits()).
 */
void
gtk_databox_get_total_limits (GtkDatabox * box,
                              gfloat * left, gfloat * right,
                              gfloat * top, gfloat * bottom) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_if_fail (GTK_IS_DATABOX (box));

    if (left)
        *left = priv->total_left;
    if (right)
        *right = priv->total_right;
    if (top)
        *top = priv->total_top;
    if (bottom)
        *bottom = priv->total_bottom;
}

/**
 * gtk_databox_get_visible_limits:
 * @box: A #GtkDatabox widget
 * @left: Space for visible left value or #NULL
 * @right: Space for visible right value or #NULL
 * @top: Space for visible top value or #NULL
 * @bottom: Space for visible bottom value or #NULL
 *
 * Gives the current visible limits. These differ from those given by gtk_databox_get_total_limits() if
 * you zoomed into the data for instance by gtk_databox_zoom_to_selection() or gtk_databox_set_visible_limits() (these values
 * can be changed by scrolling, of course).
 */
void
gtk_databox_get_visible_limits (GtkDatabox * box,
                                gfloat * left, gfloat * right,
                                gfloat * top, gfloat * bottom) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_if_fail (GTK_IS_DATABOX (box));

    if (left)
        *left = priv->visible_left;
    if (right)
        *right = priv->visible_right;
    if (top)
        *top = priv->visible_top;
    if (bottom)
        *bottom = priv->visible_bottom;
}


/**
 * gtk_databox_graph_add:
 * @box: A #GtkDatabox widget
 * @graph: A graph, e.g. a #GtkDataboxPoints or a #GtkDataboxGrid object
 *
 * Adds the @graph to the @box. The next time the @box is re-drawn, the graph will be shown.
 *
 * It might be becessary to modify the total_limits in order for the graph to be displayed properly (see gtk_databox_set_total_limits()).
 *
 * Return value: 0 on success, -1 otherwise
 */
gint
gtk_databox_graph_add (GtkDatabox * box, GtkDataboxGraph * graph) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_val_if_fail (GTK_IS_DATABOX (box), -1);
    g_return_val_if_fail (GTK_DATABOX_IS_GRAPH (graph), -1);

        priv->graphs = g_list_append (priv->graphs, graph);

    return (priv->graphs == NULL) ? -1 : 0;
}

/**
 * gtk_databox_graph_add_front:
 * @box: A #GtkDatabox widget
 * @graph: A graph, e.g. a #GtkDataboxPoints or a #GtkDataboxGrid object
 *
 * Adds the @graph to the @box and will be plotted on top. The next time the @box is re-drawn, the graph will be shown.
 *
 * It might be becessary to modify the total_limits in order for the graph to be displayed properly (see gtk_databox_set_total_limits()).
 *
 * Return value: 0 on success, -1 otherwise
 */
gint
gtk_databox_graph_add_front (GtkDatabox * box, GtkDataboxGraph * graph) {

    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_val_if_fail (GTK_IS_DATABOX (box), -1);
    g_return_val_if_fail (GTK_DATABOX_IS_GRAPH (graph), -1);

            priv->graphs = g_list_prepend (priv->graphs, graph);

    return (priv->graphs == NULL) ? -1 : 0;
}

/**
 * gtk_databox_graph_remove:
 * @box: A #GtkDatabox widget
 * @graph: A graph, e.g. a #GtkDataboxPoints or a #GtkDataboxGrid object
 *
 * Removes the @graph from the @box once. The next time the @box is re-drawn, the graph will not be shown (unless it was added more
 * than once).
 *
 * Return value: 0 on success, -1 otherwise
 */
gint
gtk_databox_graph_remove (GtkDatabox * box, GtkDataboxGraph * graph) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    GList *list;

    g_return_val_if_fail (GTK_IS_DATABOX (box), -1);
    g_return_val_if_fail (GTK_DATABOX_IS_GRAPH (graph), -1);

    list = g_list_find (priv->graphs, graph);
    g_return_val_if_fail (list, -1);

    priv->graphs = g_list_delete_link (priv->graphs, list);
    return 0;
}

/**
 * gtk_databox_graph_remove_all:
 * @box: A #GtkDatabox widget
 *
 * Removes all graphs from the @box. The next time the @box is re-drawn, no graphs will be shown.
 *
 * Return value: 0 on success, -1 otherwise
 */
gint
gtk_databox_graph_remove_all (GtkDatabox * box) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    g_return_val_if_fail (GTK_IS_DATABOX (box), -1);

    g_list_free (priv->graphs);
    priv->graphs = NULL;

    return 0;
}

/**
 * gtk_databox_value_to_pixel_x:
 * @box: A #GtkDatabox widget
 * @value: An x value
 *
 * Calculates the horizontal pixel coordinate which represents the x @value.
 * Pixel coordinates are relative to the top-left corner of the @box which is equivalent to (0,0).
 *
 * Return value: Pixel coordinate
 */
gint16
gtk_databox_value_to_pixel_x (GtkDatabox * box, gfloat value) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_x == GTK_DATABOX_SCALE_LINEAR)
        return (value -
                priv->visible_left) *
               priv->translation_factor_x;
    else if (priv->scale_type_x == GTK_DATABOX_SCALE_LOG2)
        return log2 (value / priv->visible_left) *
               priv->translation_factor_x;
    else
        return log10 (value / priv->visible_left) *
               priv->translation_factor_x;
}

/**
 * gtk_databox_value_to_pixel_y:
 * @box: A #GtkDatabox widget
 * @value: A y value
 *
 * Calculates the vertical pixel coordinate which represents the y @value.
 * Pixel coordinates are relative to the top-left corner of the @box which is equivalent to (0,0).
 *
 * Return value: Pixel coordinate
 */
gint16
gtk_databox_value_to_pixel_y (GtkDatabox * box, gfloat value) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_y == GTK_DATABOX_SCALE_LINEAR)
        return (value -
                priv->visible_top) * priv->translation_factor_y;
    else if (priv->scale_type_y == GTK_DATABOX_SCALE_LOG2)
        return log2 (value / priv->visible_top) *
               priv->translation_factor_y;
    else
        return log10 (value / priv->visible_top) *
               priv->translation_factor_y;
}

/**
 * gtk_databox_values_to_xpixels:
 * @box: A #GtkDatabox widget
 * @value: An x values array
 *
 * Calculates the horizontal pixel coordinates which represents the x @values.
 * Pixel coordinates are relative to the left corner of the @box which is equivalent to (0).
 *
 * Return value: Pixel coordinates
 */
void
gtk_databox_values_to_xpixels (GtkDatabox *box, gint16 *pixels,
	void *values, GType vtype, guint maxlen, guint start, guint stride, guint len)
{
	GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    guint i, indx;
	gfloat fval = 0.0;
	GtkDataboxScaleType scale_type;
	gfloat tf, minvis;

	scale_type = priv->scale_type_x;
	tf = priv->translation_factor_x;
	minvis = priv->visible_left;

	indx = start * stride;
	i = 0;
	do {
		/* This may be excessive, but it handles every conceivable type */
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

		if (scale_type == GTK_DATABOX_SCALE_LINEAR)
			pixels[i] = tf * (fval - minvis);
		else if (scale_type == GTK_DATABOX_SCALE_LOG2)
			pixels[i] = tf * log2(fval / minvis);
		else
			pixels[i] = tf * log10(fval / minvis);

		/* handle the wrap-around (ring buffer) issue using modulus.  for efficiency, don't do this for non-wraparound cases. */
		/* note this allows multiple wrap-arounds.  One could hold a single cycle of a sine wave, and plot a continuous wave */
		/* This can be optimized using pointers later */
		if (i + start > maxlen)
			indx = ((i + start) % maxlen) * stride;
		else
			indx += stride;
	} while (++i < len);
}

/**
 * gtk_databox_values_to_ypixels:
 * @box: A #GtkDatabox widget
 * @value: An y values array
 *
 * Calculates the vertical pixel coordinates which represents the y @values.
 * Pixel coordinates are relative to the top corner of the @box which is equivalent to (0).
 *
 * Return value: Pixel coordinates
 */
void
gtk_databox_values_to_ypixels (GtkDatabox *box, gint16 *pixels,
	void *values, GType vtype, guint maxlen, guint start, guint stride, guint len)
{
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);
    guint i, indx;
	gfloat fval = 0.0;
	GtkDataboxScaleType scale_type;
	gfloat tf, minvis;

	scale_type = priv->scale_type_y;
	tf = priv->translation_factor_y;
	minvis = priv->visible_top;

	indx = start * stride;
	i = 0;
	do {
		/* This may be excessive, but it handles every conceivable type */
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

		if (scale_type == GTK_DATABOX_SCALE_LINEAR)
			pixels[i] = tf * (fval - minvis);
		else if (scale_type == GTK_DATABOX_SCALE_LOG2)
			pixels[i] = tf * log2(fval / minvis);
		else
			pixels[i] = tf * log10(fval / minvis);

		/* handle the wrap-around (ring buffer) issue using modulus.  for efficiency, don't do this for non-wraparound cases. */
		/* note this allows multiple wrap-arounds.  One could hold a single cycle of a sine wave, and plot a continuous wave */
		/* This can be optimized using pointers later */
		if (i + start > maxlen)
			indx = ((i + start) % maxlen) * stride;
		else
			indx += stride;
	} while (++i < len);
}

/**
 * gtk_databox_pixel_to_value_x:
 * @box: A #GtkDatabox widget
 * @pixel: A horizontal pixel coordinate
 *
 * Calculates the x value which is represented by the horizontal @pixel coordinate.
 * Pixel coordinates are relative to the top-left corner of the @box which is equivalent to (0,0).
 *
 * Return value: x value
 */
gfloat
gtk_databox_pixel_to_value_x (GtkDatabox * box, gint16 pixel) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_x == GTK_DATABOX_SCALE_LINEAR)
        return priv->visible_left +
               pixel / priv->translation_factor_x;
    else if (priv->scale_type_x == GTK_DATABOX_SCALE_LOG2)
        return priv->visible_left * pow (2,
                                              pixel /
                                              priv->
                                              translation_factor_x);
    else
        return priv->visible_left * pow (10,
                                              pixel /
                                              priv->
                                              translation_factor_x);
}

/**
 * gtk_databox_pixel_to_value_y:
 * @box: A #GtkDatabox widget
 * @pixel: A vertical pixel coordinate
 *
 * Calculates the y value which is represented by the vertical @pixel coordinate.
 * Pixel coordinates are relative to the top-left corner of the @box which is equivalent to (0,0).
 *
 * Return value: y value
 */
gfloat
gtk_databox_pixel_to_value_y (GtkDatabox * box, gint16 pixel) {
    GtkDataboxPrivate *priv = GTK_DATABOX_GET_PRIVATE(box);

    if (priv->scale_type_y == GTK_DATABOX_SCALE_LINEAR)
        return priv->visible_top +
               pixel / priv->translation_factor_y;
    else if (priv->scale_type_y == GTK_DATABOX_SCALE_LOG2)
        return priv->visible_top * pow (2,
                                             pixel /
                                             priv->
                                             translation_factor_y);
    else
        return priv->visible_top * pow (10,
                                             pixel /
                                             priv->
                                             translation_factor_y);
}

/**
 * gtk_databox_create_box_with_scrollbars_and_rulers:
 * @p_box: Will contain a pointer to a #GtkDatabox widget
 * @p_grid: Will contain a pointer to a #GtkGrid widget
 * @scrollbar_x: Whether to attach a horizontal scrollbar
 * @scrollbar_y: Whether to attach a vertical scrollbar
 * @ruler_x: Whether to attach a horizontal ruler
 * @ruler_y: Whether to attach a vertical ruler
 *
 * This is a convenience function which creates a #GtkDatabox widget in a
 * GtkGrid widget optionally accompanied by scrollbars and rulers. You only
 * have to fill in the data (gtk_databox_graph_add()) and adjust the limits
 * (gtk_databox_set_total_limits() or gtk_databox_auto_rescale()).
 *
 * This function produces the default databox with rulers at the top left and
 * scroll bars at the bottom right.
 *
 * @see_also: gtk_databox_new(), gtk_databox_set_adjustment_x(), gtk_databox_set_adjustment_y(), gtk_databox_set_ruler_x(), gtk_databox_set_ruler_y()
 */
void
gtk_databox_create_box_with_scrollbars_and_rulers (GtkWidget ** p_box,
        GtkWidget ** p_grid,
        gboolean scrollbar_x,
        gboolean scrollbar_y,
        gboolean ruler_x,
        gboolean ruler_y) {
    /* create with rulers top left by default */
    gtk_databox_create_box_with_scrollbars_and_rulers_positioned (p_box, p_grid, scrollbar_x, scrollbar_y, ruler_x, ruler_y, TRUE, TRUE);
}

/**
 * gtk_databox_create_box_with_scrollbars_and_rulers_positioned:
 * @p_box: Will contain a pointer to a #GtkDatabox widget
 * @p_grid: Will contain a pointer to a #GtkGrid widget
 * @scrollbar_x: Whether to attach a horizontal scrollbar
 * @scrollbar_y: Whether to attach a vertical scrollbar
 * @ruler_x: Whether to attach a horizontal ruler
 * @ruler_y: Whether to attach a vertical ruler
 * @ruler_x_top: Whether to put the ruler_x up the top
 * @ruler_y_left: Whether to put the ruler_y on the left
 *
 * This is a convenience function which creates a #GtkDatabox widget in a
 * GtkGrid widget optionally accompanied by scrollbars and rulers. You only
 * have to fill in the data (gtk_databox_graph_add()) and adjust the limits
 * (gtk_databox_set_total_limits() or gtk_databox_auto_rescale()).
 *
 * This function produces the default databox with rulers at the top left and
 * scroll bars at the bottom right.
 *
 * @see_also: gtk_databox_new(), gtk_databox_set_adjustment_x(), gtk_databox_set_adjustment_y(), gtk_databox_set_ruler_x(), gtk_databox_set_ruler_y(), gtk_databox_create_box_with_scrollbars_and_rulers()
 */
void
gtk_databox_create_box_with_scrollbars_and_rulers_positioned (GtkWidget ** p_box,
        GtkWidget ** p_grid,
        gboolean scrollbar_x,
        gboolean scrollbar_y,
        gboolean ruler_x,
        gboolean ruler_y,
        gboolean ruler_x_top,
        gboolean ruler_y_left) {
    GtkGrid *grid;
    GtkDatabox *box;
    GtkWidget *scrollbar;
    GtkWidget *ruler;
    GtkDataboxPrivate *priv;
    gint left_col, top_row;

    *p_grid = gtk_grid_new ();
    *p_box = gtk_databox_new ();
    box = GTK_DATABOX (*p_box);
    grid = GTK_GRID (*p_grid);
    priv = GTK_DATABOX_GET_PRIVATE(box);

    left_col=1;
    top_row=1;
    gtk_grid_attach (grid, GTK_WIDGET (box), left_col, top_row, 1, 1);

    if (scrollbar_x) {
        scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, NULL);
        gtk_databox_set_adjustment_x (box,
                                      gtk_range_get_adjustment (GTK_RANGE
                                              (scrollbar)));
        if (ruler_x_top) {
            left_col=1;
            top_row=2;
        } else {
            left_col=1;
            top_row=0;
        }
        gtk_grid_attach (grid, scrollbar, left_col, top_row, 1, 1);
    }

    if (scrollbar_y) {
        scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, NULL);
        gtk_databox_set_adjustment_y (box,
                                      gtk_range_get_adjustment (GTK_RANGE
                                              (scrollbar)));
        if (ruler_y_left) {
            left_col=2;
            top_row=1;
        } else {
            left_col=0;
            top_row=1;
        }
        gtk_grid_attach (grid, scrollbar, left_col, top_row, 1, 1);
    }

    if (ruler_x) {
        ruler = gtk_databox_ruler_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_databox_ruler_set_scale_type (GTK_DATABOX_RULER (ruler),
                                          priv->scale_type_x);
        if (ruler_x_top) {
            left_col=1;
            top_row=0;
        } else {
            gtk_databox_ruler_set_invert_edge(GTK_DATABOX_RULER(ruler), TRUE); /* set the ruler to reverse its edge */
            left_col=1;
            top_row=2;
        }
        gtk_grid_attach (grid, ruler, left_col, top_row, 1, 1);
        gtk_databox_set_ruler_x (box, GTK_DATABOX_RULER (ruler));
    }

    if (ruler_y) {
        ruler = gtk_databox_ruler_new (GTK_ORIENTATION_VERTICAL);
        gtk_databox_ruler_set_scale_type (GTK_DATABOX_RULER (ruler),
                                          priv->scale_type_y);
        if (ruler_y_left) {
            left_col=0;
            top_row=1;
        } else {
            gtk_databox_ruler_set_invert_edge(GTK_DATABOX_RULER(ruler), TRUE); /* set the ruler to reverse its edge */
            left_col=2;
            top_row=1;
        }
        gtk_grid_attach (grid, ruler, left_col, top_row, 1, 1);
        gtk_databox_set_ruler_y (box, GTK_DATABOX_RULER (ruler));
    }
}

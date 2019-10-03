
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

// xwidgets.h includes xputty.h and all defined widgets from Xputty
#include "xwidgets.h"

#include "sc_record.h"

// png's linked in as binarys
EXTLD(record_png)

/*---------------------------------------------------------------------
-----------------------------------------------------------------------    
                define controller numbers
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define CONTROLS 5

/*---------------------------------------------------------------------
-----------------------------------------------------------------------    
                the main LV2 handle->XWindow
-----------------------------------------------------------------------
----------------------------------------------------------------------*/


// main window struct
typedef struct {
    void *parentXwindow;
    Xputty main;
    Widget_t *win;
    Widget_t *widget[CONTROLS];
    int block_event;

    void *controller;
    LV2UI_Write_Function write_function;
    LV2UI_Resize* resize;
} X11_UI;

static void set_colors(Xputty *app) {
    app->color_scheme->normal = (Colors){
         /* cairo/ r / g / b / a /  */
         /*fg*/{ 0.85, 0.85, 0.85, 1.0},
         /*bg*/{ 0.3, 0.4, 0.5, 1.0},
         /*base*/{ 0.0, 0.0, 0.0, 0.2},
         /*text*/{ 0.9, 0.9, 0.9, 1.0}};

    app->color_scheme->prelight = (Colors){
        /*fg*/{ 1.0, 1.0, 1.0, 1.0},
        /*bg*/{ 0.0, 0.0, 0.0, 1.0},
        /*base*/{ 0.1, 0.1, 0.1, 0.4},
        /*text*/{ 1.0, 1.0, 1.0, 1.0}};

    app->color_scheme->selected = (Colors){
        /*fg*/{ 0.9, 0.9, 0.9, 1.0},
        /*bg*/{ 0.1, 0.2, 0.3, 1.0},
        /*base*/{ 0.8, 0.18, 0.18, 0.2},
         /*text*/{ 1.0, 1.0, 1.0, 1.0}};

    app->color_scheme->active = (Colors){
        /*fg*/{ 1.0, 1.0, 1.0, 1.0},
        /*bg*/{ 0.5, 0.1, 0.1, 1.0},
        /*base*/{ 0.68, 0.28, 0.28, 0.5},
        /*text*/{ 0.75, 0.75, 0.75, 1.0}};
}

// draw the window
static void draw_window(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    widget_set_scale(w);
    cairo_set_source_surface (w->crb, w->image,0,0);
    cairo_paint (w->crb);
    widget_reset_scale(w);
}

static void draw_my_button(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    int width = attrs.width-2;
    int height = attrs.height-2;
    if (attrs.map_state != IsViewable) return;

    if (!w->state && (int)w->adj_y->value)
        w->state = 3;
    if (w->state == 3 && !(int)w->adj_y->value)
        w->state = 0;

    cairo_rectangle(w->crb,2.0, 2.0, width, height);

    if(w->state==0) {
        cairo_set_line_width(w->crb, 1.0);
       // use_bg_color_scheme(w, NORMAL_);
        cairo_fill_preserve(w->crb);
        use_bg_color_scheme(w, PRELIGHT_);
    } else if(w->state==1) {
        use_bg_color_scheme(w, PRELIGHT_);
        cairo_fill_preserve(w->crb);
        cairo_set_line_width(w->crb, 1.5);
        use_bg_color_scheme(w, PRELIGHT_);
    } else if(w->state==2) {
        use_bg_color_scheme(w, SELECTED_);
        cairo_fill_preserve(w->crb);
        cairo_set_line_width(w->crb, 1.0);
        use_bg_color_scheme(w, PRELIGHT_);
    } else if(w->state==3) {
        use_bg_color_scheme(w, ACTIVE_);
        cairo_fill_preserve(w->crb);
        cairo_set_line_width(w->crb, 1.0);
        use_bg_color_scheme(w, PRELIGHT_);
    }
    cairo_stroke(w->crb); 

    if(w->state==2) {
        cairo_rectangle(w->crb,4.0, 4.0, width, height);
        cairo_stroke(w->crb);
        cairo_rectangle(w->crb,3.0, 3.0, width, height);
        cairo_stroke(w->crb);
    } else if (w->state==3) {
        cairo_rectangle(w->crb,3.0, 3.0, width, height);
        cairo_stroke(w->crb);
    }

    float offset = 0.0;
    cairo_text_extents_t extents;
    if(w->state==0) {
        use_fg_color_scheme(w, NORMAL_);
    } else if(w->state==1 && ! (int)w->adj_y->value) {
        use_fg_color_scheme(w, PRELIGHT_);
        offset = 1.0;
    } else if(w->state==1) {
        use_fg_color_scheme(w, ACTIVE_);
        offset = 2.0;
    } else if(w->state==2) {
        use_fg_color_scheme(w, SELECTED_);
        offset = 2.0;
    } else if(w->state==3) {
        use_fg_color_scheme(w, ACTIVE_);
        offset = 1.0;
    }

    use_text_color_scheme(w, get_color_state(w));
    cairo_set_font_size (w->crb, height/2.2);
    cairo_select_font_face (w->crb, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
    cairo_text_extents(w->crb,w->label , &extents);

    cairo_move_to (w->crb, (width-extents.width)*0.5 +offset, (height+extents.height)*0.5 +offset);
    cairo_show_text(w->crb, w->label);
    cairo_new_path (w->crb);
}

// if controller value changed send notify to host
static void value_changed(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI* ui = (X11_UI*)w->parent_struct;
    if (ui->block_event != w->data) 
        ui->write_function(ui->controller,w->data,sizeof(float),0,&w->adj->value);
    ui->block_event = -1;
}

void dummy1_callback(void *w_, void* _data, void* user_data) {
    
}

void dummy_callback(void *w_, void* user_data) {
    
}

// init the xwindow and return the LV2UI handle
static LV2UI_Handle instantiate(const struct _LV2UI_Descriptor * descriptor,
            const char * plugin_uri, const char * bundle_path,
            LV2UI_Write_Function write_function,
            LV2UI_Controller controller, LV2UI_Widget * widget,
            const LV2_Feature * const * features) {

    X11_UI* ui = (X11_UI*)malloc(sizeof(X11_UI));

    if (!ui) {
        fprintf(stderr,"ERROR: failed to instantiate plugin with URI %s\n", plugin_uri);
        return NULL;
    }

    ui->parentXwindow = 0;
    LV2UI_Resize* resize = NULL;
    ui->block_event = -1;

    int i = 0;
    for (; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_UI__parent)) {
            ui->parentXwindow = features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_UI__resize)) {
            resize = (LV2UI_Resize*)features[i]->data;
        }
    }

    if (ui->parentXwindow == NULL)  {
        fprintf(stderr, "ERROR: Failed to open parentXwindow for %s\n", plugin_uri);
        free(ui);
        return NULL;
    }
    // init Xputty
    main_init(&ui->main);
    set_colors(&ui->main);
    // create the toplevel Window on the parentXwindow provided by the host
    ui->win = create_window(&ui->main, (Window)ui->parentXwindow, 0, 0, 338, 257);
    // setup a background image to use on the toplevel window
    widget_get_png(ui->win, LDVAR(record_png));
    // connect the expose func
    ui->win->func.expose_callback = draw_window;
    ui->widget[0] = add_combobox(ui->win, "", 60, 55, 100, 30);
    combobox_add_entry(ui->widget[0],"wav");
    combobox_add_entry(ui->widget[0],"ogg");
    ui->widget[0]->data = FORM;
    ui->widget[0]->parent_struct = ui;
    ui->widget[0]->func.value_changed_callback = value_changed;
    //combobox_set_active_entry(w, 1);


    // create a toggle button
    ui->widget[1] = add_toggle_button(ui->win, "REC", 170, 55, 110, 30);
    // set resize mode for the toggle button to Aspect ratio
    ui->widget[1]->scale.gravity = CENTER;
    // store the Port Index in the Widget_t data field
    ui->widget[1]->data = REC;
    // store a pointer to the X11_UI struct in the parent_struct Widget_t field
    ui->widget[1]->parent_struct = ui;
    // connect the value changed callback with the write_function
    ui->widget[1]->func.value_changed_callback = value_changed;
    ui->widget[1]->func.expose_callback = draw_my_button;
    ui->widget[1]->func.adj_callback = transparent_draw;

    ui->widget[2] = add_toggle_button(ui->win, "", 302, 18, 25, 20);
    // store the Port Index in the Widget_t data field
    ui->widget[2]->data = CLIP;
    set_adjustment(ui->widget[2]->adj,0.0, 0.0, 0.0, 1.0, 1.0, CL_METER);
    ui->widget[2]->func.expose_callback = draw_my_button;
    // disable callbacks
    ui->widget[2]->func.enter_callback = dummy_callback;
    ui->widget[2]->func.leave_callback = dummy_callback;
    ui->widget[2]->func.button_press_callback = dummy1_callback;
    ui->widget[2]->func.button_release_callback = dummy1_callback;
    // store a pointer to the X11_UI struct in the parent_struct Widget_t field
    ui->widget[2]->parent_struct = ui;
    // create a meter widget
    ui->widget[3] = add_vmeter(ui->win, "LMeter", true, 300, 39, 10, 205);
    // store the port index in the Widget_t data field
    ui->widget[3]->data = LMETER;
    ui->widget[3]->scale.gravity = CENTER;
    int a = childlist_find_child(ui->main.childlist, ui->widget[3]);
    ui->main.childlist->childs[a+1]->scale.gravity = CENTER;
    // create a meter widget
    ui->widget[4] = add_vmeter(ui->win, "RMeter", false, 320, 39, 10, 205);
    // store the port index in the Widget_t data field
    ui->widget[4]->data = RMETER;
    ui->widget[4]->scale.gravity = CENTER;
    // finally map all Widgets on screen
    widget_show_all(ui->win);
    if (strstr(plugin_uri, "stereo") == NULL)
        widget_hide(ui->widget[4]);
    // set the widget pointer to the X11 Window from the toplevel Widget_t
    *widget = (void*)ui->win->widget;
    // request to resize the parentXwindow to the size of the toplevel Widget_t
    if (resize){
        ui->resize = resize;
        resize->ui_resize(resize->handle, 380, 280);
    }
    // store pointer to the host controller
    ui->controller = controller;
    // store pointer to the host write function
    ui->write_function = write_function;
    
    return (LV2UI_Handle)ui;
}

// cleanup after usage
static void cleanup(LV2UI_Handle handle) {
    X11_UI* ui = (X11_UI*)handle;
    // Xputty free all memory used
    main_quit(&ui->main);
    free(ui);
}

/*---------------------------------------------------------------------
-----------------------------------------------------------------------    
                        LV2 interface
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

// port value change message from host
static void port_event(LV2UI_Handle handle, uint32_t port_index,
                        uint32_t buffer_size, uint32_t format,
                        const void * buffer) {
    X11_UI* ui = (X11_UI*)handle;
    float value = *(float*)buffer;
    int i=0;
    for (;i<CONTROLS;i++) {
        if (port_index == ui->widget[i]->data) {
            // prevent event loop between host and plugin
            ui->block_event = (int)port_index;
            // case port is METER, convert value to meter deflection
            if (port_index == LMETER || port_index == RMETER)
                value = power2db(ui->widget[i], value);
            // Xputty check if the new value differs from the old one
            // and set new one, when needed
            check_value_changed(ui->widget[i]->adj, &value);
        }
    }
}

// LV2 idle interface to host
static int ui_idle(LV2UI_Handle handle) {
    X11_UI* ui = (X11_UI*)handle;
    // Xputty event loop setup to run one cycle when called
    run_embedded(&ui->main);
    return 0;
}

// LV2 resize interface to host
static int ui_resize(LV2UI_Feature_Handle handle, int w, int h) {
    X11_UI* ui = (X11_UI*)handle;
    // Xputty sends configure event to the toplevel widget to resize itself
    if (ui) send_configure_event(ui->win,0, 0, w, h);
    return 0;
}

// connect idle and resize functions to host
static const void* extension_data(const char* uri) {
    static const LV2UI_Idle_Interface idle = { ui_idle };
    static const LV2UI_Resize resize = { 0 ,ui_resize };
    if (!strcmp(uri, LV2_UI__idleInterface)) {
        return &idle;
    }
    if (!strcmp(uri, LV2_UI__resize)) {
        return &resize;
    }
    return NULL;
}

static const LV2UI_Descriptor descriptor = {
    SCPLUGIN_UI_URI "#mono_record",
    instantiate,
    cleanup,
    port_event,
    extension_data
};

static const LV2UI_Descriptor descriptor1 = {
    SCPLUGIN_UI_URI "#stereo_record",
    instantiate,
    cleanup,
    port_event,
    extension_data
};


LV2_SYMBOL_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
    switch (index) {
        case 0:
            return &descriptor;
        case 1:
            return &descriptor1;
        default:
        return NULL;
    }
}


#include <gtk/gtk.h>
//#include <glib/gstdio.h>

/* object than needs update are global */
GObject *current_mac_label;
GObject *accounts_listbox;
GObject *server_url_entry;
GObject *scan_button;
GObject *mode_random_button;
GObject *mode_sequential_button;

//static void GUI_print_hello (GtkWindow *window) {
//    gtk_window_close (window);
//    printf("hhelllllllllloooo\n");
//}

static void GUI_quit_smac(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    // we link this function with a "simple action"... that is why so many parameters...
    g_application_quit(G_APPLICATION(GTK_APPLICATION(user_data)));
}

static void GUI_reset_mac_label() {
    gtk_label_set_text (GTK_LABEL(current_mac_label), NULL);
}

static void GUI_update_mac_label() {
    const char *format = "[%d] <b><span foreground=\"darkgrey\">\%s</span></b>";
    char *markup;
    markup = g_markup_printf_escaped (format, MAC_SCANNED_COUNT, mac);
    gtk_label_set_markup (GTK_LABEL(current_mac_label), markup);
    g_free (markup);
}

static void GUI_display_error(char *err_msg) {
    //// currently displaying errors on current_mac_label
    const char *format = "<span foreground=\"red\">\%s</span>";
    char *markup;
    markup = g_markup_printf_escaped (format, err_msg);
    gtk_label_set_markup (GTK_LABEL(current_mac_label), markup);
    g_free (markup);

}

static void GUI_add_to_accounts_listbox() {
    const char *format = "<b>\%s</b> [\%s]";
    char *markup;
    markup = g_markup_printf_escaped (format, mac, exp_date);
    GtkWidget *account_label = gtk_label_new(markup);
    gtk_label_set_markup(GTK_LABEL(account_label),markup); // to format text in label
    gtk_list_box_insert(GTK_LIST_BOX(accounts_listbox),account_label,-1); // -1 => end of list
    g_free (markup);
}

static void GUI_set_server_url_from_entry() {
    // TODO: validate url
    const char *user_server_url[MAX_DNS_LEN] = {0};
    *user_server_url = gtk_editable_get_text (GTK_EDITABLE (server_url_entry));
    set_server_url(*user_server_url);
}

static void GUI_set_mode_random() {
    SCAN_MODE = RANDOM;
}

static void GUI_set_mode_sequential() {
    SCAN_MODE = SEQUENTIAL;
}

static void GUI_scan_button_set_label() {
    if (SCAN_THREAD > 0) {
        gtk_button_set_label ( GTK_BUTTON (scan_button), "Stop");
    } else {
        gtk_button_set_label ( GTK_BUTTON (scan_button), "Scan");
    }
}

static void GUI_scan_button_toggle() {
    if (SCAN_THREAD > 0) {
        scan_stop();
    } else {
        scan_start();
    }
    GUI_scan_button_set_label();
}

static void GUI_clear_listbox() {
    gtk_list_box_remove_all(GTK_LIST_BOX(accounts_listbox));
}

const char *ui_builder_string = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<interface>"
"    <object id=\"window\" class=\"GtkApplicationWindow\">"
"        <property name=\"title\">smac</property>"
"        <child>"
"            <object id=\"grid_main\" class=\"GtkGrid\">"
"                <child>"
"                    <object id=\"server_label\" class=\"GtkLabel\">"
"                        <attributes>"
"                            <attribute name=\"weight\" value=\"BOLD\"/>"
"                        </attributes>"
"                        <layout>"
"                            <property name=\"column\">0</property>"
"                            <property name=\"row\">0</property>"
"                        </layout>"
"                            <property name=\"label\">  Server URL  </property>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"server_url_entry\" class=\"GtkEntry\">"
"                        <layout>"
"                            <property name=\"column\">1</property>"
"                            <property name=\"row\">0</property>"
"                            <property name=\"column-span\">2</property>"
"                        </layout>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"mode_label\" class=\"GtkLabel\">"
"                        <attributes>"
"                            <attribute name=\"weight\" value=\"BOLD\"/>"
"                        </attributes>"
"                        <layout>"
"                            <property name=\"column\">0</property>"
"                            <property name=\"row\">1</property>"
"                        </layout>"
"                            <property name=\"label\">  Mode  </property>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"mode_sequential_button\" class=\"GtkCheckButton\">"
"                        <property name=\"label\">Sequential</property>"
"                        <layout>"
"                            <property name=\"column\">1</property>"
"                            <property name=\"row\">1</property>"
"                        </layout>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"mode_random_button\" class=\"GtkCheckButton\">"
"                        <property name=\"label\">Random</property>"
"                        <layout>"
"                            <property name=\"column\">2</property>"
"                            <property name=\"row\">1</property>"
"                        </layout>"
"                    </object>"
"                </child>"
//"                <child>"
//"                    <object id=\"button_mode_mac_file\" class=\"GtkButton\">"
//"                        <property name=\"label\">MAC file</property>"
//"                        <layout>"
//"                            <property name=\"column\">3</property>"
//"                            <property name=\"row\">1</property>"
//"                        </layout>"
//"                    </object>"
//"                </child>"
"                <child>"
"                    <object id=\"mac_current_label\" class=\"GtkLabel\">"
"                        <layout>"
"                            <property name=\"column\">0</property>"
"                            <property name=\"row\">3</property>"
"                            <property name=\"column-span\">4</property>"
"                        </layout>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"mac_found_list_window\" class=\"GtkScrolledWindow\">"
"                        <child>"
"                            <object id=\"accounts_listbox\" class=\"GtkListBox\">"
//"                                <child>"
//"                                    <object id=\"found_mac_example_0\" class=\"GtkLabel\">"
//"                                        <attributes>"
//"                                            <attribute name=\"weight\" value=\"BOLD\"/>"
//"                                            <attribute name=\"foreground\" value=\"black\"/>"
//"                                        </attributes>"
//"                                        <property name=\"label\">00:1A:79:00:00:00 [August 16, 2026, 12:00 am]</property>"
//"                                    </object>"
//"                                </child>"
"                            </object>"
"                        </child>"
"                        <layout>"
"                            <property name=\"column\">0</property>"
"                            <property name=\"row\">4</property>"
"                            <property name=\"column-span\">4</property>"
"                        </layout>"
"                        <property name=\"min-content-height\">200</property>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"button_scan_toggle\" class=\"GtkButton\">"
"                        <property name=\"label\">Scan</property>"
"                        <layout>"
"                            <property name=\"column\">1</property>"
"                            <property name=\"row\">8</property>"
"                        </layout>"
"                    </object>"
"                </child>"
"            </object>"
"        </child>"
"    </object>"
"</interface>";

static void activate (GtkApplication *app, gpointer user_data) {
    
    /* Construct a GtkBuilder instance and load our UI description */
    GtkBuilder *builder = gtk_builder_new();

#if 0
    //Add from "./builder.ui" file
    gtk_builder_add_from_file (builder, "builder.ui", NULL);
#else
    //Add from "ui_builder_string" variable
    gtk_builder_add_from_string(builder, ui_builder_string, strlen(ui_builder_string), NULL);
#endif

    /* Connect signal handlers to the constructed widgets. */
    //GObject *window = gtk_builder_get_object(builder, "window");
    GtkWidget *window = GTK_WIDGET (gtk_builder_get_object(builder, "window"));
    gtk_window_set_application(GTK_WINDOW (window), app);
    //gtk_application_add_window(app,GTK_WINDOW(window));

    /////////////////////////////////
    /////* menu bar */ 
    GMenu *menu_bar;
    menu_bar = g_menu_new();

    GMenu *menu_file = g_menu_new();
    g_menu_append_submenu ( menu_bar, "File", G_MENU_MODEL ( menu_file ) );

    GSimpleAction *menu_file_quit_action = g_simple_action_new("quit",NULL);
    g_action_map_add_action ( G_ACTION_MAP ( app ), G_ACTION ( menu_file_quit_action ) );
    g_signal_connect ( menu_file_quit_action, "activate", G_CALLBACK ( GUI_quit_smac ), app );
    GMenuItem *menu_file_quit = g_menu_item_new ( "Quit", "app.quit" );
    g_menu_append_item ( menu_file, menu_file_quit );
    //g_object_unref ( menu_file_quit_action );
    
    //GMenu *menu_about = g_menu_new();
    //g_menu_append_submenu ( menu_bar, "About", G_MENU_MODEL ( menu_about ) );


    gtk_application_set_menubar             ( GTK_APPLICATION ( app ), G_MENU_MODEL ( menu_bar ) );
    gtk_application_window_set_show_menubar ( GTK_APPLICATION_WINDOW ( window ), TRUE );

    //////////

    server_url_entry = gtk_builder_get_object(builder, "server_url_entry");

    /////////////////////////////////
    /* MODE buttons (group "MODE") */
    mode_random_button = gtk_builder_get_object (builder, "mode_random_button");
    g_signal_connect (mode_random_button, "toggled", G_CALLBACK (GUI_set_mode_random), NULL);
    
    mode_sequential_button = gtk_builder_get_object (builder, "mode_sequential_button");
    g_signal_connect (mode_sequential_button, "toggled", G_CALLBACK (GUI_set_mode_sequential), NULL);

        /*default is sequential mode*/
    gtk_check_button_set_active(GTK_CHECK_BUTTON(mode_sequential_button),TRUE);

        /*group those buttons together*/
    gtk_check_button_set_group(GTK_CHECK_BUTTON(mode_random_button),GTK_CHECK_BUTTON(mode_sequential_button));

    /////////////////////////////
    /* MAC label */
    current_mac_label = gtk_builder_get_object(builder, "mac_current_label");

    /////////////////////////////
    /* list box*/
    accounts_listbox = gtk_builder_get_object(builder, "accounts_listbox");
    gtk_widget_set_hexpand(GTK_WIDGET (accounts_listbox), TRUE);

    /////////////////////////////////
    /* buttons with mutable labels */
    scan_button = gtk_builder_get_object (builder, "button_scan_toggle");
    g_signal_connect (scan_button, "clicked", G_CALLBACK (GUI_scan_button_toggle), NULL);

    /////////////////////////////////
    /* We do not need the builder any more */
    g_object_unref (builder);
   
    /////////////////////////////////
    /* make window visible */
    gtk_widget_set_visible (window, TRUE);
}

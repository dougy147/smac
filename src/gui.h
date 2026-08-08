#include <gtk/gtk.h>
//#include <glib/gstdio.h>

/* Construct a GtkBuilder instance and load our UI description */
GtkBuilder *builder; // = gtk_builder_new ();

/* object than needs update are global */
GObject *current_mac_label;
GObject *accounts_listbox;
GObject *server_url_entry;
GObject *scan_button;

static void GUI_quit_smac (GtkWindow *window) {
    if (SCAN_THREAD > 0) scan_stop();
    gtk_window_close (window);
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

const char *ui_builder_string = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<interface>"
"    <object id=\"window\" class=\"GtkWindow\">"
"        <property name=\"title\">smac</property>"
//// test menu
//"        <child>"
//            "  <menu id='menubar'>"
//            "    <submenu>"
//            "      <attribute name='label' translatable='yes'>_Edit</attribute>"
//            "      <item>"
//            "        <attribute name='label' translatable='yes'>_Copy</attribute>"
//            "        <attribute name='action'>win.copy</attribute>"
//            "      </item>"
//            "      <item>"
//            "        <attribute name='label' translatable='yes'>_Paste</attribute>"
//            "        <attribute name='action'>win.paste</attribute>"
//            "      </item>"
//            "    </submenu>"
//            "  </menu>"
//"        </child>"
////
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
"                    <object id=\"button_mode_random\" class=\"GtkButton\">"
"                        <property name=\"label\">Random</property>"
"                        <layout>"
"                            <property name=\"column\">1</property>"
"                            <property name=\"row\">1</property>"
"                        </layout>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"button_mode_sequential\" class=\"GtkButton\">"
"                        <property name=\"label\">Sequential</property>"
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
//"                            <property name=\"column-span\">2</property>"
"                        </layout>"
"                    </object>"
"                </child>"
//"                <child>"
//"                    <object id=\"quit\" class=\"GtkButton\">"
//"                        <property name=\"label\">Quit</property>"
//"                        <layout>"
//"                            <property name=\"column\">1</property>"
//"                            <property name=\"row\">8</property>"
//"                        </layout>"
//"                    </object>"
//"                </child>"
"            </object>"
"        </child>"
"    </object>"
"</interface>";

static void activate (GtkApplication *app, gpointer user_data) {
    
    builder = gtk_builder_new ();
    //gtk_builder_add_from_file (builder, "builder.ui", NULL);
    gtk_builder_add_from_string (builder, ui_builder_string, strlen(ui_builder_string), NULL);

    /* Connect signal handlers to the constructed widgets. */
    GObject *window = gtk_builder_get_object (builder, "window");
    gtk_window_set_application (GTK_WINDOW (window), app);

    /////// grid
    //GtkWidget *grid;
    //grid = GTK_WIDGET (gtk_builder_get_object (builder, "grid_main"));
    //gtk_widget_set_hexpand(grid, TRUE);
    //gtk_widget_set_halign(grid, GTK_ALIGN_FILL);

    /////* menu bar */
    //GMenuModel *menubar = G_MENU_MODEL (gtk_builder_get_object (builder, "menubar"));
    //gtk_application_set_menubar (GTK_APPLICATION (app), G_MENU_MODEL (menubar));

    /* buttons with unmutable labels */
    GObject *button;

    button = gtk_builder_get_object (builder, "button_mode_random");
    g_signal_connect (button, "clicked", G_CALLBACK (GUI_set_mode_random), NULL);
    
    button = gtk_builder_get_object (builder, "button_mode_sequential");
    g_signal_connect (button, "clicked", G_CALLBACK (GUI_set_mode_sequential), NULL);

    //button = gtk_builder_get_object (builder, "button_scan_start");
    //g_signal_connect (button, "clicked", G_CALLBACK (scan_start), NULL);
    //
    //button = gtk_builder_get_object (builder, "button_scan_stop");
    //g_signal_connect (button, "clicked", G_CALLBACK (scan_stop), NULL);

    /* buttons with mutable labels */
    scan_button = gtk_builder_get_object (builder, "button_scan_toggle");
    g_signal_connect (scan_button, "clicked", G_CALLBACK (GUI_scan_button_toggle), NULL);

    //button = gtk_builder_get_object (builder, "quit");
    //g_signal_connect_swapped (button, "clicked", G_CALLBACK (GUI_quit_smac), window);

    gtk_widget_set_visible (GTK_WIDGET (window), TRUE);
    
    current_mac_label = gtk_builder_get_object(builder, "mac_current_label");
    //gtk_label_set_text (GTK_LABEL(current_mac_label), "Hello World!");

    /////* list box*/
    accounts_listbox = gtk_builder_get_object(builder, "accounts_listbox");
    gtk_widget_set_hexpand(GTK_WIDGET (accounts_listbox), TRUE);
    //gtk_widget_set_halign(GTK_WIDGET (accounts_listbox), TRUE);

    server_url_entry = gtk_builder_get_object(builder, "server_url_entry");
    /* We do not need the builder any more */
    g_object_unref (builder);

}

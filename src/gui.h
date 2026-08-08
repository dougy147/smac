#include <gtk/gtk.h>
#include <glib/gstdio.h>

// NOTE: prepend INTERFACE_ to every functions?
    
/* Construct a GtkBuilder instance and load our UI description */
GtkBuilder *builder; // = gtk_builder_new ();

/* object than needs update are global */
GObject *current_mac_label;
GObject *accounts_listbox;
GObject *server_url_entry;

//static void print_hello (GtkWidget *widget, gpointer   data) {
//    g_print ("Hello World\n");
//}

static void quit_smac (GtkWindow *window) {
    if (SCAN_THREAD > 0) scan_stop();
    gtk_window_close (window);
}

static void update_mac_label() {
    gtk_label_set_text (GTK_LABEL(current_mac_label), mac);
}

static void display_error_on_mac_label(char *err_msg) {
    gtk_label_set_text (GTK_LABEL(current_mac_label), err_msg);
}

static void add_to_accounts_listbox() {

    char account_str[FULL_MAC_STR_LEN+MAX_EXP_DATE_LEN+1] = {0};

    strcat(account_str,"<b>");
        strcat(account_str,mac);
    strcat(account_str,"</b>");

    strcat(account_str," ");

    strcat(account_str,"[");
        strcat(account_str,exp_date);
    strcat(account_str,"]");

    GtkWidget *account_label = gtk_label_new(account_str);
    gtk_label_set_markup(GTK_LABEL(account_label),account_str); // to format text in label
    gtk_list_box_insert(GTK_LIST_BOX(accounts_listbox),account_label,-1); // -1 => end of list
}

static void set_server_url_from_entry() {
    const char *user_server_url[MAX_DNS_LEN] = {0};
    *user_server_url = gtk_editable_get_text (GTK_EDITABLE (server_url_entry));
    //printf("User server url = <%s>\n", *user_server_url);
    // TODO: trim user input
    // TODO: validate url
    set_server_url(*user_server_url);
}

// TODO: i do not know how to pass argument to function
//       called by a button
//static void set_mode(int mode) {
//    SCAN_MODE = mode;
//}

static void set_mode_random() {
    SCAN_MODE = RANDOM;
}

static void set_mode_sequential() {
    SCAN_MODE = SEQUENTIAL;
}

const char *ui_builder_string = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<interface>"
"    <object id=\"window\" class=\"GtkWindow\">"
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
"                            <property name=\"column-span\">3</property>"
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
"                    <object id=\"button_scan_start\" class=\"GtkButton\">"
"                        <property name=\"label\">Scan</property>"
"                        <layout>"
"                            <property name=\"column\">1</property>"
"                            <property name=\"row\">2</property>"
"                        </layout>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"button_scan_stop\" class=\"GtkButton\">"
"                        <property name=\"label\">Stop</property>"
"                        <layout>"
"                            <property name=\"column\">2</property>"
"                            <property name=\"row\">2</property>"
"                        </layout>"
"                    </object>"
"                </child>"
"                <child>"
"                    <object id=\"mac_current_label\" class=\"GtkLabel\">"
"                        <attributes>"
"                            <attribute name=\"weight\" value=\"BOLD\"/>"
"                            <attribute name=\"background\" value=\"darkgrey\"/>"
"                        </attributes>"
"                        <layout>"
"                            <property name=\"column\">0</property>"
"                            <property name=\"row\">3</property>"
"                            <property name=\"column-span\">4</property>"
"                        </layout>"
//"                        <property name=\"label\">the mac label</property>"
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
"                    <object id=\"quit\" class=\"GtkButton\">"
"                        <property name=\"label\">Quit</property>"
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
                                ;

static void activate (GtkApplication *app, gpointer        user_data) {
    
    builder = gtk_builder_new ();
    //gtk_builder_add_from_file (builder, "builder.ui", NULL);
    gtk_builder_add_from_string (builder, ui_builder_string, strlen(ui_builder_string), NULL);

    /* Connect signal handlers to the constructed widgets. */
    GObject *window = gtk_builder_get_object (builder, "window");
    gtk_window_set_application (GTK_WINDOW (window), app);

    GObject *button;

    button = gtk_builder_get_object (builder, "button_mode_random");
    g_signal_connect (button, "clicked", G_CALLBACK (set_mode_random), NULL);
    
    button = gtk_builder_get_object (builder, "button_mode_sequential");
    g_signal_connect (button, "clicked", G_CALLBACK (set_mode_sequential), NULL);

    button = gtk_builder_get_object (builder, "button_scan_start");
    g_signal_connect (button, "clicked", G_CALLBACK (scan_start), NULL);
    
    button = gtk_builder_get_object (builder, "button_scan_stop");
    g_signal_connect (button, "clicked", G_CALLBACK (scan_stop), NULL);

    button = gtk_builder_get_object (builder, "quit");
    g_signal_connect_swapped (button, "clicked", G_CALLBACK (quit_smac), window);

    gtk_widget_set_visible (GTK_WIDGET (window), TRUE);
    
    current_mac_label = gtk_builder_get_object(builder, "mac_current_label");
    //gtk_label_set_text (GTK_LABEL(current_mac_label), "Hello World!");
    
    accounts_listbox = gtk_builder_get_object(builder, "accounts_listbox");

    server_url_entry = gtk_builder_get_object(builder, "server_url_entry");
    /* We do not need the builder any more */
    g_object_unref (builder);
}

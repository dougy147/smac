#include <QApplication>
#include <QWidget>
#include <QtUiTools/QUiLoader>

#include <QFileInfo>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QStringListModel>
#include <QRadioButton>
#include <QToolButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>

#include <unistd.h>

#ifdef _WIN32
    #include <windows.h>
#endif

/* Qt interface stuff */
    /* main scan tab */
QLineEdit        *label_mac; // was a label is now lineedit disabled
QPushButton      *button_reset_to_first_mac;
QLineEdit        *entry_server_url;
QListView        *accounts_listview;
QStringListModel *accounts_listview_model;
QStringList      *accounts_list;
QPushButton      *button_scan;
QRadioButton     *radio_button_sequential;
QRadioButton     *radio_button_random;
    /* settings tab */
QLineEdit   *entry_settings_first_mac;
QLineEdit   *entry_settings_last_mac;
QLineEdit   *entry_settings_mac_prefix;
QLineEdit   *entry_settings_save_dir;
QCheckBox   *checkbox_settings_autosave;
QCheckBox   *checkbox_settings_checkpoints;
QToolButton *toolbutton_settings_select_dir;

/* Scan stuff */
bool SCANNING = false;
bool GRACEFUL_EXIT_ASKED = false;

#define THREADS_LIMIT 32
int NB_THREADS = 1; // user defined

#include "src/smac.c"
#include "src/shared.h"

pthread_t main_thread;

//extern "C" void GUI_update_scanning_labels(const char *mac)
void GUI_update_scanning_labels(const char *mac)
{
    const QString mac_str = QString::fromUtf8(mac);

    // the QueuedConnection invoked on the label "label_mac"
    // avoids trouble with the interface crashing because it
    // was being updated by a function running in a thread
    // (the "smac.c -> scan() function). it seems to solve
    // things for now, but let's be vigilant <|:)
    if (SCANNING) {
        QMetaObject::invokeMethod(label_mac, [mac_str]() {
            label_mac->setText(mac_str);
        }, Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(label_mac, []() {
            button_scan->setText("Start");
        }, Qt::QueuedConnection);
    }
}

//extern "C" void GUI_add_account_to_accounts_list(const char *mac, const char *exp_date)
void GUI_add_account_to_accounts_list(const char *mac, const char *exp_date)
{
    char account[STR_MAC_LEN + MAX_EXP_LEN];
    snprintf(account,sizeof(account),"%s [%s]", mac, exp_date);
    const QString account_str = QString::fromUtf8(account);

    QMetaObject::invokeMethod(label_mac, [account_str]() {
            accounts_list->append(account_str);
            // Don't forget to update the model StringList ptr
            accounts_listview_model->setStringList(*accounts_list);
    }, Qt::QueuedConnection);

}

bool is_whitespace(char c) {
    const char *ws = " \t\n\r";
    while (ws[0] != '\0') if (c == *ws++) return true;
    return false;
}

void trim(char *s) {
    char *p = s;
    int l = strlen(s);
    while (p[0] != '\0' && is_whitespace(p[0])) p++;
    for (int i=0;i<=l-(p-s);i++) s[i] = s[i+(p-s)];
    while (l-1 >= 0 && is_whitespace(s[l-1])) l--;
    s[l] = '\0';
}

void update_output_filename_from_url(char *output_filename, char *url) {
    trim(url);
    const char *http = "http://";
    const char *https = "https://";

    int i = 0;
    while (*url != '\0') {
        if (strncmp(url,http,strlen(http)) == 0)   url+=strlen(http);
        if (strncmp(url,https,strlen(https)) == 0) url+=strlen(https);
        output_filename[i++] = url[0];
        url++;
        if (*url == '/') break;
    }
    const char *extension = ".txt";
    while (extension[0] != '\0') output_filename[i++] = extension++[0];
    output_filename[i] = '\0';
}

bool update_server_url() {

    char new_host[MAX_URL_LEN] = {0};

    strcpy(new_host,entry_server_url->text().toStdString().c_str());

    trim(new_host);
    strcpy(host,new_host);

    if (strcmp(host,host_previous) == 0) return false;
    
    //TODO: Check if it is a correct URL
    update_output_filename_from_url(output_filename_accounts, host);
    update_output_filename_from_url(output_filename_checkpoints, host);

    printf("[i] Updated 'entry_server_url' from \"%s\" to \"%s\"\n", host_previous, host);
    printf("[i] Updated 'output_filename' = \"%s\"\n", output_filename_accounts);

    strcpy(host_previous,host);
    return true;
}

void import_settings() {
    strcpy(mac_first,  entry_settings_first_mac->text().toStdString().c_str());
    strcpy(mac_last,   entry_settings_last_mac->text().toStdString().c_str());
    strcpy(mac_prefix, entry_settings_mac_prefix->text().toStdString().c_str());
}

void path_to_windows_path(char *path) {
    char *p = path;
    char win_path[MAX_URL_LEN] = {0};
    while (p[0] != '\0') {
        if (p[0] == '/') {
            win_path[strlen(win_path)] = '\\';
            win_path[strlen(win_path)] = '\\';
        } else {
            win_path[strlen(win_path)] = p[0];
        }
        p++;
    }
    win_path[strlen(win_path)] = '\0';
    strcpy(path,win_path);
}

void load_checkpoint() {
    char checkpoint_path[MAX_URL_LEN] = {0};
    snprintf(checkpoint_path,sizeof(checkpoint_path),"%s/%s",output_dir_checkpoints,output_filename_checkpoints);
    printf("checkpoint_path = %s\n", checkpoint_path);
    FILE *f = fopen(checkpoint_path,"r");
    if (f) {
        char buffer[STR_MAC_LEN] = {0};
        fread (buffer, 1, STR_MAC_LEN, f);
        printf("read last checkpoint = %s\n", buffer);
        strcpy(mac_first,buffer);
        printf("new first mac = %s\n", mac_first);
        fclose(f);
    }

    if (SCAN_MODE == SEQUENTIAL) {
        label_mac->setText(mac_first);
        strcpy(mac,mac_first);
        printf("new mac = %s\n", mac);
    }
}

void remove_checkpoint() {
    char checkpoint_path[MAX_URL_LEN] = {0};
    snprintf(checkpoint_path,sizeof(checkpoint_path),"%s/%s",output_dir_checkpoints,output_filename_checkpoints);
#ifdef _WIN32
        DeleteFileA(checkpoint_path);
#else
        unlink(checkpoint_path);
#endif
}

int main(int argc, char *argv[]) {
    
    srand(time(NULL));

#ifdef _WIN32 // Hide useless widnows console
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_HIDE);
#endif

    if (NB_THREADS <= 0) {
        fprintf(stderr,"[!] NB_THREADS must be a positive integer.\n");
        return 1;
    }

    if (NB_THREADS > THREADS_LIMIT) {
        fprintf(stderr,"[!] %d threads declared, but limit is %d.\n",NB_THREADS,THREADS_LIMIT);
        return 1;
    }

    QApplication app(argc, argv);

    QFile uiFile("interface.ui");
    if (!uiFile.open(QIODevice::ReadOnly)) return 1;

    QUiLoader loader;
    //loader.setLanguageChangeEnabled(true);

    QWidget *w = loader.load(&uiFile);
    uiFile.close();
    if (!w) return 1;

    /* ================= MENU BAR ===================== */

    QAction *menu_quit = w->findChild<QAction*>("actionQuit");
    QObject::connect(menu_quit, &QAction::triggered, menu_quit, [&]() {
            app.exit();
    });

    /* ============== MAIN SCAN TAB =================== */
    
    /* Host/Server_URL entry text (QLineEdit for now) */
    entry_server_url = w->findChild<QLineEdit*>("server_url");
    entry_server_url->setText(host);

    /* Radio Buttons (sequential, random, mac file?) */
    radio_button_sequential = w->findChild<QRadioButton*>("radio_button_sequential");
    radio_button_random = w->findChild<QRadioButton*>("radio_button_random");

    QObject::connect(radio_button_sequential, &QRadioButton::clicked,radio_button_sequential, [&]() { 
            SCAN_MODE = SEQUENTIAL;
            printf("[i] SCAN_MODE = SEQUENTIAL\n");
    });
    
    QObject::connect(radio_button_random, &QRadioButton::clicked,radio_button_random, [&]() { 
            SCAN_MODE = RANDOM;
            printf("[i] SCAN_MODE = RANDOM\n");
    });

    /* Current MAC scanned label */
    label_mac = w->findChild<QLineEdit*>("label_mac");
    label_mac->setText(mac_first);

    button_reset_to_first_mac = w->findChild<QPushButton*>("button_reset_to_first_mac");

    QObject::connect(button_reset_to_first_mac, &QPushButton::clicked,button_reset_to_first_mac, [&]() { 
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(button_reset_to_first_mac, "Reset checkpoint", "Reset checkpoint for that host?", QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                remove_checkpoint();
                strcpy(mac_first,entry_settings_first_mac->text().toLocal8Bit().constData());
                label_mac->setText(mac_first);
                if (!SCANNING && SCAN_MODE == SEQUENTIAL) strcpy(mac,mac_first);
            }
    });

    /* Threads Combobox */
    QComboBox *thread_combobox= w->findChild<QComboBox*>("thread_combobox");

    for (int i=1;i <= (THREADS_LIMIT / 4) ;i++) {
        char str[10];
        sprintf(str, "%d", i);
        thread_combobox->addItems({str});
    }

    QObject::connect(thread_combobox, &QComboBox::activated,thread_combobox, [&]() { 
            NB_THREADS = thread_combobox->currentIndex() + 1;
            printf("[i] NB_THREADS = %d\n", NB_THREADS);
    });

    /* ListView */
    accounts_listview       = w->findChild<QListView*>("accounts_listview");
    accounts_listview_model = new QStringListModel(); // initialize model
    accounts_list           = new QStringList(); // initialize strings list
    
    accounts_listview_model->setStringList(*accounts_list); // connect string list to model
    accounts_listview->setModel(accounts_listview_model); // connect model to listview

    /* Scan button */ 
    button_scan = w->findChild<QPushButton*>("pushButton");

    // [&]() is a lambda that captures everything by reference (so you can mention previous code)
    // else []()   does not capture
    QObject::connect(button_scan, &QPushButton::clicked, w, [&]() { 

            if (!SCANNING) {

                import_settings();

                bool url_updated = update_server_url();
                if (url_updated) {
                    MAC_COUNT = 0;
                    strcpy(mac,mac_first); // for sequential mode
                }
                
                load_checkpoint();

                GRACEFUL_EXIT_ASKED = false; // cf below
                pthread_create(&main_thread, NULL, &start, NULL);
                button_scan->setText("Stop");

            } else {

                GRACEFUL_EXIT_ASKED = true; // cf below
                pthread_cancel(main_thread); // does nothing on windows
                if (main_thread > 0) pthread_join(main_thread, NULL); //// wait for thread to finish
#ifndef _WIN32
                // ignore this if compiling for windows
                // race condition => thread might never update GRACEFUL_EXIT_ASKED
                GRACEFUL_EXIT_ASKED = false;
#endif
                button_scan->setText("Start");

            }
            SCANNING = !SCANNING;
            GUI_update_scanning_labels(mac);
    });
    
    /* ============== SETTINGS TAB =================== */

    entry_settings_first_mac       = w->findChild<QLineEdit*>("settings_first_mac");
    if (strlen(mac_first) > 0) entry_settings_first_mac->setText(mac_first);

    entry_settings_last_mac        = w->findChild<QLineEdit*>("settings_last_mac");
    if (strlen(mac_last) > 0) entry_settings_last_mac->setText(mac_last);

    entry_settings_mac_prefix      = w->findChild<QLineEdit*>("settings_mac_prefix");
    if (strlen(mac_prefix) > 0) entry_settings_mac_prefix->setText(mac_prefix);

    checkbox_settings_checkpoints  = w->findChild<QCheckBox*>("settings_use_checkpoints");
    if (USE_CHECKPOINTS) {
        checkbox_settings_checkpoints->setChecked(true);
    }
    
    QObject::connect(checkbox_settings_checkpoints, &QCheckBox::toggled, w, [&]() {
        USE_CHECKPOINTS = !USE_CHECKPOINTS;
        printf("USE_CHECKPOINTS = %d\n",USE_CHECKPOINTS);
    });

    checkbox_settings_autosave     = w->findChild<QCheckBox*>("settings_autosave_checkbox");
    if (AUTO_SAVE_ACCOUNTS) {
        checkbox_settings_autosave->setChecked(true);
    }
    
    QObject::connect(checkbox_settings_autosave, &QCheckBox::toggled, w, [&]() {
        AUTO_SAVE_ACCOUNTS = !AUTO_SAVE_ACCOUNTS;
        printf("AUTO_SAVE_ACCOUNTS = %d\n",AUTO_SAVE_ACCOUNTS);
    });


    toolbutton_settings_select_dir = w->findChild<QToolButton*>("settings_select_dir_toolbutton");
    entry_settings_save_dir        = w->findChild<QLineEdit*>("settings_save_dir");

    if (strlen(output_dir) > 0) {
        entry_settings_save_dir->setText(output_dir);

        // TODO: make this in a proper function
        char mkdir_cmd[MAX_URL_LEN] = {0};
        strcat(mkdir_cmd, "mkdir ");
        strcat(mkdir_cmd, output_dir);
        system(mkdir_cmd); // create result dir
        printf("created saving dir: %s\n", mkdir_cmd);
        mkdir_cmd[0] = '\0';
        strcat(mkdir_cmd, "mkdir ");
        strcat(mkdir_cmd, output_dir_checkpoints);
        system(mkdir_cmd); // create results/checkpoints dir
        printf("created checkpoints saving dir: %s\n", mkdir_cmd);

    }

    QObject::connect(toolbutton_settings_select_dir, &QToolButton::clicked,toolbutton_settings_select_dir, [&]() { 
            const QString f = QFileDialog::getExistingDirectory();
            char filepath[128];
            strcpy(filepath,f.toLocal8Bit().constData());

            if (strlen(filepath) == 0) return;

            // save new output dir
            entry_settings_save_dir->setText(filepath);
            strcpy(output_dir,filepath);
#ifdef _WIN32
            path_to_windows_path(output_dir);
#endif
            printf("selected dir = %s\n", output_dir);

            // save new checkpoints dir
            strcpy(output_dir_checkpoints, output_dir);
            strcat(output_dir_checkpoints,"/checkpoints");
#ifdef _WIN32
            path_to_windows_path(output_dir_checkpoints);
#endif
            char mkdir_cmd[MAX_URL_LEN] = {0}; 
            strcat(mkdir_cmd, "mkdir ");
            strcat(mkdir_cmd, output_dir_checkpoints);
            system(mkdir_cmd); // create checkpoints dir
            printf("selected checkpoints dir = %s\n", output_dir_checkpoints);
    });

    /* =============================================== */

    w->show();
    return app.exec();
}

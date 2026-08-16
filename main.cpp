#include <QApplication>
#include <QWidget>
#include <QtUiTools/QUiLoader>

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
#include <QProgressBar>

#include <unistd.h>

#ifdef _WIN32
    #include <windows.h>
#endif

/* Qt interface stuff */
    /* main scan tab */
QLineEdit        *label_mac; // was a label is now lineedit disabled for style
QProgressBar     *busy_indicator;
QPushButton      *button_reset_checkpoint;
QLineEdit        *entry_server_url;
QListView        *accounts_listview;
QStringListModel *accounts_listview_model;
QStringList      *accounts_list;
QPushButton      *button_scan;
QRadioButton     *radio_button_sequential;
QRadioButton     *radio_button_random;
    /* settings tab */
QLineEdit   *entry_settings_mac_first;
QLineEdit   *entry_settings_mac_last;
QLineEdit   *entry_settings_mac_prefix;
QLineEdit   *entry_settings_save_dir;
QCheckBox   *checkbox_settings_autosave;
QCheckBox   *checkbox_settings_checkpoints;
QToolButton *toolbutton_settings_select_dir;
QLineEdit   *entry_request_delay;
QLineEdit   *entry_request_timeout;
QLineEdit   *entry_pause_nb;
QLineEdit   *entry_pause_duration;
QLineEdit   *entry_proxy_url;
QLineEdit   *entry_proxy_username;
QLineEdit   *entry_proxy_password;

#include "src/smac.c"
#include "src/shared.h"

pthread_t main_thread;

// https://runebook.dev/en/docs/qt/qleinteger/QLEInteger
// given an int variable name, ensure its associated entry_ will only allow integers in
// the text field.
#define entry_of_int(NAME) \
    QIntValidator* validator_entry_##NAME = new QIntValidator(0, 3600000, entry_##NAME); \
    entry_##NAME->setValidator(validator_entry_##NAME); \
    if (NAME >= 0) entry_##NAME->setText(QString::number(NAME)); \
    else printf("[w] Invalid or absent default value provided for global variable \"" #NAME "\"\n"); \
    QObject::connect(entry_##NAME, &QLineEdit::textChanged,entry_##NAME, []() {  \
        NAME = entry_##NAME->text().toInt(); \
    });

#define yesno(QOBJECT,TITLE,MESSAGE) \
    QMessageBox::StandardButton reply; \
    reply = QMessageBox::question((QOBJECT), (TITLE), (MESSAGE), QMessageBox::Yes|QMessageBox::No);

#define to_cstr(QSTRING) \
    (char*)(QSTRING).toLocal8Bit().constData()
        
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

void GUI_scan_ended_by_itself() {
    // update stuff related to a scan not running
    GRACEFUL_EXIT_ASKED = false;
    QMetaObject::invokeMethod(button_scan, []() {
        button_scan->setText("Start");
    }, Qt::QueuedConnection); 

    QMetaObject::invokeMethod(entry_server_url, []() {
        entry_server_url->setEnabled(true);
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

void path_to_windows_path(char *path) {
    char *p = path;
    char win_path[MAX_URL_LEN] = {0};
    while (p[0] != '\0') {
        if (strlen(win_path) == MAX_URL_LEN) {
            // TODO: properly alert user we are not changing the path because of this:
            printf("[!] Could not convert to Windows path: buffer overflow, path too long");
            strcpy(win_path,path);
            break;
        }
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

void mkdir(char *path) {
    char mkdir_cmd[MAX_URL_LEN] = {0};
    strcat(mkdir_cmd, "mkdir ");
#ifdef _WIN32
    path_to_windows_path(path);
#endif
    strcat(mkdir_cmd, path);
    system(mkdir_cmd);
}

void build_filename_from_url(char *filename, char *url, const char *extension) {
    // copy sanitized substring of URL + extension into filename
    trim(url);
    const char *http = "http://";
    const char *https = "https://";

    int i = 0;
    while (*url != '\0') {
        if (strncmp(url,http,strlen(http)) == 0)   url+=strlen(http);
        if (strncmp(url,https,strlen(https)) == 0) url+=strlen(https);
        filename[i++] = url[0];
        url++;
        if (*url == '/') break;
    }

    while (extension[0] != '\0') filename[i++] = extension++[0];
    filename[i] = '\0';
}

void clean_accounts_listview() {
    accounts_listview_model->removeRows(0, accounts_listview_model->rowCount());
    accounts_listview->setModel(accounts_listview_model);
}

bool server_url_changed() {
    char new_host[MAX_URL_LEN] = {0};
    strcpy(new_host,to_cstr(entry_server_url->text()));
    trim(new_host);
    if (strcmp(host,new_host) == 0) return false;
    return true;
}

void update_server_url() {
    if (!server_url_changed) return;
    
    char new_host[MAX_URL_LEN] = {0};
    strcpy(new_host,to_cstr(entry_server_url->text()));
    trim(new_host);
    strcpy(host,new_host);
}

void load_settings() {
    strcpy(mac_first,  to_cstr(entry_settings_mac_first->text()));
    strcpy(mac_last,   to_cstr(entry_settings_mac_last->text()));
    strcpy(mac_prefix, to_cstr(entry_settings_mac_prefix->text()));
}

void load_checkpoint(char *server) {

    char checkpoint_path[MAX_URL_LEN] = {0};
    build_filename_from_url(checkpoint_filename, server, ".txt");
    
    snprintf(checkpoint_path,sizeof(checkpoint_path),"%s/%s",checkpoints_dir,checkpoint_filename);
    
    FILE *f = fopen(checkpoint_path,"r");
    if (f) {
        char last_checkpoint[STR_MAC_LEN] = {0};
        fread (last_checkpoint, 1, STR_MAC_LEN, f);
        strcpy(mac_first,last_checkpoint);
        fclose(f);
    } else {
        label_mac->setText(mac_first);
    }


    if (SCAN_MODE == SEQUENTIAL) {
        label_mac->setText(mac_first);
        strcpy(mac,mac_first);
    }
}

void remove_checkpoint(char *server) {
    char path[MAX_URL_LEN] = {0};
    snprintf(path,sizeof(path),"%s/%s",checkpoints_dir,checkpoint_filename);
#ifdef _WIN32
        DeleteFileA(path);
#else
        unlink(path);
#endif
}

void start_scanning_user() {
    load_settings();
    
    bool url_changed = server_url_changed();
    if (url_changed) {

        if (!AUTO_SAVE_ACCOUNTS && ACCOUNTS_COUNT > 0) {
            yesno(button_scan, "Clear accounts?", "Accounts in the list are not saved and will be deleted by a new scan. Proceed?");
            if (reply == QMessageBox::No) return;
        }
        
        update_server_url();
        MAC_COUNT = 0;
        ACCOUNTS_COUNT = 0;
        strcpy(mac,mac_first); // for sequential mode
        clean_accounts_listview();
    }

    strcpy(host_previous,host);

    build_filename_from_url(accounts_filename,   host, ".txt");
    //build_filename_from_url(checkpoint_filename, host, ".txt");
    if (USE_CHECKPOINTS) load_checkpoint(host);
    entry_server_url->setEnabled(false);

    SCANNING = true;
    printf("scanning is %d\n", SCANNING);
    GRACEFUL_EXIT_ASKED = false; // cf below
    pthread_create(&main_thread, NULL, &start, NULL);
    button_scan->setText("Stop");
}

void stop_scanning_user() {
    if (main_thread <= 0) {
        return;
    }
    GRACEFUL_EXIT_ASKED = true; // cf below
    pthread_cancel(main_thread); // does nothing on windows
    
    if (main_thread > 0) {
        pthread_join(main_thread, NULL); //// wait for thread to finish
    }

#ifndef _WIN32
    // ignore this if compiling for windows
    // race condition => thread might never update GRACEFUL_EXIT_ASKED
    GRACEFUL_EXIT_ASKED = false;
#endif
    
    button_scan->setText("Start");
    entry_server_url->setEnabled(true);
    SCANNING = false;
}

int main(int argc, char *argv[]) {
    
    srand(time(NULL));

//#ifdef _WIN32 // Hide useless widnows console
//    HWND console = GetConsoleWindow();
//    ShowWindow(console, SW_HIDE);
//#endif

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
    QObject::connect(entry_server_url, &QLineEdit::textChanged,entry_server_url, []() { 
       if (!SCANNING && USE_CHECKPOINTS) load_checkpoint(to_cstr(entry_server_url->text()));
    });


    /* Radio Buttons (sequential, random, mac file?) */
    radio_button_sequential = w->findChild<QRadioButton*>("radio_button_sequential");
    radio_button_random     = w->findChild<QRadioButton*>("radio_button_random");

    QObject::connect(radio_button_sequential, &QRadioButton::clicked,radio_button_sequential, [&]() { 
            SCAN_MODE = SEQUENTIAL;
    });
    
    QObject::connect(radio_button_random, &QRadioButton::clicked,radio_button_random, [&]() { 
            SCAN_MODE = RANDOM;
    });

    /* Current MAC scanned label */
    label_mac = w->findChild<QLineEdit*>("label_mac");
    label_mac->setText(mac_first);

    /* busy indicator (just a progress bar with no number)*/
    busy_indicator = w->findChild<QProgressBar*>("busy_indicator");
    busy_indicator->setVisible(SCANNING);

    /* reset check point button */

    button_reset_checkpoint = w->findChild<QPushButton*>("button_reset_checkpoint");

    QObject::connect(button_reset_checkpoint, &QPushButton::clicked, button_reset_checkpoint, [&]() {
        yesno(button_reset_checkpoint,"Reset checkpoint","Reset checkpoint for that host?");

        if (reply == QMessageBox::Yes) {
            //char server_cstr[MAX_URL_LEN] = {0};
            //strcpy(server_cstr, entry_server_url->text().toLocal8Bit().constData());
            remove_checkpoint(to_cstr(entry_server_url->text()));
            
            strcpy(mac_first,entry_settings_mac_first->text().toLocal8Bit().constData());
            label_mac->setText(mac_first);
            if (!SCANNING && SCAN_MODE == SEQUENTIAL) strcpy(mac,mac_first);
        }
    });

    /* Threads Combobox */
    QComboBox *thread_combobox= w->findChild<QComboBox*>("thread_combobox");

    for (int i=1;i <= (THREADS_LIMIT / 4) ;i++) {
        thread_combobox->addItems({QString::number(i)});
    }

    QObject::connect(thread_combobox, &QComboBox::activated,thread_combobox, [&]() { 
            NB_THREADS = thread_combobox->currentIndex() + 1;
    });

    /* ListView */
    accounts_listview       = w->findChild<QListView*>("accounts_listview");
    accounts_listview_model = new QStringListModel(); // initialize model
    accounts_list           = new QStringList(); // initialize strings list
    
    accounts_listview_model->setStringList(*accounts_list); // connect string list to model
    accounts_listview->setModel(accounts_listview_model); // connect model to listview

    /* Scan button */ 
    button_scan = w->findChild<QPushButton*>("button_scan");

    // [&]() is a lambda that captures everything by reference (so you can mention previous code)
    // else []()   does not capture
    QObject::connect(button_scan, &QPushButton::clicked, w, [&]() { 
        if (!SCANNING) start_scanning_user();
        else           stop_scanning_user();
        busy_indicator->setVisible(SCANNING);
        GUI_update_scanning_labels(mac);
    });
    
    /* ============== SETTINGS TAB =================== */

    entry_settings_mac_first       = w->findChild<QLineEdit*>("settings_mac_first");
    if (strlen(mac_first) > 0) entry_settings_mac_first->setText(mac_first);
    
    QObject::connect(entry_settings_mac_first, &QLineEdit::textChanged,entry_settings_mac_first, []() { 
            strcpy(mac_first,  to_cstr(entry_settings_mac_first->text()));
            if (SCAN_MODE == SEQUENTIAL && !SCANNING && MAC_COUNT == 0) {
                label_mac->setText(mac_first);
            }
    });

    entry_settings_mac_last        = w->findChild<QLineEdit*>("settings_mac_last");
    if (strlen(mac_last) > 0) entry_settings_mac_last->setText(mac_last);
    
    QObject::connect(entry_settings_mac_last, &QLineEdit::textChanged,entry_settings_mac_last, []() { 
            strcpy(mac_last,  to_cstr(entry_settings_mac_last->text()));
    });

    entry_settings_mac_prefix      = w->findChild<QLineEdit*>("settings_mac_prefix");
    if (strlen(mac_prefix) > 0) entry_settings_mac_prefix->setText(mac_prefix);
    
    QObject::connect(entry_settings_mac_last, &QLineEdit::textChanged,entry_settings_mac_last, []() { 
            strcpy(mac_prefix,  to_cstr(entry_settings_mac_last->text()));
    });

    checkbox_settings_checkpoints  = w->findChild<QCheckBox*>("settings_use_checkpoints");
    if (USE_CHECKPOINTS) {
        checkbox_settings_checkpoints->setChecked(true);
    }
    
    QObject::connect(checkbox_settings_checkpoints, &QCheckBox::toggled, w, [&]() {
        USE_CHECKPOINTS = !USE_CHECKPOINTS;
    });

    checkbox_settings_autosave     = w->findChild<QCheckBox*>("settings_autosave_checkbox");
    if (AUTO_SAVE_ACCOUNTS) {
        checkbox_settings_autosave->setChecked(true);
    }
    
    QObject::connect(checkbox_settings_autosave, &QCheckBox::toggled, w, [&]() {
        AUTO_SAVE_ACCOUNTS = !AUTO_SAVE_ACCOUNTS;
    });

    toolbutton_settings_select_dir = w->findChild<QToolButton*>("settings_select_dir_toolbutton");
    entry_settings_save_dir        = w->findChild<QLineEdit*>("settings_save_dir");

    if (strlen(results_dir) > 0) {
        entry_settings_save_dir->setText(results_dir);
        mkdir(results_dir);
        mkdir(checkpoints_dir);
    }

    QObject::connect(toolbutton_settings_select_dir, &QToolButton::clicked,toolbutton_settings_select_dir, [&]() { 
            const QString f = QFileDialog::getExistingDirectory();
            char filepath[128];
            strcpy(filepath,f.toLocal8Bit().constData());

            if (strlen(filepath) == 0) return;

            // save new output dir
            entry_settings_save_dir->setText(filepath);
            strcpy(results_dir,filepath);
#ifdef _WIN32
            path_to_windows_path(results_dir);
#endif
            // save new checkpoints dir
            strcpy(checkpoints_dir, results_dir);
            strcat(checkpoints_dir,DEFAULT_PATH_SEPARATOR"checkpoints");
            mkdir(checkpoints_dir);
        });

    entry_request_delay = w->findChild<QLineEdit*>("entry_request_delay");
    entry_of_int(request_delay);

    entry_request_timeout = w->findChild<QLineEdit*>("entry_request_timeout");
    entry_of_int(request_timeout);
    
    entry_pause_nb = w->findChild<QLineEdit*>("entry_pause_nb");
    entry_of_int(pause_nb);

    entry_pause_duration = w->findChild<QLineEdit*>("entry_pause_duration");
    entry_of_int(pause_duration);

    /* ========= proxy settings ======= */
    entry_proxy_url = w->findChild<QLineEdit*>("entry_proxy_url");
    QObject::connect(entry_proxy_url, &QLineEdit::textChanged,entry_proxy_url, []() { 
        strcpy(proxy_url,  to_cstr(entry_proxy_url->text()));
        trim(proxy_url);
    });
    
    entry_proxy_username = w->findChild<QLineEdit*>("entry_proxy_username");
    QObject::connect(entry_proxy_username, &QLineEdit::textChanged,entry_proxy_username, []() { 
        strcpy(proxy_username,  to_cstr(entry_proxy_username->text()));
        trim(proxy_username);
    });

    entry_proxy_password = w->findChild<QLineEdit*>("entry_proxy_password");
    QObject::connect(entry_proxy_password, &QLineEdit::textChanged,entry_proxy_password, []() { 
        strcpy(proxy_password,  to_cstr(entry_proxy_password->text()));
        trim(proxy_password);
    });

    /* =============================================== */

    w->show();
    return app.exec();
}

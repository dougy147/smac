#define SMAC_GUI 1

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
#include <QFrame>

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
QRadioButton     *radio_button_from_file;
QToolButton      *toolbutton_file_mac_file;
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
QLineEdit   *entry_stop_count;
QLineEdit   *entry_genre_match;
QCheckBox   *checkbox_settings_check_playable;
/*proxy settings*/

QCheckBox    *checkbox_use_proxy;
QFrame       *frame_proxy;
QLineEdit    *entry_manual_proxy_url;
QLineEdit    *entry_manual_proxy_username;
QLineEdit    *entry_manual_proxy_password;
QRadioButton *radio_button_proxy_manual;
QRadioButton *radio_button_proxy_file;
QRadioButton *radio_button_proxy_file_url;
QLabel       *label_manual_proxy_url;
QLabel       *label_manual_proxy_username;
QLabel       *label_manual_proxy_password;
QLabel       *label_file_proxy;
QToolButton  *toolbutton_file_proxy;
QToolButton  *toolbutton_file_url_proxy;
QLineEdit    *entry_file_proxy;
QLineEdit    *entry_file_url_proxy;

#include "src/smac.c"
#include "src/shared.h"
#include "src/utils.c"

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
    })

#define yesnobox(PARENT,TITLE,MESSAGE) \
    QMessageBox::StandardButton reply; \
    reply = QMessageBox::question((PARENT), (TITLE), (MESSAGE), QMessageBox::Yes|QMessageBox::No)
    
#define okbox(PARENT,TITLE,MESSAGE) \
    QMessageBox::information((PARENT), (TITLE), (MESSAGE), QMessageBox::Ok)

#define to_cstr(QSTRING) \
    (char*)(QSTRING).toLocal8Bit().constData()
        
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
            button_scan->setText("Stop");
        }, Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(label_mac, []() {
            button_scan->setText("Start");
        }, Qt::QueuedConnection);
    }
}

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
    // attaching to 'button_scan' seems to work
    QMetaObject::invokeMethod(button_scan, []() {
        button_scan->setEnabled(false);
        GRACEFUL_EXIT_ASKED = false;
        button_scan->setText("Start");
        entry_server_url->setEnabled(true);
        busy_indicator->setVisible(false);
        button_scan->setEnabled(true);
        SCANNING = false;
    }, Qt::QueuedConnection); 
}

void GUI_clean_accounts_listview() {
    accounts_listview_model->removeRows(0, accounts_listview_model->rowCount());
    accounts_listview->setModel(accounts_listview_model);
}

bool GUI_server_url_changed() {
    char new_host[MAX_URL_LEN] = {0};
    strcpy(new_host,to_cstr(entry_server_url->text()));
    trim(new_host);
    if (strcmp(host,new_host) == 0) return false;
    return true;
}

void GUI_update_server_url() {
    if (!GUI_server_url_changed) return;
    
    char new_host[MAX_URL_LEN] = {0};
    strcpy(new_host,to_cstr(entry_server_url->text()));
    trim(new_host);
    strcpy(host,new_host);
}

void GUI_load_settings() {
    strcpy(host     ,  to_cstr(entry_server_url->text()));
    strcpy(mac_first,  to_cstr(entry_settings_mac_first->text()));
    strcpy(mac_last,   to_cstr(entry_settings_mac_last->text()));
    strcpy(mac_prefix, to_cstr(entry_settings_mac_prefix->text()));
}

void GUI_scan_start() {
    button_scan->setEnabled(false);
    
    GUI_load_settings();

    bool url_changed = GUI_server_url_changed();
    if (url_changed) {

        if (!AUTO_SAVE_ACCOUNTS && ACCOUNTS_COUNT > 0) {
            yesnobox(button_scan, "Clear accounts?", "Accounts in the list are not saved and will be deleted by a new scan. Proceed?");
            if (reply == QMessageBox::No) return;
        }
        
        GUI_update_server_url();
        MAC_COUNT = 0;
        ACCOUNTS_COUNT = 0;
        strcpy(mac,mac_first); // for sequential mode
        GUI_clean_accounts_listview();
    }

    strcpy(host_previous,host);
    
    label_mac->setText(mac_first);
    entry_server_url->setEnabled(false);    
    button_scan->setText("Stop");
    button_scan->setEnabled(true);

    // must be last call of current func to avoid blocking
    smac_main();
}

void GUI_scan_stop() {
    button_scan->setEnabled(false);
    
    if (main_thread > 0) {
        
        GRACEFUL_EXIT_ASKED = true; // cf below
        pthread_cancel(main_thread); // does nothing on windows
        
        if (main_thread > 0) {
            pthread_join(main_thread, NULL); //// wait for thread to finish
        }
        
    }

#ifndef _WIN32
    // ignore this if compiling for windows
    // race condition => thread might never update GRACEFUL_EXIT_ASKED
    GRACEFUL_EXIT_ASKED = false;
#endif
    
    button_scan->setText("Start");
    entry_server_url->setEnabled(true);
    busy_indicator->setVisible(false);
    SCANNING = false;

    button_scan->setEnabled(true);
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
    radio_button_sequential  = w->findChild<QRadioButton*>("radio_button_sequential");
    radio_button_random      = w->findChild<QRadioButton*>("radio_button_random");
    radio_button_from_file   = w->findChild<QRadioButton*>("radio_button_from_file");
    toolbutton_file_mac_file = w->findChild<QToolButton*>("toolbutton_file_mac_file");

    QObject::connect(radio_button_sequential, &QRadioButton::clicked,radio_button_sequential, [&]() { 
        SCAN_MODE = SEQUENTIAL;
        toolbutton_file_mac_file->setEnabled(false);
    });
    
    QObject::connect(radio_button_random, &QRadioButton::clicked,radio_button_random, [&]() { 
        SCAN_MODE = RANDOM;
        toolbutton_file_mac_file->setEnabled(false);
    });

    QObject::connect(radio_button_from_file, &QRadioButton::clicked,radio_button_from_file, [&]() { 
        SCAN_MODE = FROM_FILE;
        toolbutton_file_mac_file->setEnabled(true);
    });

    QObject::connect(toolbutton_file_mac_file, &QToolButton::clicked,toolbutton_file_mac_file, [&]() { 
        const QString f = QFileDialog::getOpenFileName();
        char filepath[128];
        strcpy(filepath,f.toLocal8Bit().constData());

        if (strlen(filepath) == 0) return;

        // save new output dir
        strcpy(MAC_FILE_FILEPATH,filepath);
#ifdef _WIN32
        path_to_windows_path(MAC_FILE_FILEPATH);
#endif
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
        yesnobox(button_reset_checkpoint,"Reset checkpoint","Reset checkpoint for that host?");

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
        if (!SCANNING) GUI_scan_start();
        else           GUI_scan_stop();
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
        make_directory(results_dir);
        make_directory(checkpoints_dir);
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
            make_directory(checkpoints_dir);
        });

    entry_request_delay = w->findChild<QLineEdit*>("entry_request_delay");
    entry_of_int(request_delay);

    entry_request_timeout = w->findChild<QLineEdit*>("entry_request_timeout");
    entry_of_int(request_timeout);
    
    entry_pause_nb = w->findChild<QLineEdit*>("entry_pause_nb");
    entry_of_int(pause_nb);

    entry_pause_duration = w->findChild<QLineEdit*>("entry_pause_duration");
    entry_of_int(pause_duration);

    entry_stop_count = w->findChild<QLineEdit*>("entry_stop_count");

    QIntValidator* validator_entry_stop_count = new QIntValidator(-1, 3600000, entry_stop_count);
    entry_stop_count->setValidator(validator_entry_stop_count);
    if (MAX_MAC_COUNT >= 0) entry_stop_count->setText(QString::number(MAX_MAC_COUNT));
    QObject::connect(entry_stop_count, &QLineEdit::textChanged,entry_stop_count, []() {
        MAX_MAC_COUNT = entry_stop_count->text().toInt();
    });


    checkbox_settings_check_playable  = w->findChild<QCheckBox*>("checkbox_settings_check_playable");
    if (CHECK_PLAYABLE) {
        checkbox_settings_check_playable->setChecked(true);
    }
    
    QObject::connect(checkbox_settings_check_playable, &QCheckBox::toggled, w, [&]() {
        CHECK_PLAYABLE = !CHECK_PLAYABLE;
    });

    entry_genre_match = w->findChild<QLineEdit*>("entry_genre_match");
    QObject::connect(entry_genre_match, &QLineEdit::textChanged,entry_genre_match, []() { 
        strcpy(GENRE_PATTERN,  to_cstr(entry_genre_match->text()));
        trim(GENRE_PATTERN);
        if (strlen(GENRE_PATTERN) > 0) {
            CHECK_GENRE_MATCH = true;
        } else {
            CHECK_GENRE_MATCH = false;
        }
    });


    /* ========= proxy settings ======= */
    checkbox_use_proxy = w->findChild<QCheckBox*>("checkbox_use_proxy");
    frame_proxy = w->findChild<QFrame*>("frame_proxy");
    frame_proxy->setEnabled(USE_PROXY);
    frame_proxy->setVisible(USE_PROXY);
    
    radio_button_proxy_manual   = w->findChild<QRadioButton*>("radio_button_proxy_manual"); // this on is checked by default
    radio_button_proxy_file     = w->findChild<QRadioButton*>("radio_button_proxy_file");
    radio_button_proxy_file_url = w->findChild<QRadioButton*>("radio_button_proxy_file_url");

    QObject::connect(checkbox_use_proxy, &QCheckBox::toggled, w, [&]() {
        USE_PROXY = !USE_PROXY;
        frame_proxy->setEnabled(USE_PROXY);
        frame_proxy->setVisible(USE_PROXY);
        
        radio_button_proxy_manual->setEnabled(USE_PROXY);
        radio_button_proxy_file->setEnabled(USE_PROXY);
        radio_button_proxy_file_url->setEnabled(USE_PROXY);

        if (USE_PROXY) {
            PROXY_MODE = MANUAL;
        } else {
            PROXY_MODE = NONE;
        }
        
    });

    //radio_button_proxy_manual->setChecked(true); // this is the default selection if user uses
    label_manual_proxy_url = w->findChild<QLabel*>("label_manual_proxy_url");
    entry_manual_proxy_url = w->findChild<QLineEdit*>("entry_manual_proxy_url");
    QObject::connect(entry_manual_proxy_url, &QLineEdit::textChanged,entry_manual_proxy_url, []() { 
        strcpy(PROXY_MANUAL_URL,  to_cstr(entry_manual_proxy_url->text()));
        trim(PROXY_MANUAL_URL);
    });

    label_manual_proxy_username = w->findChild<QLabel*>("label_manual_proxy_username");
    entry_manual_proxy_username = w->findChild<QLineEdit*>("entry_manual_proxy_username");
    QObject::connect(entry_manual_proxy_username, &QLineEdit::textChanged,entry_manual_proxy_username, []() { 
        strcpy(PROXY_MANUAL_USERNAME,  to_cstr(entry_manual_proxy_username->text()));
        trim(PROXY_MANUAL_USERNAME);
    });
    
    label_manual_proxy_password = w->findChild<QLabel*>("label_manual_proxy_password");
    entry_manual_proxy_password = w->findChild<QLineEdit*>("entry_manual_proxy_password");
    QObject::connect(entry_manual_proxy_password, &QLineEdit::textChanged,entry_manual_proxy_password, []() { 
        strcpy(PROXY_MANUAL_PASSWORD,  to_cstr(entry_manual_proxy_password->text()));
        trim(PROXY_MANUAL_PASSWORD);
    });

    label_file_proxy = w->findChild<QLabel*>("label_file_proxy");
    toolbutton_file_proxy = w->findChild<QToolButton*>("toolbutton_file_proxy");
    entry_file_proxy = w->findChild<QLineEdit*>("entry_file_proxy");

    if (strlen(PROXY_FILE_FILEPATH) > 0) {
        entry_file_proxy->setText(PROXY_FILE_FILEPATH);
    }
    
    QObject::connect(toolbutton_file_proxy, &QToolButton::clicked,toolbutton_file_proxy, [&]() { 
            const QString f = QFileDialog::getOpenFileName();
            char filepath[MAX_PATH_LEN];
            strcpy(filepath,f.toLocal8Bit().constData());
            if (strlen(filepath) == 0) return;
            entry_file_proxy->setText(filepath);
            init_proxy_from_file(filepath);
        });

    toolbutton_file_url_proxy = w->findChild<QToolButton*>("toolbutton_file_url_proxy");
    entry_file_url_proxy = w->findChild<QLineEdit*>("entry_file_url_proxy");
    entry_file_url_proxy->setText(PROXY_FILE_URL);
    
    QObject::connect(toolbutton_file_url_proxy, &QToolButton::clicked,toolbutton_file_url_proxy, [&]() { 

        char url[MAX_URL_LEN];
        strcpy(url,to_cstr(entry_file_url_proxy->text()));
        trim(url);
        bool ok = init_proxy_from_url(url);
        if (ok) {
            okbox(toolbutton_file_url_proxy,"Proxy list imported","Successfully imported proxy list");
        } else {
            okbox(toolbutton_file_url_proxy,"Error","No proxy found in provided list");
        }
    });

    // CONNECT RADIO BUTTONS HERE
    QObject::connect(radio_button_proxy_manual, &QRadioButton::clicked,radio_button_proxy_manual, [&]() { 
        // enables stuff
        label_manual_proxy_url->setEnabled(true);
        entry_manual_proxy_url->setEnabled(true);
        label_manual_proxy_username->setEnabled(true);
        entry_manual_proxy_username->setEnabled(true);
        label_manual_proxy_password ->setEnabled(true);
        entry_manual_proxy_password->setEnabled(true);

        // disables stuff
        label_file_proxy->setEnabled(false);
        toolbutton_file_proxy->setEnabled(false);
        //entry_file_proxy->setEnabled(false);
        toolbutton_file_url_proxy->setEnabled(false);
        entry_file_url_proxy->setEnabled(false);

        PROXY_MODE = MANUAL;
        
    });

    QObject::connect(radio_button_proxy_file, &QRadioButton::clicked,radio_button_proxy_file, [&]() { 
        // enables stuff
        label_file_proxy->setEnabled(true);
        toolbutton_file_proxy->setEnabled(true);
        //entry_file_proxy->setEnabled(true);
        
        
        // disables stuff
        label_manual_proxy_url->setEnabled(false);
        entry_manual_proxy_url->setEnabled(false);
        label_manual_proxy_username->setEnabled(false);
        entry_manual_proxy_username->setEnabled(false);
        label_manual_proxy_password->setEnabled(false);
        entry_manual_proxy_password->setEnabled(false);

        toolbutton_file_url_proxy->setEnabled(false);
        entry_file_url_proxy->setEnabled(false);

        PROXY_MODE = FROM_FILE;
    });

    QObject::connect(radio_button_proxy_file_url, &QRadioButton::clicked,radio_button_proxy_file_url, [&]() { 
        // enables stuff
        toolbutton_file_url_proxy->setEnabled(true);
        entry_file_url_proxy->setEnabled(true); 
        
        // disables stuff
        label_manual_proxy_url->setEnabled(false);
        entry_manual_proxy_url->setEnabled(false);
        label_manual_proxy_username->setEnabled(false);
        entry_manual_proxy_username->setEnabled(false);
        label_manual_proxy_password->setEnabled(false);
        entry_manual_proxy_password->setEnabled(false);
        label_file_proxy->setEnabled(false);
        toolbutton_file_proxy->setEnabled(false);
        //entry_file_proxy->setEnabled(false);

        PROXY_MODE = FROM_URL;
    });    





    /* =============================================== */

    w->show();
    return app.exec();
}

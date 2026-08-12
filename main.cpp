#include <QApplication>
#include <QWidget>
#include <QtUiTools/QUiLoader>
#include <QFileInfo>
//#include <QFile>
#include <QPushButton>
#include <QLabel>

#define THREADS_LIMIT 32
int NB_THREADS = 1; // user defined

QLabel *label_mac;

#include "src/smac.c"

pthread_t main_thread;


int main(int argc, char *argv[]) {

    if (NB_THREADS > THREADS_LIMIT) {
        printf("[!] %d threads declared, but limit is %d.\n",NB_THREADS,THREADS_LIMIT);
        return 1;
    }

    QApplication app(argc, argv);

    QFile uiFile("interface.ui");
    if (!uiFile.open(QIODevice::ReadOnly)) return 1;

    QUiLoader loader;
    loader.setLanguageChangeEnabled(true);

    QWidget *w = loader.load(&uiFile);
    uiFile.close();
    if (!w) return 2;

    //

    //if (auto *btn = w->findChild<QPushButton*>("pushButton")) {
    //    QObject::connect(btn, &QPushButton::clicked, w, []() { hello(); });
    //}
    //
    label_mac = w->findChild<QLabel*>("label_mac");
    
    auto *btn = w->findChild<QPushButton*>("pushButton");
    bool toggled = false;

    //btn->setText("hello");
    
    // [&]() is a lambda that captures everything by reference (so you can mention previous code)
    // else []()   does not capture
    QObject::connect(btn, &QPushButton::clicked, w, [&]() { 
            if (!toggled) {
                //btn->setText("goodbye");
                //hello();
                pthread_create(&main_thread, NULL, &start, NULL);
                btn->setText("Stop");
            } else {
                pthread_cancel(main_thread);
                btn->setText("Scan");
            }
            toggled = !toggled;
    });

    w->show();


    return app.exec();
}

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.setFixedSize(525, 450); // Set required size from specs
    w.setWindowTitle("Pokémon RPG");
    w.show();

    return a.exec();
}

#include <QApplication>
#include "PlayerMainForm.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    PlayerMainForm* w = new PlayerMainForm;
    w->show();
    return a.exec();
}

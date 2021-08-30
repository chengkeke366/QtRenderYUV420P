#include <QApplication>
#include "PlayerMainForm.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    PlayerMainForm* w = new PlayerMainForm;
    w->show();
    w->startReadYuv420FileThread("C:/Users/ChengKeKe/Desktop/player420.yuv",852,480);
    return a.exec();
}

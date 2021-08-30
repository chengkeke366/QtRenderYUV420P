#ifndef PLAYERMAINFORM_H
#define PLAYERMAINFORM_H

#include <QMainWindow>
#include <thread>

namespace Ui {
class PlayerMainForm;
}
class QFile;
class PlayerMainForm : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlayerMainForm(QWidget *parent = nullptr);
    ~PlayerMainForm();
    void startReadYuv420FileThread(const QString &filename, int width, int height);
protected:
    bool eventFilter(QObject* watched, QEvent* event);

private:
    Ui::PlayerMainForm *ui;
    std::thread *m_read_yuv_data_thread ;
    bool m_bexit_thread = false;
    bool m_pause_thread = false;
    QFile* m_yuv_file = nullptr;
    int m_yuv_width=0, m_yuv_height=0;
};

#endif // PLAYERMAINFORM_H

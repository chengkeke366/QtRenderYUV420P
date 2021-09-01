#ifndef PLAYERMAINFORM_H
#define PLAYERMAINFORM_H

#include <QMainWindow>
#include <thread>
#include <mutex>

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
private:
    //获取前一帧数据
    void getCurrentPositionBackWardFrame(int position);
    //获取后一帧数据
    void getCurrentPositionNextFrame(int position);

    bool frameIsValid(QByteArray YuvData[3], int width, int height);

    void exitReadYUVThread();
public slots:
    void openFile();
private:
    Ui::PlayerMainForm *ui = nullptr;
    std::thread *m_read_yuv_data_thread = nullptr;
    std::atomic_bool m_bexit_thread = false;
    std::atomic_bool m_pause_thread = false;
    QFile* m_yuv_file = nullptr;
    int m_yuv_width=0, m_yuv_height=0;
    std::mutex m_seek_mutex;
    std::atomic_int m_speed_intelval = 1000/25;
};

#endif // PLAYERMAINFORM_H

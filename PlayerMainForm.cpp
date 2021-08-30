#include "PlayerMainForm.h"
#include "ui_PlayerMainForm.h"
#include <QMouseEvent>

PlayerMainForm::PlayerMainForm(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PlayerMainForm)
{
    ui->setupUi(this);
    ui->horizontalSlider->installEventFilter(this);
	connect(ui->horizontalSlider, &QSlider::sliderReleased, this, [=]() {
		qint64 currentValue = ui->horizontalSlider->value();
		qint64 yuv_step = m_yuv_width * m_yuv_height + m_yuv_width * m_yuv_height /4 ;
		qint64 new_position = currentValue% yuv_step != 0 ? currentValue - (currentValue % yuv_step) + yuv_step : currentValue;
		m_yuv_file->seek(new_position);
		ui->horizontalSlider->setValue(new_position);
		ui->openGLWidget->update();
		m_pause_thread = false;
	});

	connect(ui->horizontalSlider, &QSlider::sliderPressed, this, [=]() {
		m_pause_thread = true;
	});
}

PlayerMainForm::~PlayerMainForm()
{
	 m_pause_thread = true;
     m_bexit_thread = true;
     if(m_read_yuv_data_thread && m_read_yuv_data_thread->joinable())
     {
         m_read_yuv_data_thread->join();
     }
    delete ui;
}

void PlayerMainForm::startReadYuv420FileThread(const QString& filename, int width, int height)
{
	m_yuv_width = width;
	m_yuv_height = height;

	m_read_yuv_data_thread = new std::thread([this](const QString& filename) {
		m_yuv_file = new QFile(filename);
		QByteArray data[3];
		if (m_yuv_file->open(QIODevice::ReadOnly))
		{ //read yuv
			qint64 maxsize = m_yuv_file->size();
			ui->horizontalSlider->setMaximum(maxsize);
			while (!m_bexit_thread)
			{
				while (!m_pause_thread)
				{
					if (m_yuv_file->atEnd()) {
						qDebug() << "repaly file:" << m_yuv_file->fileName();
						m_yuv_file->seek(0);
						continue;
					}

					for (int i = 0; i < 3; i++)
					{
						if (i == 0) {
							data[i] = m_yuv_file->read(m_yuv_width * m_yuv_height);
						}
						else {
							data[i] = m_yuv_file->read(m_yuv_width * m_yuv_height / 4);
						}
					}
					//跨线程，刷新界面送往UI主线程，此时必须捕获data数组，如果只捕获data分量指针，有可能由于导致跨线程读写问题引起crash
					QMetaObject::invokeMethod(this, [this, data]() mutable {
						int stride[3] = { m_yuv_width, m_yuv_width / 2, m_yuv_width / 2 };//填入stride
						uint8_t* yuvArr[3] = { (uint8_t*)(data[0].data()), yuvArr[1] = (uint8_t*)(data[1].data()), yuvArr[2] = (uint8_t*)(data[2].data()) };
						ui->openGLWidget->setTextureI420PData(yuvArr, stride, m_yuv_width, m_yuv_height);
					});
					std::this_thread::sleep_for(std::chrono::milliseconds(40));
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
	}, filename);
}

bool PlayerMainForm::eventFilter(QObject* watched, QEvent* event)
{  
    if (event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->button() == Qt::LeftButton)	//判断左键
		{
			int dur = ui->horizontalSlider->maximum() - ui->horizontalSlider->minimum();
			int pos = ui->horizontalSlider->minimum() + dur * ((double)mouseEvent->x() / ui->horizontalSlider->width());
			if (pos != ui->horizontalSlider->sliderPosition())
			{
                ui->horizontalSlider->setValue(pos);
			}
		}
	}
    return QObject::eventFilter(watched, event);
}

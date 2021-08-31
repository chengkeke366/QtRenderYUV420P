#include "PlayerMainForm.h"
#include "ui_PlayerMainForm.h"
#include <QMouseEvent>
#include <QButtonGroup>
#include <QComboBox>

PlayerMainForm::PlayerMainForm(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PlayerMainForm)
{
    ui->setupUi(this);
  

	QButtonGroup* btn_group = new QButtonGroup(this);
	btn_group->addButton(ui->play);
	btn_group->addButton(ui->pause);
	btn_group->addButton(ui->stop);


	connect(ui->horizontalSlider, &QSlider::sliderReleased, this, [=]() {
		int currentValue = ui->horizontalSlider->sliderPosition();
		int yuv_step = m_yuv_width * m_yuv_height* 3 /2 ;
		qint64 new_position = (qint64)yuv_step * currentValue;
		{
			std::lock_guard<std::mutex> locker(m_seek_mutex);
			if (m_yuv_file)
			{
				m_yuv_file->seek(new_position);
			}
		}

		if (currentValue == ui->horizontalSlider->maximum()) {
			ui->stop->setChecked(true);
			ui->stop->toggled(true);
			return;
		}
		qDebug() << "[PlayerMainForm::sliderReleased]";
		if (!ui->pause->isChecked() && !ui->stop->isChecked())
		{
			m_pause_thread = false;
		}
	});

	connect(ui->horizontalSlider, &QSlider::sliderPressed, this, [=]() {
		qDebug() << "[PlayerMainForm::sliderPressed]" ;
		m_pause_thread = true;
		ui->label->setText(QString("%1/%2").arg(ui->horizontalSlider->value()).arg(ui->horizontalSlider->maximum()));
	});

	//拖拽时，动态画面展示
	connect(ui->horizontalSlider, &QSlider::sliderMoved, this, [=](int positon) {
		ui->label->setText(QString("%1/%2").arg(positon).arg(ui->horizontalSlider->maximum()));
		if (ui->horizontalSlider->sliderPosition() != positon)
		{
			ui->horizontalSlider->setSliderPosition(positon);
		}
	});

	//拖拽时，动态画面展示
	connect(ui->play, &QPushButton::toggled, this, [=](bool checked) {
		if (checked)
		{
			{
				std::lock_guard<std::mutex> locker(m_seek_mutex);
				if (m_yuv_file && m_yuv_file->atEnd()) {
					qDebug() << "repaly file:" << m_yuv_file->fileName();
					m_yuv_file->seek(0);
				}
			}
			m_pause_thread = false;
		}
	});

	connect(ui->pause, &QPushButton::toggled, this, [=](bool checked) {
		if (checked)
		{
			m_pause_thread = true;
		}
	});

	connect(ui->stop, &QPushButton::toggled, this, [=](bool checked) {
		
		if (checked)
		{
			m_pause_thread = true;
			{
				std::lock_guard<std::mutex> locker(m_seek_mutex);
				if(m_yuv_file)
					m_yuv_file->seek(m_yuv_file->size());
			}
			int maxStepNumber = ui->horizontalSlider->maximum();
			ui->horizontalSlider->setValue(maxStepNumber);
			ui->label->setText(QString("%1/%1").arg(maxStepNumber));
			ui->openGLWidget->clearTextureColor();
		}
	});
	connect(ui->comboBox, static_cast<void (QComboBox::*)(const QString & text)>(&QComboBox::currentIndexChanged), this, [=](const QString& text) {
		
		auto speed_grad = text.toFloat();//原子类型没有提供float，因此转换到int来使用
		if (speed_grad < 0)
		{
			m_speed_intelval = m_speed_intelval * speed_grad;
		}
		else
		{
			m_speed_intelval = m_speed_intelval / speed_grad;
		}
	});
}

PlayerMainForm::~PlayerMainForm()
{
	{
		std::lock_guard<std::mutex> locker(m_seek_mutex);
		m_pause_thread = true;
		m_bexit_thread = true;
	}

     if(m_read_yuv_data_thread && m_read_yuv_data_thread->joinable())
     {
         m_read_yuv_data_thread->join();
     }
    delete ui;
}

void PlayerMainForm::startReadYuv420FileThread(const QString& filename, int width, int height)
{
	ui->play->setChecked(true);
	m_yuv_width = width;
	m_yuv_height = height;

	m_read_yuv_data_thread = new std::thread([this](const QString& filename) {
		m_yuv_file = new QFile(filename);
		QByteArray data[3];
		if (m_yuv_file->open(QIODevice::ReadOnly))
		{ //read yuv
			assert(m_yuv_height != 0 && m_yuv_width != 0);
			int yuv_step = (uint64_t)m_yuv_height * m_yuv_width * 3 / 2;
			qint64 maxsize = m_yuv_file->size()/ yuv_step;
			ui->horizontalSlider->setMinimum(0);
			ui->horizontalSlider->setMaximum(maxsize);
			while (!m_bexit_thread)
			{
				while (!m_pause_thread)
				{
					if (m_yuv_file->atEnd()) {
						m_pause_thread = true;
						QMetaObject::invokeMethod(this, [this]() {
							ui->stop->setChecked(true);
							ui->stop->toggled(true);
						});
						qDebug() << "[PlayerMainForm::startReadYuv420File] Thread m_yuv_file->atEnd()";
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						continue;
					}
					{
						std::lock_guard<std::mutex> locker(m_seek_mutex);
						for (int i = 0; i < 3; i++)
						{
							if (i == 0) {
								data[i] = m_yuv_file->read(m_yuv_width * m_yuv_height);//附带seek操作
							}
							else {
								data[i] = m_yuv_file->read(m_yuv_width * m_yuv_height / 4);
							}
						}
						if (data[0].size() != m_yuv_width * m_yuv_height || data[1].size() != m_yuv_width * m_yuv_height / 4 || data[2].size() != m_yuv_width * m_yuv_height / 4)
						{
							qDebug() << "read file error" << endl;
							continue;
						}
					}
					//跨线程，刷新界面送往UI主线程，此时必须捕获data数组，如果只捕获data分量指针，有可能由于导致跨线程读写问题引起crash
					QMetaObject::invokeMethod(this, [this, maxsize, yuv_step, data]() mutable {
						int stride[3] = { m_yuv_width, m_yuv_width / 2, m_yuv_width / 2 };//填入stride
						uint8_t* yuvArr[3] = { (uint8_t*)(data[0].data()), yuvArr[1] = (uint8_t*)(data[1].data()), yuvArr[2] = (uint8_t*)(data[2].data()) };
						ui->openGLWidget->setTextureI420PData(yuvArr, stride, m_yuv_width, m_yuv_height);
						qint64 postion = m_yuv_file->pos();
						int currentFrameNum = postion / yuv_step;
						ui->horizontalSlider->setSliderPosition(currentFrameNum);
						ui->label->setText(QString("%1/%2").arg(currentFrameNum).arg(maxsize));
						});
					std::this_thread::sleep_for(std::chrono::milliseconds(m_speed_intelval));//除100 还原原来的倍数
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
	}, filename);
}

void PlayerMainForm::getCurrentPositionBackWardFrame()
{

}

void PlayerMainForm::getCurrentPositionNextFrame()
{

}


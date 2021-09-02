#include "QtConfigParameterDig.h"
#include <QFile>

QtConfigParameterDig::QtConfigParameterDig(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.width_lineEdit, &QLineEdit::textChanged, [this]() {
		previewFirstFrame(m_file_name, m_format, ui.width_lineEdit->text().toInt(), ui.height_lineEdit->text().toInt(), ui.fps_comboBox->currentText().toInt());
	});
	connect(ui.height_lineEdit, &QLineEdit::textChanged, [this]() {
		previewFirstFrame(m_file_name, m_format, ui.width_lineEdit->text().toInt(), ui.height_lineEdit->text().toInt(), ui.fps_comboBox->currentText().toInt());
	});

	connect(ui.fps_comboBox, static_cast<void(QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), [this]() {
		m_fps = ui.fps_comboBox->currentText().toInt();
	});
}

QtConfigParameterDig::~QtConfigParameterDig()
{
}

std::tuple<QString, int, int, int,int> QtConfigParameterDig::getSelectParameters()
{
	return std::tuple<QString, int, int, int, int>(m_file_name,m_width, m_height, m_format,m_fps);
}

void QtConfigParameterDig::previewFirstFrame(const QString& filename, FrameFormat format, int width, int height, int fps)
{
	m_fps = fps;
	m_width = width;
	m_height = height;
	m_format = format;
	m_file_name = filename;

	QSharedPointer<QFile> file(new QFile(filename), [](QFile* file) {
		file->close();
		delete file;
	});
	if (file->open(QIODevice::ReadOnly))
	{
		QByteArray data[3];
		for (int i = 0; i < 3; i++)
		{
			if (i == 0) {
				data[i] = file->read(width * height);//¸½´øseek²Ù×÷
			}
			else {
				data[i] = file->read(width * height / 4);
			}
		}
		if (data[0].size() != width * height || data[1].size() != width * height / 4 || data[2].size() != width * height / 4)
		{
			return;
		}

		uint8_t* yuvArr[3] = { (uint8_t*)(data[0].data()), yuvArr[1] = (uint8_t*)(data[1].data()), yuvArr[2] = (uint8_t*)(data[2].data()) };
		int stride[3] = { width, width / 2, width / 2 };//ÌîÈëstride
		ui.openGLWidget->setTextureI420PData(yuvArr, stride, width, height);
	}
}

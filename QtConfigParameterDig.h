#pragma once

#include <QDialog>
#include "ui_QtConfigParameterDig.h"
#include "YuvPlayerGlobal.h"

class QtConfigParameterDig : public QDialog
{
	Q_OBJECT
public:
	QtConfigParameterDig(QWidget *parent = Q_NULLPTR);
	~QtConfigParameterDig();
	std::tuple<QString, int, int, int, int>  getSelectParameters();
	void previewFirstFrame(const QString& filename, FrameFormat format, int width, int height, int fps);
private:
	Ui::QtConfigParameterDig ui;
	QString m_file_name;
	int m_width;
	int m_height;
	enum FrameFormat  m_format = IYUV_I420;
	int m_fps = 25;
};

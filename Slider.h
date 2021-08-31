#pragma once

#include <QSlider>

class Slider : public QSlider
{
	Q_OBJECT

public:
	Slider(QWidget *parent = Q_NULLPTR);
	~Slider();
	void mousePressEvent(QMouseEvent* ev);
	void mouseReleaseEvent(QMouseEvent* ev);
private:
};

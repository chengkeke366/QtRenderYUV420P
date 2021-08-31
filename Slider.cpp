#include "Slider.h"
#include <QCoreApplication>
#include <QMouseEvent>
#include <QStyleOptionSlider>
Slider::Slider(QWidget *parent)
	: QSlider(parent)
{
	
}

Slider::~Slider()
{
}

void Slider::mousePressEvent(QMouseEvent* event)
{
	QSlider::mousePressEvent(event);
	QStyleOptionSlider opt;
	initStyleOption(&opt);
	QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
	if (event->button() == Qt::LeftButton && sr.contains(event->pos()) == false)
	{
		if (orientation() == Qt::Vertical)
			setSliderPosition(minimum() + ((maximum() - minimum()) * (height() - event->y())) / height());
		else
			setSliderPosition(minimum() + ((maximum() - minimum()) * event->x()) / width());

		emit QSlider::sliderPressed();
	}
}

void Slider::mouseReleaseEvent(QMouseEvent* event)
{
	QSlider::mouseReleaseEvent(event);
	QStyleOptionSlider opt;
	initStyleOption(&opt);
	QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
	if (event->button() == Qt::LeftButton)
	{
		emit QSlider::sliderReleased();
	}
}

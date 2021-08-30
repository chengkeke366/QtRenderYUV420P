#include "QVideoRenderWidget.h"

#include <QApplication>
#include <iostream>
#include <thread>
#include <QMetaObject>
#include <QTimer>
#include <string>
#include <QFile>
#include <QMutex>
#include <QTimer>
#include <QMutexLocker>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	QVideoRenderWidget* w = new QVideoRenderWidget;
    w->show();

	std::thread thread([w]() {

		QFile* file = new QFile("C:/Users/ChengKeKe/Desktop/out852x480.yuv");
		QByteArray data[3];
		if (file->open(QIODevice::ReadOnly)) 
		{ //read yuv
			int width = 852;//填入自己视频yuv的宽高
			int height = 480;
			int stride[3] = { 852, 852 / 2, 852 / 2 };//填入stride
			while (1) 
			{
				if (file->atEnd()) {
					qDebug() << "repaly file:"<<file->fileName();
					file->seek(0);
					continue;
				}
				
				for (int i = 0; i < 3; i++)
				{
					if (i == 0){
						data[i] = file->read(width * height);	
					}
					else{
						data[i] = file->read(width * height / 4);	
					}
				}
				//跨线程，刷新界面送往UI主线程，此时必须捕获data数组，如果只捕获data分量指针，有可能由于导致跨线程读写问题引起crash
				QMetaObject::invokeMethod(w, [data, w, stride, width, height]() mutable {
					uint8_t* yuvArr[3] = { (uint8_t*)(data[0].data()), yuvArr[1] = (uint8_t*)(data[1].data()), yuvArr[2] = (uint8_t*)(data[2].data()) };
					w->setTextureI420PData(yuvArr, stride, width, height);
				});

				std::this_thread::sleep_for(std::chrono::milliseconds(40));
			}
		}
	});

	thread.detach();
    return a.exec();
}

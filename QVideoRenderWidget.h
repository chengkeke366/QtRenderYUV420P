#ifndef QVIDEORENDERWIDGET_H
#define QVIDEORENDERWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QFile>
#include <array>
#include <thread>
#include <mutex>

QT_BEGIN_NAMESPACE
namespace Ui { class QVideoRenderWidget; }
QT_END_NAMESPACE

class QOpenGLTexture;
class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject; //vao
class QOpenGLBuffer;

class QVideoRenderWidget : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT

public:
    QVideoRenderWidget(QWidget *parent = nullptr);
    ~QVideoRenderWidget();
    //对外接口，设置I420P数据
    void setTextureI420PData(uint8_t* Buffer[3], int Stride[3], int width, int height);
protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
private:
    QOpenGLTexture* m_texture_2d_array[3] = { nullptr,nullptr,nullptr };//yuv纹理
    QOpenGLShaderProgram* m_shaderProgram = nullptr; //shader program
    // vao
	QOpenGLVertexArrayObject* m_vao = nullptr;
    //vbo
    QOpenGLBuffer *m_vbo_yuv = nullptr;
    std::shared_ptr<uint8_t[]> m_yTexture_data;
    std::shared_ptr<uint8_t[]> m_uTexture_data;
    std::shared_ptr<uint8_t[]> m_vTexture_data;
    std::once_flag m_b_initTxture ;
};
#endif // QVIDEORENDERWIDGET_H

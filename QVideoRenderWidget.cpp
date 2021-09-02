#include "QVideoRenderWidget.h"
#include <QWeakPointer>
#include <QPointer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QTimer>

#include <thread>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <cstdint>

#define GLVERSION  "#version 330 core\n"
#define  GET_SHADER(arg) GLVERSION#arg

const char* vertex_shader = GET_SHADER(
	layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

	void main()
	{
		gl_Position = vec4(aPos, 1.0);
		TexCoord = aTexCoord;
	}
);

const char* frag_shader = GET_SHADER(
	out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D texY;
    uniform sampler2D texU;
    uniform sampler2D texV;

	void main()
	{
		vec3 rgb;
		vec3 yuv;
		yuv.x = texture(texY, TexCoord).r - 0.0625;
		yuv.y = texture(texU, TexCoord).r - 0.5;
		yuv.z = texture(texV, TexCoord).r - 0.5;

		rgb = mat3(1.164, 1.164,    1.164,
				   0.0,   -0.213,   2.114,
			       1.792, -0.534,   0.0) * yuv;

		FragColor = vec4(rgb, 1.0);
	}
);

QVideoRenderWidget::QVideoRenderWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

QVideoRenderWidget::~QVideoRenderWidget()
{
	// Make sure the context is current and then explicitly
	 // destroy all underlying OpenGL resources.
	makeCurrent();
    m_vao->destroy();
    m_vbo_yuv->destroy();
    delete m_vbo_yuv;
    delete m_vao;

    delete m_shaderProgram;
    doneCurrent();
}

void QVideoRenderWidget::setTextureI420PData(uint8_t* Buffer[3],int Stride[3], int width, int height)
{
    if (m_width!=width || m_height!=height)
    {
		makeCurrent();//If you need to call the standard OpenGL API functions from other places (e.g. in your widget's constructor or in your own paint functions), you must call makeCurrent() first.
//创建yuv 纹理
		for (int i = 0; i < 3; i++)
		{
			m_texture_2d_array[i] = QSharedPointer<QOpenGLTexture>( new QOpenGLTexture(QOpenGLTexture::Target2D));
			if (i == 0)
			{
				m_texture_2d_array[i]->setSize(width, height);
			}
			else
			{
				m_texture_2d_array[i]->setSize(width / 2, height / 2);
			}
			m_texture_2d_array[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
			m_texture_2d_array[i]->create();
			m_texture_2d_array[i]->setFormat(QOpenGLTexture::R8_UNorm);
			m_texture_2d_array[i]->allocateStorage();
		}
		doneCurrent();
    }

    m_yTexture_data = std::shared_ptr<uint8_t[]>(new uint8_t[width * height], std::default_delete<uint8_t[]>());
    m_uTexture_data = std::shared_ptr<uint8_t[]>(new uint8_t[width * height/4], std::default_delete<uint8_t[]>());
	m_vTexture_data = std::shared_ptr<uint8_t[]>(new uint8_t[width * height/4], std::default_delete<uint8_t[]>());

    //copy y
    const uint8_t* p_ydata = Buffer[0];
    uint8_t* ypoint = m_yTexture_data.get();
    for (int i=0; i<height; i++)
    {
        memcpy(ypoint, p_ydata, width);
        p_ydata = p_ydata + Stride[0];
        ypoint = ypoint + width;
    }

    // copy u
    const uint8_t* p_udata = Buffer[1];
    uint8_t* upoint = m_uTexture_data.get();
	for (int i = 0; i < height/2; i++)
	{
		memcpy(upoint, p_udata, width/2);
        p_udata = p_udata + Stride[1];
        upoint = upoint + width / 2;
	}

    // copy v
    const uint8_t* p_vdata = Buffer[2];
    uint8_t* vpoint = m_vTexture_data.get();
	for (int i = 0; i < height/2; i++)
	{
		memcpy(vpoint, p_vdata, width/2);
        p_vdata = p_vdata + Stride[2];
        vpoint = vpoint + width / 2;
	}
    update();
}

void QVideoRenderWidget::clearTextureColor()
{
    m_yTexture_data.reset();
    m_uTexture_data.reset();
    m_vTexture_data.reset();
    update();
}

void QVideoRenderWidget::initializeGL()
{
    initializeOpenGLFunctions();//Initializes OpenGL function resolution for the current context.

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    //设置背景色为黑色

    //makeCurrent();//使用当前窗口的上下文
    
    //创建shader program
    m_shaderProgram = new QOpenGLShaderProgram(this);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, frag_shader);

    m_shaderProgram->link();

    float vertices[] = {
        //顶点坐标                    纹理坐标
        -1.0f, -1.0f, 0.0f,           0.0f, 1.0f, //左下角
         1.0f, -1.0f, 0.0f,           1.0f, 1.0f, //右下角
        -1.0f,  1.0f, 0.0f,           0.0f, 0.0f, //左上角
         1.0f,  1.0f, 0.0f,           1.0f, 0.0f  //右上角
    };
    //创建VAO、VBO
    m_vao = new QOpenGLVertexArrayObject(this);
    m_vao->create();
    m_vao->bind();
	
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);//非常关键，设置对齐。如果stride不等于宽度，需要告诉gl数据的对齐方式。否则可能出现UV颜色与Y无法重叠的问题

    m_vbo_yuv = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbo_yuv->create();//相当于glGenBuffer
    m_vbo_yuv->bind();  //相当于glBindBuffer
    m_vbo_yuv->allocate(vertices, sizeof(vertices));//相当于glBufferData

    //告诉OpenGL如何读取顶点及纹理:首先得到属性的location，然后指定每个location的[数据类型、长度、读取步长、起始偏移量]这四个参数

    // 如果在顶点着色器中没有指定layout(location=) 这个说明，则可以使用该方法获取到属性的location
     uint32_t aPos = m_shaderProgram->attributeLocation("aPos"); 
     uint32_t aTexCoord = m_shaderProgram->attributeLocation("aTexCoord");
    
    //告诉Shader顶点坐标如何读取，相当于调用了glVertexAttribPointer。这部执行完，只是告诉GPU如何读取，还需要激活对应顶点属性，否则GPU还是无法读取到数据的
    m_shaderProgram->setAttributeBuffer(aPos,      GL_FLOAT,       0,           3, sizeof(float) * 5);
    //告诉Shader纹理坐标如何读取，相当于调用了glVertexAttribPointer
    m_shaderProgram->setAttributeBuffer(aTexCoord, GL_FLOAT, 3*sizeof(float),   2, sizeof(float) * 5);

    //通过location激活对应的顶点属性，那么GPU才能读取相应顶点属性。默认情况下即使
    m_shaderProgram->enableAttributeArray(aPos);
    m_shaderProgram->enableAttributeArray(aTexCoord);

    //此时已经完成了VBO与VAO绑定，可以对VBO和VAO进行解绑了
    m_vao->release();
    m_vbo_yuv->release();
}

void QVideoRenderWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void QVideoRenderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);//渲染glClearColor设置的颜色
    m_shaderProgram->bind();
    m_vao->bind();
	if (m_yTexture_data.use_count()!=0 && m_uTexture_data.use_count() != 0 && m_vTexture_data.use_count() != 0)
	{
		//告诉OpenGL每个着色器采样器属于哪个纹理单元，相当于调用 glUniform1i(glGetUniformLocation(ourShader.ID, "texture1"), 0);
		m_shaderProgram->setUniformValue("texY", 0);
		m_shaderProgram->setUniformValue("texU", 1);
		m_shaderProgram->setUniformValue("texV", 2);

		//设置YUV数据到纹理中

		m_texture_2d_array[0]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, m_yTexture_data.get());
		m_texture_2d_array[0]->bind(0);//激活纹理
		m_texture_2d_array[1]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, m_uTexture_data.get());
		m_texture_2d_array[1]->bind(1);
		m_texture_2d_array[2]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, m_vTexture_data.get());
		m_texture_2d_array[2]->bind(2);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);//使用VAO进行绘制

		m_texture_2d_array[0]->release();
		m_texture_2d_array[1]->release();
		m_texture_2d_array[2]->release();
	}
    
    m_vao->release();
    m_shaderProgram->release();
}


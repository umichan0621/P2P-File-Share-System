#include "ui_left_bar.h"
#include <QFile>
#include <QHBoxLayout>
constexpr int32_t LEFT_BAR_WIDTH = 76;

namespace gui
{
	LeftBarButton::LeftBarButton(QWidget* Parent)
		:QPushButton(Parent) {}

	void LeftBarButton::init(const QString& ImagePath)
	{
		setFixedSize(30, 30);
		setCursor(QCursor(Qt::PointingHandCursor));

		QFile QssButton("qss/left_bar.qss");
		QssButton.open(QFile::ReadOnly);
		QString Temp = QssButton.readAll();
		Temp.replace("IMAGE_PATH", ImagePath);
		setStyleSheet(Temp);
	}

	LeftBar::LeftBar(QWidget* Parent)
		:QWidget(Parent),
		m_pHome(new LeftBarButton(this)),
		m_pDownload(new LeftBarButton(this)),
		m_pShare(new LeftBarButton(this)),
		m_pAdd(new LeftBarButton(this)),
		m_pSetting(new LeftBarButton(this)),
		m_pAbout(new LeftBarButton(this))
	{
		init();
	}

	void LeftBar::init()
	{
		//添加按钮控件
		m_pHome->init("image/home.png");
		m_pDownload->init("image/download.png");
		m_pShare->init("image/upload.png");
		m_pAdd->init("image/add.png");
		m_pSetting->init("image/setting.png");
		m_pAbout->init("image/about.png");
		//设置布局
		QVBoxLayout* pLayout = new QVBoxLayout(this);
		QSpacerItem* pSpacer = new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
		pLayout->addItem(new QSpacerItem(0, 50, QSizePolicy::Minimum, QSizePolicy::Fixed));
		pLayout->addWidget(m_pHome);
		pLayout->addSpacing(20);
		pLayout->addWidget(m_pDownload);
		pLayout->addSpacing(20);
		pLayout->addWidget(m_pShare);
		pLayout->addSpacing(20);
		pLayout->addWidget(m_pAdd);
		pLayout->addItem(pSpacer);
		pLayout->addWidget(m_pSetting);
		pLayout->addSpacing(20);
		pLayout->addWidget(m_pAbout);
		//设置样式
		setFixedWidth(LEFT_BAR_WIDTH);
	}
}
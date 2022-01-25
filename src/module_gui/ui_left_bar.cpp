#include "ui_left_bar.h"
#include <QFile>
#include <QStyle>
#include <QVariant>
#include <QHBoxLayout>

constexpr int32_t LEFT_BAR_WIDTH = 76;

namespace gui
{
	LeftBarButton::LeftBarButton(QWidget* Parent)
		:QPushButton(Parent) {}

	void LeftBarButton::init(const QString& qssStyle, const QString& strType)
	{
		setFixedSize(30, 30);
		setCursor(QCursor(Qt::PointingHandCursor));
		setProperty("Type", strType);
		setStyleSheet(qssStyle);
	}

	LeftBar::LeftBar(QWidget* Parent)
		:QWidget(Parent),
		m_pMyFile(new LeftBarButton(this)),
		m_pDownload(new LeftBarButton(this)),
		m_pShare(new LeftBarButton(this)),
		m_pAdd(new LeftBarButton(this)),
		m_pSetting(new LeftBarButton(this)),
		m_pAbout(new LeftBarButton(this))
	{
		//读取Qss
		QFile QssButton("qss/left_bar.qss");
		QssButton.open(QFile::ReadOnly);
		QString qssStyle = QssButton.readAll();
		//添加按钮控件
		m_pMyFile->init(qssStyle, "my_file");
		m_pDownload->init(qssStyle, "download");
		m_pShare->init(qssStyle, "share");
		m_pAdd->init(qssStyle, "add");
		m_pSetting->init(qssStyle, "setting");
		m_pAbout->init(qssStyle, "about");
		//设置布局
		QVBoxLayout* pLayout = new QVBoxLayout(this);
		QSpacerItem* pSpacer = new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
		pLayout->addItem(new QSpacerItem(0, 50, QSizePolicy::Minimum, QSizePolicy::Fixed));
		pLayout->addWidget(m_pMyFile);
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
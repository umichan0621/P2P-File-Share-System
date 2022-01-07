#include "ui_top_bar.h"
#include <QFile>
#include <QHBoxLayout>
#include <base/logger/logger.h>

namespace gui
{
	TopBarButton::TopBarButton(QWidget* Parent)
		:QPushButton(Parent), m_bIsClose(false) {}

	void TopBarButton::init(const std::string& ImagePath, bool bIsClose)
	{
		m_ImagePath = QString::fromStdString(ImagePath);
		m_bIsClose = bIsClose;
		setFixedSize(45, 32);

	}

	void TopBarButton::set_style(const QString& Style)
	{
		QFile QssStyle("qss/top_bar.qss");
		QssStyle.open(QFile::ReadOnly);
		QString Temp = QssStyle.readAll();
		Temp.replace("IMAGE_PATH", m_ImagePath);
		setStyleSheet(Temp);
		if (m_bIsClose == true)
		{
			setObjectName("TopBarClose");
		}
		else
		{
			setObjectName("TopBar_" + Style);
		}
	}

	TopBar::TopBar(QWidget* Parent)
		:QWidget(Parent),
		m_pMin(new TopBarButton(this)),
		m_pMax(new TopBarButton(this)),
		m_pClose(new TopBarButton(this))
	{
		init();
	}

	void TopBar::init()
	{
		m_pMin->init("image/min.png");
		m_pMax->init("image/max.png");
		m_pClose->init("image/close.png", true);
		QHBoxLayout* pRightLayout = new QHBoxLayout(this);
		pRightLayout->addWidget(m_pMin);
		pRightLayout->addWidget(m_pMax);
		pRightLayout->addWidget(m_pClose);
		pRightLayout->setMargin(0);
		pRightLayout->setSpacing(0);
	}

	void TopBar::set_style(const QString& Style)
	{
		m_pMin->set_style(Style);
		m_pMax->set_style(Style);
		m_pClose->set_style(Style);
	}
}
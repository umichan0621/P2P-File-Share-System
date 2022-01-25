#include "ui_top_bar.h"
#include <QFile>
#include <QStyle>
#include <Qvariant>
#include <QHBoxLayout>

namespace gui
{
	TopBarButton::TopBarButton(QWidget* Parent)
		:QPushButton(Parent){}

	void TopBarButton::init(const std::string& strType)
	{
		m_strType = QString::fromStdString(strType);
		setProperty("Type", m_strType);
		if (m_strType == "close")
		{
			setProperty("IsClose", "close");
		}
		setFixedSize(45, 32);
	}

	void TopBarButton::set_style(const QString& qssStyle, const QString& Style)
	{
		setStyleSheet(qssStyle);
		if (m_strType != "close")
		{
			setProperty("IsClose", Style);
			style()->unpolish(this);
			style()->polish(this);
		}
	}

	TopBar::TopBar(QWidget* Parent)
		:QWidget(Parent),
		m_pMin(new TopBarButton(this)),
		m_pMax(new TopBarButton(this)),
		m_pClose(new TopBarButton(this))
	{
		//读取样式
		QFile QssStyle("qss/top_bar.qss");
		QssStyle.open(QFile::ReadOnly);
		m_qssStyle = QssStyle.readAll();

		m_pMin->init("min");
		m_pMax->init("max");
		m_pClose->init("close");
		QHBoxLayout* pRightLayout = new QHBoxLayout(this);
		pRightLayout->addWidget(m_pMin);
		pRightLayout->addWidget(m_pMax);
		pRightLayout->addWidget(m_pClose);
		pRightLayout->setMargin(0);
		pRightLayout->setSpacing(0);
	}

	void TopBar::set_style(const QString& Style)
	{
		m_pMin->set_style(m_qssStyle,Style);
		m_pMax->set_style(m_qssStyle, Style);
		m_pClose->set_style(m_qssStyle, Style);
	}
}
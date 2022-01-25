/***********************************************************************/
/* 名称:右上角Widget												   */
/* 说明:组合和初始化右上角的三个按钮								   */
/* 创建时间:2021/12/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <QPushButton>
#include <QWidget>

namespace gui
{
	class TopBarButton : public QPushButton
	{
		Q_OBJECT
	public:
		TopBarButton(QWidget* Parent = Q_NULLPTR);
		void init(const std::string& strType);
		void set_style(const QString& qssStyle, const QString& Style);
	private:
		QString		m_strType;
	};

	class TopBar : public QWidget
	{
		Q_OBJECT
	public:
		TopBar(QWidget* Parent = Q_NULLPTR);
		void set_style(const QString& Style);
	public:
		TopBarButton*	m_pMin;
		TopBarButton*	m_pMax;
		TopBarButton*	m_pClose;
		QString			m_qssStyle;
	};
}
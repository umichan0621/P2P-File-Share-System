/***********************************************************************/
/* 名称:QT左侧导航栏												   */
/* 说明:组合和初始化左侧按钮										   */
/* 创建时间:2021/12/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <QWidget>
#include <QPushButton>

namespace gui
{
	class LeftBarButton :public QPushButton
	{
		Q_OBJECT
	public:
		LeftBarButton(QWidget* Parent = Q_NULLPTR);
		void init(const QString& qssStyle, const QString& strType);
	};

	class LeftBar : public QWidget
	{
		Q_OBJECT
	public:
		LeftBar(QWidget* Parent = Q_NULLPTR);
	public:
		LeftBarButton* m_pMyFile;
		LeftBarButton* m_pDownload;
		LeftBarButton* m_pShare;
		LeftBarButton* m_pAdd;
		LeftBarButton* m_pSetting;
		LeftBarButton* m_pAbout;
	};
}
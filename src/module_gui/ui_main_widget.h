/***********************************************************************/
/* 名称:主窗体											               */
/* 说明:整个应用的UI												   */
/* 创建时间:2021/12/09												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <QWidget>
#include "ui_frameless_widget.h"
#include "ui_add_dialog.h"
#include "ui_top_bar.h"
#include "ui_left_bar.h"
#include "ui_download_list.h"
#include "ui_share_tree.h"

namespace gui
{
	class MainWidget : public FramelessWidget
	{
		Q_OBJECT
	private:
		enum Page
		{
			PAGE_NULL = 0,
			PAGE_HOME,
			PAGE_DOWNLOAD,
			PAGE_SHARE,
			PAGE_ADD,
			PAGE_SETTING,
			PAGE_ABOUT
		};
	public:
		MainWidget();
	public:
		void init();
		AddDialog* add_dialog();
		DownloadList* download_list();
		ShareTree* share_tree();
	private:
		void init_slots();	//连接各个按钮和槽函数
		void set_style();
	private://Event override
		void paintEvent(QPaintEvent* pEvent) override;
		void resizeEvent(QResizeEvent* pEvent) override;
	private:
		TopBar*				m_pTopBar;
		LeftBar*			m_pLeftBar;
		AddDialog*			m_pAddDialog;
		DownloadList*		m_pDownloadList;
		ShareTree*			m_pShareTree;
	private:
		QString				m_strStyle;
		QString				m_strLanguage;
	private:
		int8_t				m_CurPage;
		bool				m_bIsMax;
	};
}
/***********************************************************************/
/* 名称:UI-File Manager    											   */
/* 说明:添加右键功能的ListWidget							           */
/* 创建时间:2022/01/22												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QWidget>

namespace gui
{
	class FileListWidget :public QListWidget
	{
		Q_OBJECT
	public:
		FileListWidget(QWidget* Parent = Q_NULLPTR);
	private:
		void init();
		void init_menu();
		void init_slots();

		//重写右键调出菜单栏的事件
		void contextMenuEvent(QContextMenuEvent* pEvent) override;

	signals:
		void create_folder();
		void open_folder(int32_t FileSeq);
		void delete_file(int32_t FileSeq,int32_t ItemSeq);
	private:
		QMenu*				m_pBlankMenu;
		QMenu*				m_pItemMenu;
		QAction*			m_pNewFolder;
		QAction*			m_pShare;
		QAction*			m_pAdd;
		QAction*			m_pOpen;
		QAction*			m_pRename;
		QAction*			m_pDelete;
		QListWidgetItem*	m_pCurItem;
	};
}
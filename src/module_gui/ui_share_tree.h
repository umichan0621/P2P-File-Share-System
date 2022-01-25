/***********************************************************************/
/* 名称:分享页面											           */
/* 说明:显示和控制所有分享任务										   */
/* 创建时间:2021/12/09												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <unordered_map>
#include <QWidget>
#include "ui_share_component.h"

namespace gui
{
	class ShareTree :public QWidget
	{
		typedef std::unordered_map<int32_t, QTreeWidgetItem*> ItemMap;
		Q_OBJECT
	public:
		ShareTree(QWidget* Parent = Q_NULLPTR);
	public:
		void set_style(const QString& Style, const QString& Language);
	private:
		void init_slots();
		void init_sub_slots(FileNameGroup* pGroup);
	signals:
		void delete_file(int32_t FileSeq);
		void create_link(int32_t FileSeq);
		void open_folder(int32_t FileSeq);
		void new_share(int32_t FileSeq);
	private:
		TreeWidget*			m_pTree;
		FileNameGroup*		m_pCurGroup;
		ItemMap				m_pItemMap;
		std::mutex			m_Mutex;
		QString				m_strStyle;
		QString				m_strLanguage;
	};
}
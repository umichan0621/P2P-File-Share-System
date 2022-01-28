/***********************************************************************/
/* 名称:分享页面组件											       */
/* 说明:显示和控制分享页面的任务									   */
/* 创建时间:2021/12/09												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <QWidget>
#include <QCheckBox>
#include <QTreeWidget>
#include <QPushButton>
#include <QMouseEvent>

namespace gui
{
	class FileNameGroup :public QWidget
	{
		Q_OBJECT
	public:
		FileNameGroup(QWidget* Parent = Q_NULLPTR);
		void set_style(const QString& Style, const QString& Language);
		void init(int32_t FileSeq,QString FileName);
		void hide_button();
		void show_button();
	private:
		void init_solts();
	signals:
		void delete_me(int32_t FileSeq);
		void link_me(int32_t FileSeq);
		void folder_me(int32_t FileSeq);
	public:
		QCheckBox*		m_pCheckBox;
		QPushButton*	m_pLink;
		QPushButton*	m_pDelete;
		QPushButton*	m_pFolder;
		int32_t			m_FileSeq;
	};

	//重写enter和leave信号的QTreeWidget
	class TreeWidget : public QTreeWidget
	{
		Q_OBJECT
	public:
		TreeWidget(QWidget* Parent = Q_NULLPTR);
		QTreeWidgetItem* itemCur();
		void reset();
	private:
		void mouseMoveEvent(QMouseEvent* pEvent) override;
	signals:
		void itemLeft(QTreeWidgetItem* pItem);
		void itemEntered(QTreeWidgetItem* pItem);
	private:
		QTreeWidgetItem*	m_pCurItem;
	};
}
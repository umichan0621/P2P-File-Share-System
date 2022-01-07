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
		~FileNameGroup();
		static void load_qss();
		void set_style(const QString& Style, const QString& Language);
		void init(int32_t FileSeq,QString FileName);
		void init_solts();
		void show_button();
		void hide_button();
	signals:
		void delete_me(int32_t FileSeq);
		void link_me(int32_t FileSeq);
		void folder_me(int32_t FileSeq);

	public:
		QCheckBox*		m_pCheckBox;
		QPushButton*	m_pLink;
		QPushButton*	m_pDelete;
		QPushButton*	m_pFolder;
	private:
		int32_t			m_FileSeq;
	private:
		static QString	m_qssStyle;
	};

	
	class TreeWidget : public QTreeWidget
	{
		//当前类只为重写enter和leave信号
		Q_OBJECT
	public:
		TreeWidget(QWidget* Parent = Q_NULLPTR);
		QTreeWidgetItem* itemCur();
		void reset() { m_pCurItem = nullptr; }
	private:
		void mouseMoveEvent(QMouseEvent* pEvent) override;
	signals:
		void itemLeft(QTreeWidgetItem* pItem);
		void itemEntered(QTreeWidgetItem* pItem);
	private:
		QTreeWidgetItem*	m_pCurItem;
	};

	class ShareTree :public QWidget
	{
		typedef std::unordered_map<int32_t, QTreeWidgetItem*> ItemMap;
		Q_OBJECT
	public:
		ShareTree(QWidget* Parent = Q_NULLPTR);
	public:
		void init();
		void set_style(const QString& Style, const QString& Language);
	private:
		void init_slots();
		void init_sub_slots(FileNameGroup* pGroup);
	signals:
		void delete_file(int32_t FileSeq);
		void create_link(int32_t FileSeq);
		void open_folder(int32_t FileSeq);
		void new_share(int32_t FileSeq, const QString& FileName,
			const QString& Remark, const QString& CreateTime, uint64_t UploadData);
	private:
		TreeWidget*			m_pTree;
		FileNameGroup*		m_pCurGroup;
	private:
		ItemMap				m_pItemMap;
		std::mutex			m_Mutex;
	private:
		QString				m_strStyle;
		QString				m_strLanguage;
	};
}
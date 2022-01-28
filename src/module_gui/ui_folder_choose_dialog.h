/***********************************************************************/
/* 名称:MyFile-选择保存路径     									   */
/* 说明:选择文件系统的逻辑文件的路径							       */
/* 创建时间:2022/01/26												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QTreeWidget>
#include "ui_frameless_dialog.h"
#include "ui_my_file.h"

namespace gui
{
	class FolderChooseDialog :public FramelessDialog
	{
		Q_OBJECT
	public:
		FolderChooseDialog(QWidget* Parent = Q_NULLPTR);
		void set_style(const QString& Style, const QString& Language);

		//输入参数：整体的文件夹信息，当前文件/文件夹序号
		void show_dialog(const Folder& FolderInfo,int32_t FileSeq);
	private:
		void show_sub_folder(QTreeWidgetItem* m_pCurItem, const Folder& FolderInfo);
		void init_slots();
	signals:
		void file_move_to(int32_t FileSeq,int32_t FolderSeq);
	private:
		int32_t			m_FileSeq;
		QTreeWidget*	m_pFolderTree;
		QWidget*		m_pBottom;
		QPushButton*	m_pCreateFolder;
		QPushButton*	m_pCancel;
		QPushButton*	m_pConfirm;
		QString			m_qssStyle;
	};
}
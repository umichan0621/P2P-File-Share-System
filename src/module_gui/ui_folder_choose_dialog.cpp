#include "ui_folder_choose_dialog.h"
#include <QFile>
#include <QHBoxLayout>
#include <base/logger/logger.h>
#include <base/encoder.h>
#include <base/config.hpp>
#include <module_db/database.h>
#include <QHeaderView>
#include <QWindow>
#pragma warning(disable:26812)
#define SET_PROPERTY(QOBJ,PROPERTY,STYLE) QOBJ->setProperty(PROPERTY, STYLE);style()->unpolish(QOBJ);style()->polish(QOBJ)

namespace gui
{
	FolderChooseDialog::FolderChooseDialog(QWidget* Parent) :
		FramelessDialog(Parent),
		m_FileSeq(0),
		m_pFolderTree(new QTreeWidget(m_pDialog)),
		m_pBottom(new QWidget(m_pDialog)),
		m_pCreateFolder(new QPushButton(m_pDialog)),
		m_pCancel(new QPushButton(m_pDialog)),
		m_pConfirm(new QPushButton(m_pDialog))
	{
		QFile qssTotal("qss/folder_choose_dialog.qss");
		qssTotal.open(QFile::ReadOnly);
		m_qssStyle = qssTotal.readAll();
		QFile qssButton("qss/button_style.qss");
		qssButton.open(QFile::ReadOnly);
		QString qssButtonStyle = qssButton.readAll();

		m_pBottom->setStyleSheet(m_qssStyle);
		m_pDialog->setStyleSheet(m_qssStyle);
		m_pFolderTree->setStyleSheet(m_qssStyle);

		m_pCancel->setStyleSheet(qssButtonStyle);
		m_pConfirm->setStyleSheet(qssButtonStyle);
		m_pCreateFolder->setStyleSheet(qssButtonStyle);
		//设置固定布局
		int32_t Width = 400;
		int32_t Height = 300;
		int32_t Bounadry = 20;
		m_pBottom->setGeometry(0, Height - 60, Width, 60);
		m_pCreateFolder->setGeometry(Bounadry, 256, 100, 26);
		m_pCancel->setGeometry(230, 256, 70, 26);
		m_pConfirm->setGeometry(310, 256, 70, 26);
		m_pFolderTree->setGeometry(Bounadry, 40, Width - Bounadry * 2, Height - 98);
		m_pFolderTree->header()->hide();
		m_pFolderTree->setColumnCount(2);
		m_pFolderTree->hideColumn(1);
		init_slots();
	}

	void FolderChooseDialog::set_style(const QString& Style, const QString& Language)
	{
		m_pCreateFolder->setText(QString::fromLocal8Bit("新建文件夹"));
		m_pCancel->setText(QString::fromLocal8Bit("取消"));
		m_pConfirm->setText(QString::fromLocal8Bit("确认"));

		SET_PROPERTY(m_pDialog, "Background", Style);
		SET_PROPERTY(m_pBottom, "Bottom", Style);
		SET_PROPERTY(m_pCancel, "Cancel", Style);
		SET_PROPERTY(m_pConfirm, "Confirm", Style);
		SET_PROPERTY(m_pCreateFolder, "Cancel", Style);
		SET_PROPERTY(m_pFolderTree, "TreeWidget", Style);
	}


	void FolderChooseDialog::show_dialog(const Folder& FolderInfo, int32_t FileSeq)
	{
		m_FileSeq = FileSeq;
		//清除原来的
		while (nullptr != m_pFolderTree->takeTopLevelItem(0))
		{

		}
		QTreeWidgetItem* m_pCurItem = new QTreeWidgetItem();
		m_pFolderTree->addTopLevelItem(m_pCurItem);
		show_sub_folder(m_pCurItem, FolderInfo);
		m_pCurItem->setExpanded(true);
		show();
	}

	void FolderChooseDialog::init_slots()
	{
		connect(m_pCancel, &QPushButton::clicked, this, [&]()
			{
				hide();
			});

		connect(m_pConfirm, &QPushButton::clicked, this, [&]()
			{
				for (auto& Item : m_pFolderTree->selectedItems())
				{
					emit(file_move_to(m_FileSeq, Item->text(1).toInt()));
				}
				hide();
			});
	}

	void FolderChooseDialog::show_sub_folder(QTreeWidgetItem* m_pCurItem, const Folder& FolderInfo)
	{
		m_pCurItem->setIcon(0, QIcon("image/folder_my_file.svg"));
		m_pCurItem->setText(0, FolderInfo.FileName);
		m_pCurItem->setText(1, QString::number(FolderInfo.FileSeq));
		for (Folder CurSubFolder : FolderInfo.SubFolder)
		{
			if (m_FileSeq != CurSubFolder.FileSeq)
			{
				QTreeWidgetItem* pSubItem = new QTreeWidgetItem();
				m_pCurItem->addChild(pSubItem);
				show_sub_folder(pSubItem, CurSubFolder);
			}

		}
	}

}
#include "ui_file_manager.h"
#include <QFile>
#include <QMenu>
#include <QHBoxLayout>
#include <base/logger/logger.h>
#include <module_db/database.h>
#include "ui_list_component.h"

namespace gui
{
	FileListWidget::FileListWidget(QWidget* Parent) :
		QListWidget(Parent),
		m_pBlankMenu(new QMenu(this)),
		m_pItemMenu(new QMenu(this)),
		m_pNewFolder(new QAction(this)),
		m_pShare(new QAction(this)),
		m_pAdd(new QAction(this)),
		m_pOpen(new QAction(this)),
		m_pRename(new QAction(this)),
		m_pDelete(new QAction(this)),
		m_pCurItem(nullptr)
	{
		init();
	}

	void FileListWidget::init()
	{
		setViewMode(QListView::ViewMode::IconMode);
		setDragDropMode(QAbstractItemView::InternalMove);
		setResizeMode(QListView::Adjust);
		setMouseTracking(true);
		resize(1200, 700);
		init_menu();
		init_slots();
	}

	void FileListWidget::init_menu()
	{

		m_pNewFolder->setText(QString::fromLocal8Bit("新建文件夹"));
		m_pAdd->setText(QString::fromLocal8Bit("添加文件"));
		m_pShare->setText(QString::fromLocal8Bit("分享"));
		m_pOpen->setText(QString::fromLocal8Bit("打开"));
		m_pRename->setText(QString::fromLocal8Bit("重命名"));
		m_pDelete->setText(QString::fromLocal8Bit("删除"));

		m_pBlankMenu->addAction(m_pAdd);
		m_pBlankMenu->addAction(m_pNewFolder);
		m_pItemMenu->addAction(m_pOpen);
		m_pItemMenu->addAction(m_pShare);
		m_pItemMenu->addAction(m_pRename);
		m_pItemMenu->addAction(m_pDelete);

	}

	void FileListWidget::init_slots()
	{
		//连接new folder事件
		connect(m_pNewFolder, &QAction::triggered, this, [&]()
			{
				emit(create_folder());
			});
		
		//连接delete事件
		connect(m_pDelete, &QAction::triggered, this, [&]()
			{
				IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				if (nullptr != pCurWidget)
				{
					emit(delete_file(pCurWidget->file_seq(), row(m_pCurItem)));
				}
			});
		
		//连接open事件
		connect(m_pOpen, &QAction::triggered, this, [&]()
			{
				IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				if (nullptr != pCurWidget)
				{
					emit(open_folder(pCurWidget->file_seq()));
				}
			});

		//连接rename事件
		connect(m_pRename, &QAction::triggered, this, [&]()
			{
				LOG_ERROR << "RENAME";
				IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				if (nullptr != pCurWidget)
				{
					pCurWidget->rename_start();
				}
			});
		connect(m_pShare, &QAction::triggered, this, [&]()
			{

				//IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				//LOG_ERROR << pCurWidget->file_seq();
			});
		connect(this, &QListWidget::itemClicked, this, [&]
		(QListWidgetItem* pCurItem)
			{
				//之前有重命名的文件/文件夹，完成重命名
				if (nullptr != m_pCurItem)
				{
					IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
					if (nullptr != m_pCurItem)
					{
						pCurWidget->rename_over();
					}
				}
			});

		//连接双击item事件
		connect(this, &QListWidget::itemDoubleClicked, this, [&]
		(QListWidgetItem* pCurItem)
			{
				IconComponent* pCurWidget = (IconComponent*)itemWidget(pCurItem);
				if (nullptr != pCurWidget)
				{
					emit(open_folder(pCurWidget->file_seq()));
				}
			});
	}

	void FileListWidget::contextMenuEvent(QContextMenuEvent* pEvent)
	{
		//之前有重命名的文件/文件夹，完成重命名
		if (nullptr != m_pCurItem)
		{
			IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
			if (nullptr != m_pCurItem)
			{
				pCurWidget->rename_over();
			}
		}
		m_pCurItem = itemAt(mapFromGlobal(QCursor::pos()));
		//当前右键点击Item
		if (nullptr != m_pCurItem)
		{
			m_pItemMenu->exec(QCursor::pos());
		}
		//当前右键点击空白
		else
		{
			m_pBlankMenu->exec(QCursor::pos());
		}
		m_pCurItem = nullptr;
	}


}
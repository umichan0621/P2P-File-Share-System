#include "ui_file_manager.h"
#include <QFile>
#include <QMenu>
#include <QHBoxLayout>
#include <base/logger/logger.h>
#include <module_db/database.h>
#include <QVariant>
#include "ui_list_component.h"
#define SET_PROPERTY(QOBJ,PROPERTY,STYLE) QOBJ->setProperty(PROPERTY, STYLE);style()->unpolish(QOBJ);style()->polish(QOBJ)

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
		m_pUpload(new QAction(this)),
		m_pMove(new QAction(this)),
		m_pDeleteMessageBox(new QMessageBox(this)),
		m_pCurItem(nullptr)
	{
		QFile QssFile("qss/button_style.qss");
		QssFile.open(QFile::ReadOnly);
		QString qssButton = QssFile.readAll();
		setViewMode(QListView::ViewMode::IconMode);
		setDragDropMode(QAbstractItemView::InternalMove);
		setResizeMode(QListView::Adjust);
		setMouseTracking(true);
		resize(1200, 700);
		m_pDeleteMessageBox->setStandardButtons( QMessageBox::Yes| QMessageBox::No );
		m_pDeleteMessageBox->setDefaultButton(QMessageBox::No);
		m_pDeleteMessageBox->button(QMessageBox::Yes)->setFixedSize(60, 25);
		m_pDeleteMessageBox->button(QMessageBox::No)->setFixedSize(60, 25);
		m_pDeleteMessageBox->button(QMessageBox::Yes)->setStyleSheet(qssButton);
		m_pDeleteMessageBox->button(QMessageBox::No)->setStyleSheet(qssButton);
		m_pDeleteMessageBox->setFixedSize(300, 200);
		//m_pDeleteMessageBox->setStyleSheet("background:red;");
		init_menu();
		init_slots();
	}

	void FileListWidget::set_style(const QString& Style, const QString& Language)
	{
		m_pDeleteMessageBox->setWindowTitle(QString::fromLocal8Bit(""));
		m_pDeleteMessageBox->button(QMessageBox::Yes)->setText(QString::fromLocal8Bit("确认"));
		m_pDeleteMessageBox->button(QMessageBox::No)->setText(QString::fromLocal8Bit("取消"));

		SET_PROPERTY(m_pDeleteMessageBox->button(QMessageBox::Yes), "Confirm", Style);
		SET_PROPERTY(m_pDeleteMessageBox->button(QMessageBox::No), "Cancel", Style);
	}

	void FileListWidget::init_menu()
	{

		m_pAdd->setText(QString::fromLocal8Bit("X添加文件"));
		m_pUpload->setText(QString::fromLocal8Bit("X添加分享文件"));
		m_pNewFolder->setText(QString::fromLocal8Bit("新建文件夹"));

		m_pOpen->setText(QString::fromLocal8Bit("打开"));
		m_pRename->setText(QString::fromLocal8Bit("重命名"));
		m_pDelete->setText(QString::fromLocal8Bit("删除"));
		m_pShare->setText(QString::fromLocal8Bit("X分享"));
		m_pMove->setText(QString::fromLocal8Bit("移动到"));

		m_pBlankMenu->addAction(m_pAdd);
		m_pBlankMenu->addAction(m_pUpload);
		m_pBlankMenu->addAction(m_pNewFolder);
		m_pItemMenu->addAction(m_pOpen);
		m_pItemMenu->addAction(m_pShare);
		m_pItemMenu->addAction(m_pRename);
		m_pItemMenu->addAction(m_pDelete);
		m_pItemMenu->addAction(m_pMove);


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
					QString Message = QString::fromLocal8Bit("确定要删除\"");
					Message += pCurWidget->file_name();
					Message += QString::fromLocal8Bit("\"吗?");
					m_pDeleteMessageBox->setText(Message);
					if (QMessageBox::Yes == m_pDeleteMessageBox->exec())
					{
						emit(delete_file(pCurWidget->file_seq(), row(m_pCurItem)));
					}
					m_pCurItem = nullptr;
				}
			});
		
		//连接open事件
		connect(m_pOpen, &QAction::triggered, this, [&]()
			{
				IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				if (nullptr != pCurWidget)
				{
					emit(open_folder(pCurWidget->file_seq()));
					m_pCurItem = nullptr;
				}
			});

		//连接rename事件
		connect(m_pRename, &QAction::triggered, this, [&]()
			{
				IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				if (nullptr != pCurWidget)
				{
					pCurWidget->rename_start();
				}
			});
		//连接move事件
		connect(m_pMove, &QAction::triggered, this, [&]()
			{
				IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				if (nullptr != pCurWidget)
				{
					emit(move_file(pCurWidget->file_seq()));
					m_pCurItem = nullptr;
				}
			});
		connect(m_pShare, &QAction::triggered, this, [&]()
			{

				//IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				//LOG_ERROR << pCurWidget->file_seq();
			});
		connect(this, &QListWidget::itemClicked, this, [&]
		(QListWidgetItem* m_pCurItem)
			{
				//之前有重命名的文件/文件夹，完成重命名
				for (int32_t i = 0; i < count(); ++i)
				{
					IconComponent* pCurWidget = (IconComponent*)itemWidget(item(i));
					pCurWidget->rename_over();
				}
			});

		//连接双击item事件
		connect(this, &QListWidget::itemDoubleClicked, this, [&]
		(QListWidgetItem* m_pCurItem)
			{
				IconComponent* pCurWidget = (IconComponent*)itemWidget(m_pCurItem);
				if (nullptr != pCurWidget)
				{
					emit(open_folder(pCurWidget->file_seq()));
				}
			});
	}

	void FileListWidget::contextMenuEvent(QContextMenuEvent* pEvent)
	{
		//之前有重命名的文件/文件夹，完成重命名
		for (int32_t i = 0; i < count(); ++i)
		{
			IconComponent* pCurWidget = (IconComponent*)itemWidget(item(i));
			pCurWidget->rename_over();
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
	}


}
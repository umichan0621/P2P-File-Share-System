#include "ui_download_list.h"
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFile>
#include <base/config.hpp>
#include <base/logger/logger.h>
#include <module_db/database.h>
#define SET_PROPERTY(QOBJ,PROPERTY,STYLE) QOBJ->setProperty(PROPERTY, STYLE);style()->unpolish(QOBJ);style()->polish(QOBJ)

namespace gui
{
	DownloadList::DownloadList(QWidget* Parent)
		:QWidget(Parent),
		m_CurChoose(STATUS_DOWNLOAD),
		m_pDownloading(new QPushButton(this)),
		m_pPause(new QPushButton(this)),
		m_pComplete(new QPushButton(this)),
		m_pLineLow(new QFrame(this)),
		m_pListWidget(new QListWidget(this))
	{
		//初始化组件
	   //为子组件读取样式，避免重复读文件
		DownloadCard::load_qss();
		//按钮组件设置
		m_pDownloading->setGeometry(13, 5, 90, 47);
		m_pPause->setGeometry(103, 5, 90, 47);
		m_pComplete->setGeometry(193, 5, 90, 47);
		m_pDownloading->setCursor(QCursor(Qt::PointingHandCursor));
		m_pPause->setCursor(QCursor(Qt::PointingHandCursor));
		m_pComplete->setCursor(QCursor(Qt::PointingHandCursor));
		//设置可多选
		m_pListWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);
		//分隔线设置
		m_pLineLow->setFrameShape(QFrame::HLine);
		m_pLineLow->lower();
		//设置槽函数
		init_slots();
		setStyleSheet("background-color:transparent");
		QFile QssFile("qss/download_list.qss");
		QssFile.open(QFile::ReadOnly);
		QString QssStyle = QssFile.readAll();
		//设置List样式
		m_pListWidget->setStyleSheet(QssStyle);
		//分隔线样式
		m_pLineLow->setStyleSheet(QssStyle);
		//设置按钮样式
		m_pPause->setStyleSheet(QssStyle);
		m_pComplete->setStyleSheet(QssStyle);
		m_pDownloading->setStyleSheet(QssStyle);
		//滚动条样式
		QFile QssScrollFile("qss/scrollbar.qss");
		QssScrollFile.open(QFile::ReadOnly);
		m_pListWidget->verticalScrollBar()->setStyleSheet(QssScrollFile.readAll());

	}

	void DownloadList::init_slots()
	{
		//切换到正在下载文件界面
		connect(m_pDownloading, &QPushButton::clicked, this, [&]()
			{
				std::lock_guard<std::mutex> Lock(m_DownloadListMutex);
				if (m_CurChoose == STATUS_DOWNLOAD)
				{
					return;
				}
				m_CurChoose = STATUS_DOWNLOAD;
				for (auto& Item : m_DownloadWidgetMap)
				{
					DownloadCard* pCur = (DownloadCard*)m_pListWidget->itemWidget(Item.second);
					if (nullptr == pCur)
					{
						continue;
					}
					//只是隐藏，不是删除
					m_pListWidget->setItemHidden(Item.second, pCur->status() != STATUS_DOWNLOAD);
				}
				set_button_style();
			});
		//切换到暂停下载文件界面
		connect(m_pPause , &QPushButton::clicked, this, [&]()
			{
				std::lock_guard<std::mutex> Lock(m_DownloadListMutex);
				if (m_CurChoose == STATUS_PAUSE)
				{
					return;
				}
				m_CurChoose = STATUS_PAUSE;
				for (auto& Item : m_DownloadWidgetMap)
				{
					DownloadCard* pCur = (DownloadCard*)m_pListWidget->itemWidget(Item.second);
					if (nullptr == pCur)
					{
						continue;
					}
					//只是隐藏，不是删除
					m_pListWidget->setItemHidden(Item.second, pCur->status() != STATUS_PAUSE);
				}
				set_button_style();
			});
		//切换到已完成下载文件界面
		connect(m_pComplete, &QPushButton::clicked, this, [&]()
			{
				std::lock_guard<std::mutex> Lock(m_DownloadListMutex);
				if (m_CurChoose == STATUS_COMPLETE)
				{
					return;
				}
				m_CurChoose = STATUS_COMPLETE;
				for (auto& Item : m_DownloadWidgetMap)
				{
					DownloadCard* pCur = (DownloadCard*)m_pListWidget->itemWidget(Item.second);
					if (nullptr == pCur)
					{
						continue;
					}
					//只是隐藏，不是删除
					m_pListWidget->setItemHidden(Item.second, pCur->status() != STATUS_COMPLETE);
				}
				set_button_style();
			});
		//向Gui添加新的下载任务
		connect(this, &DownloadList::new_download, this,
			[&](int32_t FileSeq)
			{
				uint8_t Status = 0;
				uint64_t FileSize = 0;
				std::string FileName;
				std::unique_lock<std::mutex> Lock(m_DownloadListMutex);
				//当前文件序号没有被使用
				if (0 == m_DownloadWidgetMap.count(FileSeq))
				{
					bool bRes = g_pDataBaseManager->select_file_info(FileSeq, Status, FileSize, FileName);
					if (false == bRes)
					{
						return;
					}
					//初始化组件
					DownloadCard* pCurCard = new DownloadCard();
					pCurCard->init(FileSeq);
					//加入ListWidget
					QListWidgetItem* pItem = new QListWidgetItem();
					pItem->setSizeHint(QSize(0, 120));
					m_pListWidget->addItem(pItem);
					m_pListWidget->setItemWidget(pItem, pCurCard);
					//设置Style
					pCurCard->set_style(m_strStyle, m_strLanguage);
					//设置连接信号和槽
					init_sub_slots(pCurCard);
					m_DownloadWidgetMap[FileSeq] = pItem;
					Lock.unlock();
					pCurCard->set_file_info(QString::fromStdString(FileName), FileSize);
					pCurCard->set_file_progress(0, 0);
					pCurCard->set_status(Status);
				}
			});
		//向Gui更新下载任务进度
		connect(this, &DownloadList::update_progress, this,
			[&](int32_t FileSeq, uint64_t CurFileSize, uint64_t CurSpeed)
			{
				std::unique_lock<std::mutex> Lock(m_DownloadListMutex);
				if (0 != m_DownloadWidgetMap.count(FileSeq))
				{
					QListWidgetItem* pItem = m_DownloadWidgetMap[FileSeq];
					DownloadCard* pCurCard = (DownloadCard*)m_pListWidget->itemWidget(pItem);
					pCurCard->set_file_progress(CurFileSize, CurSpeed);
				}
			});
		//文件下载完成
		connect(this, &DownloadList::file_complete, this,
			[&](int32_t FileSeq)
			{
				std::unique_lock<std::mutex> Lock(m_DownloadListMutex);
				if (0 != m_DownloadWidgetMap.count(FileSeq))
				{
					QListWidgetItem* pItem = m_DownloadWidgetMap[FileSeq];
					DownloadCard* pCurCard = (DownloadCard*)m_pListWidget->itemWidget(pItem);
					Lock.unlock();
					pCurCard->set_status(STATUS_COMPLETE);
				}
			});
	}

	void DownloadList::init_sub_slots(DownloadCard* pCurWidget)
	{
		//收到子组件的状态改变信号
		connect(pCurWidget, &DownloadCard::status_change, this,
			[&](int32_t FileSeq)
			{
				std::lock_guard<std::mutex> Lock(m_DownloadListMutex);
				if (m_DownloadWidgetMap.count(FileSeq) == 0)
				{
					return;
				}
				QListWidgetItem* pItem = m_DownloadWidgetMap[FileSeq];
				DownloadCard* pCurWidget = (DownloadCard*)m_pListWidget->itemWidget(pItem);
				uint8_t CurStatus = pCurWidget->status();
				m_pListWidget->setItemHidden(pItem, false);
				//当前任务已完成，但页面不在已完成
				if (CurStatus != m_CurChoose)
				{
					m_pListWidget->setItemHidden(pItem, true);
				}
			});
		//收到子组件的开始信号
		connect(pCurWidget, &DownloadCard::start_me, this,
			[&](int32_t FileSeq)
			{
				std::lock_guard<std::mutex> Lock(m_DownloadListMutex);
				if (m_CurChoose == STATUS_PAUSE)
				{
					if (m_DownloadWidgetMap.count(FileSeq) != 0)
					{
						m_pListWidget->setItemHidden(m_DownloadWidgetMap[FileSeq], true);
					}
				}
				//通知控制器任务已开始
				emit(start_file(FileSeq));
			});
		//收到子组件的暂停信号
		connect(pCurWidget, &DownloadCard::pause_me, this,
			[&](int32_t FileSeq)
			{
				std::lock_guard<std::mutex> Lock(m_DownloadListMutex);
				if (m_CurChoose == STATUS_DOWNLOAD)
				{
					if (m_DownloadWidgetMap.count(FileSeq) != 0)
					{
						m_pListWidget->setItemHidden(m_DownloadWidgetMap[FileSeq], true);
					}
				}
				//通知控制器任务已暂停
				emit(pause_file(FileSeq));
			});
		//收到子组件的删除信号
		connect(pCurWidget, &DownloadCard::delete_me, this,
			[&](int32_t FileSeq)
			{
				std::lock_guard<std::mutex> Lock(m_DownloadListMutex);
				if (m_DownloadWidgetMap.count(FileSeq) == 0)
				{
					return;
				}
				QListWidgetItem* pItem = m_DownloadWidgetMap[FileSeq];
				DownloadCard* pCurWidget = (DownloadCard*)m_pListWidget->itemWidget(pItem);
				//从ListWidget中删除
				m_pListWidget->takeItem(m_pListWidget->row(pItem));
				delete pCurWidget;
				delete pItem;
				m_DownloadWidgetMap.erase(FileSeq);
				//通知控制器任务已删除
				emit(delete_file(FileSeq));
			});
		//收到子组件的打开文件位置信号
		connect(pCurWidget, &DownloadCard::folder_me, this,
			[&](int32_t FileSeq)
			{
				//通知控制器打开文件位置
				emit(open_folder(FileSeq));
			});
		//收到子组件的生成当前文件的链接信号
		connect(pCurWidget, &DownloadCard::link_me, this,
			[&](int32_t FileSeq)
			{
				//通知控制器生成当前文件的链接
				emit(create_link(FileSeq));
			});
		//收到子组件的详细信息信号
		connect(pCurWidget, &DownloadCard::info_me, this,
			[&](int32_t FileSeq)
			{
				//emit(link_file(FileSeq));
			});
	}

	void DownloadList::set_style(const QString& Style, const QString& Language)
	{
		m_strStyle = Style;
		m_strLanguage = Language;
		QScrollBar* pScrollBar = m_pListWidget->verticalScrollBar();
		SET_PROPERTY(pScrollBar, "Style", m_strStyle);

		SET_PROPERTY(m_pPause, "ButtonStyle", m_strStyle);
		SET_PROPERTY(m_pComplete, "ButtonStyle", m_strStyle);
		SET_PROPERTY(m_pDownloading, "ButtonStyle", m_strStyle);
		for (auto& Item : m_DownloadWidgetMap)
		{
			DownloadCard* pCurWidget = (DownloadCard*)m_pListWidget->itemWidget(Item.second);
			pCurWidget->set_style(m_strStyle, m_strLanguage);
		}
		set_button_style();
	}

	void DownloadList::set_button_style()
	{
		m_pDownloading->setText(QString::fromLocal8Bit("下载中"));
		m_pPause->setText(QString::fromLocal8Bit("等待中"));
		m_pComplete->setText(QString::fromLocal8Bit("已完成"));

		if (STATUS_DOWNLOAD == m_CurChoose)
		{
			SET_PROPERTY(m_pPause, "ButtonStatus", "unchoose" );
			SET_PROPERTY(m_pComplete, "ButtonStatus", "unchoose" );
			SET_PROPERTY(m_pDownloading, "ButtonStatus", "choose" );
		}
		else if (STATUS_PAUSE == m_CurChoose)
		{
			SET_PROPERTY(m_pPause, "ButtonStatus", "choose" );
			SET_PROPERTY(m_pComplete, "ButtonStatus", "unchoose" );
			SET_PROPERTY(m_pDownloading, "ButtonStatus", "unchoose" );
		}
		else if (STATUS_COMPLETE == m_CurChoose)
		{
			SET_PROPERTY(m_pPause, "ButtonStatus", "unchoose");
			SET_PROPERTY(m_pComplete, "ButtonStatus", "choose" );
			SET_PROPERTY(m_pDownloading, "ButtonStatus", "unchoose" );
		}
	}

	void DownloadList::resizeEvent(QResizeEvent* pEvent)
	{
		m_pLineLow->setGeometry(13, 50, width() - 35, 2);
		m_pListWidget->setGeometry(3, 52, width() - 6, height() - 52);
	}
}
#include "ui_main_widget.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <base/logger/logger.h>
#include <base/config.hpp>

namespace gui
{
	MainWidget::MainWidget() :
		m_pTopBar(new TopBar(this)),
		m_pLeftBar(new LeftBar(this)),
		m_pAddDialog(new AddDialog(this)),
		m_pFolderChooseDialog(new FolderChooseDialog(this)),
		m_pMyFile(new MyFile(this)),
		m_pDownloadList(new DownloadList(this)),
		m_pShareTree(new ShareTree(this)),
		m_CurPage(PAGE_NULL),
		m_bIsMax(false),
		m_strStyle("dark")
	{
		m_pAddDialog->raise();
		m_pFolderChooseDialog->raise();
		set_boundary(8, 32);
		//QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
		////设置阴影距离
		//shadow->setOffset(0, 0);
		////设置阴影颜色
		//shadow->setColor(QColor("#444444"));
		////设置阴影圆角
		//shadow->setBlurRadius(8);
		////给嵌套QWidget设置阴影
		//setGraphicsEffect(shadow);
		////给垂直布局器设置边距(此步很重要, 设置宽度为阴影的宽度)
		//pVLayout->setMargin(8);
		setMinimumWidth(640);
		setMinimumHeight(440);
		resize(1280, 720);
		init_slots();
		m_pDownloadList->hide();
		m_pShareTree->hide();
		m_pAddDialog->hide();
		m_pFolderChooseDialog->hide();
		m_pMyFile->show();
		m_CurPage = PAGE_MY_FILE;
		m_pDownloadList->hide();
		//读取样式表
		QFile Background("qss/main_widget.qss");
		Background.open(QFile::ReadOnly);
		setStyleSheet(Background.readAll());
		set_style();
	}

	void MainWidget::init_slots()
	{
		//右上角按钮
		//最小化/还原
		connect(m_pTopBar->m_pMin, &QPushButton::clicked, this, [&]()
			{
				if (windowState() != Qt::WindowMinimized)
				{
					setWindowState(Qt::WindowMinimized);
				}
			});
		//最大化/还原
		connect(m_pTopBar->m_pMax, &QPushButton::clicked, this, [&]()
			{
				if (false == m_bIsMax)
				{
					setWindowState(Qt::WindowMaximized);
				}
				else
				{
					setWindowState(Qt::WindowNoState);
				}
				m_bIsMax = !m_bIsMax;
			});
		//关闭窗口
		connect(m_pTopBar->m_pClose, &QPushButton::clicked, this, [&]()
			{
				close(); 
			});
		//左侧按钮
		//切换到My FIle页面
		connect(m_pLeftBar->m_pMyFile, &QPushButton::clicked, this, [&]()
			{
				if (PAGE_MY_FILE != m_CurPage)
				{
					m_pMyFile->show();
					m_pDownloadList->hide();
					m_pShareTree->hide();
					m_CurPage = PAGE_MY_FILE;
				}

			});
		//切换到Download页面
		connect(m_pLeftBar->m_pDownload, &QPushButton::clicked, this, [&]()
			{
				if (PAGE_DOWNLOAD != m_CurPage)
				{
					m_pDownloadList->show();
					m_pShareTree->hide();
					m_pMyFile->hide();
					m_CurPage = PAGE_DOWNLOAD;
				}

			});
		//切换到Share页面
		connect(m_pLeftBar->m_pShare, &QPushButton::clicked, this, [&]()
			{
				if (PAGE_SHARE != m_CurPage)
				{
					m_pShareTree->show();
					m_pDownloadList->hide();
					m_pMyFile->hide();
					m_CurPage = PAGE_SHARE;
				}
			});

		connect(m_pLeftBar->m_pAdd, &QPushButton::clicked, this, [&]()
			{
				m_pAddDialog->set_style(m_strStyle, "ch");
				m_pAddDialog->set_path_download(QString::fromStdString(g_pConfig->path_download()));
				m_pAddDialog->set_path_share(QString::fromStdString(g_pConfig->path_share()));
				m_pAddDialog->set_size(600, 270, width(), height());
				if (PAGE_SHARE == m_CurPage)
				{
					m_pAddDialog->set_mode(ADD_SHARE);

				}
				else
				{
					m_pAddDialog->set_mode(ADD_FILE);
				}
				
				m_pAddDialog->show();
			});


		connect(m_pLeftBar->m_pSetting, &QPushButton::clicked, this, [&]()
			{
				set_style();
			});

		//收到移动文件的信号，调出对话框
		connect(m_pMyFile, &MyFile::file_move, this, [&](int32_t FileSeq)
			{
				m_pFolderChooseDialog->set_size(400,300,width(),height());
				m_pFolderChooseDialog->show_dialog(m_pMyFile->folder_info(), FileSeq);

			});

		}

	void MainWidget::set_style()
	{
		if ("light" == m_strStyle)
		{
			m_strStyle = "dark";
		}
		else
		{
			m_strStyle = "light";
		}
		setProperty("Style", m_strStyle);
		style()->unpolish(this);
		style()->polish(this);

		m_pTopBar->set_style(m_strStyle);
		m_pMyFile->set_style(m_strStyle, m_strLanguage);
		m_pDownloadList->set_style(m_strStyle, m_strLanguage);
		m_pShareTree->set_style(m_strStyle, m_strLanguage);
		m_pAddDialog->set_style(m_strStyle, m_strLanguage);
		m_pFolderChooseDialog->set_style(m_strStyle, m_strLanguage);

		update();
	}

	AddDialog* MainWidget::add_dialog()
	{
		return m_pAddDialog;
	}
	
	FolderChooseDialog* MainWidget::folder_choose_dialog()
	{
		return m_pFolderChooseDialog;
	}

	DownloadList* MainWidget::download_list()
	{
		return m_pDownloadList;
	}

	ShareTree* MainWidget::share_tree()
	{
		return m_pShareTree;
	}

	MyFile* MainWidget::my_file()
	{
		return m_pMyFile;
	}

	//Event override
	void MainWidget::paintEvent(QPaintEvent* pEvent)
	{
		QPainter Painter(this);
		//绘制左侧栏的颜色
		if ("light" == m_strStyle)
		{
			Painter.setBrush(QColor("#333333"));
			Painter.setPen(QColor("#333333"));
		}
		else
		{
			Painter.setBrush(QColor("#191919"));
			Painter.setPen(QColor("#191919"));
		}
		Painter.drawRect(0, 0, 76, height());
	}

	void MainWidget::resizeEvent(QResizeEvent* pEvent)
	{
		m_pTopBar->setGeometry(width() - 135, 0, 135, 32);
		m_pLeftBar->setGeometry(14, 32, 30, height() - 52);
		m_pMyFile->setGeometry(106, 40, width() - 126, height() - 50);
		m_pDownloadList->setGeometry(106, 40, width() - 126, height() - 50);
		m_pShareTree->setGeometry(106, 40, width() - 126, height() - 50);
	}
	//Event override
}
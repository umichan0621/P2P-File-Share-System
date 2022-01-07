#include "ui_main_widget.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <base/logger/logger.h>
#include <base/config.hpp>

#define CONNECT_BUTTON(_BUTTON,_FUNC) connect(_BUTTON, &QPushButton::clicked, this, _FUNC)

namespace gui
{
	MainWidget::MainWidget() :
		m_pTopBar(new TopBar(this)),
		m_pLeftBar(new LeftBar(this)),
		m_pAddDialog(new AddDialog(this)),
		m_pDownloadList(new DownloadList(this)),
		m_pShareTree(new ShareTree(this)),
		m_bIsMax(false),
		m_strStyle("dark"){}

	void MainWidget::init()
	{
		m_pAddDialog->raise();
		set_boundary(8, 32);
		//setMouseTracking(true);
		//QWidget* pTemp
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
		//m_pTopBar->hide();
		//pWorkingArea->hide();
		//m_pShareList->setStyleSheet("background-color:#444444");
		setMinimumWidth(640);
		setMinimumHeight(440);
		resize(1280, 720);
		init_slots();
		set_style();
		m_pDownloadList->hide();
		m_pShareTree->hide();
		m_pAddDialog->hide();
		m_CurPage = PAGE_DOWNLOAD;
		m_pDownloadList->show();
		//读取样式表
		QFile Background("qss/main_widget.qss");
		Background.open(QFile::ReadOnly);
		setStyleSheet(Background.readAll());
	}

	void MainWidget::init_slots()
	{
		//右上角按钮
		//最小化/还原
		CONNECT_BUTTON(m_pTopBar->m_pMin, [&]()
			{
				if (windowState() != Qt::WindowMinimized)
				{
					setWindowState(Qt::WindowMinimized);
				}
			});
		//最大化/还原
		CONNECT_BUTTON(m_pTopBar->m_pMax, [&]() 
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
		CONNECT_BUTTON(m_pTopBar->m_pClose, [&]() 
			{
				close(); 
			});
		//左侧按钮
		//切换到Download页面
		CONNECT_BUTTON(m_pLeftBar->m_pDownload, [&]() 
			{
				if (PAGE_DOWNLOAD != m_CurPage)
				{
					m_pDownloadList->show();
					m_pShareTree->hide();
					m_CurPage = PAGE_DOWNLOAD;
				}

			});
		//切换到Share页面
		CONNECT_BUTTON(m_pLeftBar->m_pShare, [&]()
			{
				if (PAGE_SHARE != m_CurPage)
				{
					m_pShareTree->show();
					m_pDownloadList->hide();
					m_CurPage = PAGE_SHARE;
				}
			});
		CONNECT_BUTTON(m_pLeftBar->m_pAdd, [&]()
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


		CONNECT_BUTTON(m_pLeftBar->m_pSetting, [&]() 
			{
				set_style();
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
		this->setProperty("Style", m_strStyle);
		LOG_ERROR << m_strStyle.toStdString();
		style()->unpolish(this);

		style()->polish(this);
		m_pTopBar->set_style(m_strStyle);
		m_pDownloadList->set_style(m_strStyle, m_strLanguage);
		m_pShareTree->set_style(m_strStyle, m_strLanguage);
		m_pAddDialog->set_style(m_strStyle, m_strLanguage);

		
		
		
		update();
	}

	AddDialog* MainWidget::add_dialog()
	{
		return m_pAddDialog;
	}

	DownloadList* MainWidget::download_list()
	{
		return m_pDownloadList;
	}

	ShareTree* MainWidget::share_tree()
	{
		return m_pShareTree;
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
		m_pDownloadList->setGeometry(106, 40, width() - 126, height() - 50);
		m_pShareTree->setGeometry(106, 40, width() - 126, height() - 50);
	}
	//Event override
}
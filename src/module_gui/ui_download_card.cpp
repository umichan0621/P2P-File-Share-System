#include "ui_download_card.h"
#include <QFile>
#include <QHBoxLayout>
#include <QStyle>
#include <QVariant>
#include <base/config.hpp>
#include <base/logger/logger.h>
#define CONNECT_BUTTON(_BUTTON,_FUNC) connect(_BUTTON, &QPushButton::clicked, this, _FUNC)
#define DISCONNECT_BUTTON(_BUTTON,_FUNC) disconnect(_BUTTON, &QPushButton::clicked, this, _FUNC)

QString gui::DownloadCard::m_qssStyle = "";

namespace gui
{
	TopRightButton::TopRightButton(QWidget* Parent) :
		QPushButton(Parent) {}

	void TopRightButton::init(const QString& ImagePath)
	{
		setFixedSize(32, 26);
		setCursor(QCursor(Qt::PointingHandCursor));
		setProperty("ToolBarButton", ImagePath);
		style()->unpolish(this);
		style()->polish(this);
	}

	DownloadCard::DownloadCard(QWidget* Parent) :
		QWidget(Parent),
		m_pDownloadProgress(new QProgressBar(this)),
		m_pFileName(new QLabel(this)),
		m_pDownloadByte(new QLabel(this)),
		m_pDownloadSpeed(new QLabel(this)),
		m_pToolBar(new QWidget(this)),
		m_pStartPause(new TopRightButton(m_pToolBar)),
		m_pDelete(new TopRightButton(m_pToolBar)),
		m_pFolder(new TopRightButton(m_pToolBar)),
		m_pLink(new TopRightButton(m_pToolBar)),
		m_pInfo(new TopRightButton(m_pToolBar)),
		m_FileSeq(-1),
		m_FileSize(0),
		m_Status(STATUS_NULL) {}

	void DownloadCard::load_qss()
	{
		//读取样式
		QFile QssFile("qss/download_list.qss");
		QssFile.open(QFile::ReadOnly);
		m_qssStyle = QssFile.readAll();
	}

	void DownloadCard::init(int32_t FileSeq)
	{
		m_FileSeq = FileSeq;
		//初始化按钮
		m_pStartPause->init("pause");
		m_pDelete->init("close");
		m_pFolder->init("folder");
		m_pLink->init("link");
		m_pInfo->init("info");
		//设置按钮控件布局
		QHBoxLayout* pTopRightLayout = new QHBoxLayout(m_pToolBar);
		pTopRightLayout->setMargin(0);
		pTopRightLayout->setSpacing(0);
		pTopRightLayout->addSpacing(10);
		pTopRightLayout->addWidget(m_pStartPause);
		pTopRightLayout->addWidget(m_pDelete);
		pTopRightLayout->addWidget(m_pLink);
		pTopRightLayout->addWidget(m_pFolder);
		pTopRightLayout->addWidget(m_pInfo);
		pTopRightLayout->addSpacing(10);
		//设置整体布局
		QGridLayout* pLayout = new QGridLayout(this);
		pLayout->addWidget(m_pFileName, 0, 0, Qt::AlignLeft | Qt::AlignBottom);
		pLayout->addWidget(m_pToolBar, 0, 1, Qt::AlignRight | Qt::AlignBottom);
		pLayout->addWidget(m_pDownloadByte, 2, 0, Qt::AlignLeft | Qt::AlignTop);
		pLayout->addWidget(m_pDownloadSpeed, 2, 1, Qt::AlignRight | Qt::AlignTop);
		pLayout->addWidget(m_pDownloadProgress, 1, 0, 1, 2, Qt::AlignBottom);
		pLayout->setContentsMargins(20, 10, 20, 0);
		//设置工具栏高度
		m_pToolBar->setFixedHeight(26);
		//设置进度条样式
		m_pDownloadProgress->setFormat("");

		//设置按钮样式
		m_pStartPause->setStyleSheet(m_qssStyle);
		m_pDelete->setStyleSheet(m_qssStyle);
		m_pFolder->setStyleSheet(m_qssStyle);
		m_pLink->setStyleSheet(m_qssStyle);
		m_pInfo->setStyleSheet(m_qssStyle);
		//设置文件名样式
		m_pFileName->setStyleSheet(m_qssStyle);
		//设置下载信息样式
		m_pDownloadByte->setStyleSheet(m_qssStyle);
		m_pDownloadSpeed->setStyleSheet(m_qssStyle);
		//设置工具栏样式
		m_pToolBar->setStyleSheet(m_qssStyle);
		//设置进度条样式
		m_pDownloadProgress->setStyleSheet(m_qssStyle);
		//连接槽函数
		init_slots();
		setFixedHeight(110);
	}

	void DownloadCard::init_slots()
	{
		CONNECT_BUTTON(m_pStartPause, &DownloadCard::do_pause);
		CONNECT_BUTTON(m_pDelete, [&]() {emit(delete_me(m_FileSeq)); });
		CONNECT_BUTTON(m_pFolder, [&]() {emit(folder_me(m_FileSeq)); });
		CONNECT_BUTTON(m_pLink, [&]() {emit(link_me(m_FileSeq)); });
		CONNECT_BUTTON(m_pInfo, [&]() {emit(info_me(m_FileSeq)); });
	}

	void DownloadCard::set_status(uint8_t Status)
	{
		if (STATUS_COMPLETE == m_Status)
		{
			return;
		}
		QString Path;
		if (STATUS_PAUSE== Status)
		{
			DISCONNECT_BUTTON(m_pStartPause, &DownloadCard::do_pause);
			CONNECT_BUTTON(m_pStartPause, &DownloadCard::do_start);
			m_pStartPause->init("start");
			m_pDownloadProgress->setProperty("Progress", "pause_" + m_strStyle);
		}
		else if (STATUS_DOWNLOAD== Status)
		{
			DISCONNECT_BUTTON(m_pStartPause, &DownloadCard::do_start);
			CONNECT_BUTTON(m_pStartPause, &DownloadCard::do_pause);
			m_pStartPause->init("pause");
			m_pDownloadProgress->setProperty("Progress", m_strStyle);

		}
		else if (STATUS_COMPLETE== Status )
		{
			DISCONNECT_BUTTON(m_pStartPause, &DownloadCard::do_pause);
			DISCONNECT_BUTTON(m_pStartPause, &DownloadCard::do_start);
			set_file_progress(m_FileSize, 0);
			m_pDownloadProgress->setProperty("Progress", "complete_" + m_strStyle);
		}
		style()->unpolish(m_pDownloadProgress);
		style()->polish(m_pDownloadProgress);
		m_Status = Status;

		//通知上层界面任务状态已改变
		emit(status_change(m_FileSeq));
	}

	uint8_t DownloadCard::status()
	{
		return m_Status;
	}

	void DownloadCard::set_style(const QString& Style, const QString& Language)
	{
		{
			load_qss();
			//设置按钮样式
			m_pStartPause->setStyleSheet(m_qssStyle);
			m_pDelete->setStyleSheet(m_qssStyle);
			m_pFolder->setStyleSheet(m_qssStyle);
			m_pLink->setStyleSheet(m_qssStyle);
			m_pInfo->setStyleSheet(m_qssStyle);
		}
		m_strStyle = Style;
		m_strLanguage = Language;
		//设置各个控件的属性并刷新
		m_pFileName->setProperty("FileName", Style);
		style()->unpolish(m_pFileName);
		style()->polish(m_pFileName);
		m_pDownloadByte->setProperty("DownloadInfo", Style);
		style()->unpolish(m_pDownloadByte);
		style()->polish(m_pDownloadByte);
		m_pDownloadSpeed->setProperty("DownloadInfo", Style);
		style()->unpolish(m_pDownloadSpeed);
		style()->polish(m_pDownloadSpeed);
		m_pToolBar->setProperty("ToolBar", Style);
		style()->unpolish(m_pToolBar);
		style()->polish(m_pToolBar);

		if (STATUS_NULL != m_Status)
		{
			set_status(m_Status);
		}
	}

	void DownloadCard::set_file_info(const QString& FileName, uint64_t FileSize)
	{
		m_pFileName->setText(FileName);
		m_FileSize = FileSize;
		m_strFileSize = file_size(FileSize);
		m_pDownloadByte->setText(QString::fromStdString("0/" + m_strFileSize));
	}

	void DownloadCard::set_file_progress(uint64_t CurFileSize, uint64_t CurSpeed)
	{
		if (0 == m_FileSize)
		{
			return;
		}
		int32_t Percent = CurFileSize / (m_FileSize / 100);
		m_pDownloadProgress->setValue(Percent);
		m_pDownloadByte->setText(QString::fromStdString(file_size(CurFileSize) + "/" + m_strFileSize));
		m_pDownloadSpeed->setText(QString::fromStdString(file_size(CurSpeed) + "/s"));
	}

	std::string DownloadCard::file_size(uint64_t FileSize)
	{
		if (FileSize >= 0x40000000)
		{
			double SizeGB = FileSize * 1.0 / 0x40000000;
			std::string Res = std::to_string(SizeGB);
			int32_t Pos = Res.find('.');
			if (Pos >= 0)
			{
				int32_t Len = Res.size() < Pos + 3 ? Res.size() : Pos + 3;
				Res = Res.substr(0, Len);
			}
			return Res + "GB";
		}
		else if (FileSize >= 0x100000)
		{
			double SizeMB = FileSize * 1.0 / 0x100000;
			std::string Res = std::to_string(SizeMB);
			int32_t Pos = Res.find('.');
			if (Pos >= 0)
			{
				int32_t Len = Res.size() < Pos + 3 ? Res.size() : Pos + 3;
				Res = Res.substr(0, Len);
			}
			return Res + "MB";
		}
		else if (FileSize >= 0x400)
		{
			double SizeKB = FileSize * 1.0 / 0x400;
			std::string Res = std::to_string(SizeKB);
			int32_t Pos = Res.find('.');
			if (Pos >= 0)
			{
				int32_t Len = Res.size() < Pos + 3 ? Res.size() : Pos + 3;
				Res = Res.substr(0, Len);
			}
			return Res + "KB";
		}
		return std::to_string(FileSize) + "B";
	}

	//slot函数
	void DownloadCard::do_start()
	{
		m_Status = STATUS_DOWNLOAD;
		m_pStartPause->init("pause");
		DISCONNECT_BUTTON(m_pStartPause, &DownloadCard::do_start);
		CONNECT_BUTTON(m_pStartPause, &DownloadCard::do_pause);
		m_pDownloadProgress->setProperty("Progress", m_strStyle);
		style()->unpolish(m_pDownloadProgress);
		style()->polish(m_pDownloadProgress);
		emit(start_me(m_FileSeq));
	}

	void DownloadCard::do_pause()
	{
		m_Status = STATUS_PAUSE;
		m_pStartPause->init("start");
		DISCONNECT_BUTTON(m_pStartPause, &DownloadCard::do_pause);
		CONNECT_BUTTON(m_pStartPause, &DownloadCard::do_start);
		//设置进度条样式
		m_pDownloadProgress->setProperty("Progress", "pause_" + m_strStyle);
		style()->unpolish(m_pDownloadProgress);
		style()->polish(m_pDownloadProgress);
		emit(pause_me(m_FileSeq));
	}
	//slot函数
}
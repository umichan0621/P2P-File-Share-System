/***********************************************************************/
/* 名称:下载页面											           */
/* 说明:显示和控制所有下载任务										   */
/* 创建时间:2021/12/09												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <stdint.h>
#include <unordered_map>
#include <QPushButton>
#include <QWidget.h>
#include <QListWidget.h>
#include "ui_download_card.h"
namespace gui
{
	class DownloadList :public QWidget
	{
		Q_OBJECT
	private:
		typedef std::unordered_map<int32_t, QListWidgetItem*> DownloadWidgetMap;
	public:
		DownloadList(QWidget* Parent = Q_NULLPTR);
	public:
		void set_style(const QString& Style, const QString& Language);
	signals:
		//转发下层Download Card信号的信号
		void start_file(int32_t FileSeq);
		void pause_file(int32_t FileSeq);
		void delete_file(int32_t FileSeq);
		void create_link(int32_t FileSeq);
		void open_folder(int32_t FileSeq);
		//接受外部状态改变信号的信号
		void new_download(int32_t FileSeq);
		void update_progress(int32_t FileSeq, uint64_t CurFileSize, uint64_t CurSpeed);
		void file_complete(int32_t FileSeq);
	private:
		void init_slots();
		void init_sub_slots(DownloadCard* pCurWidget);
		void set_button_style();
	private:
		void resizeEvent(QResizeEvent* pEvent);
	private:
		QFrame*				m_pLineLow;
		QListWidget*		m_pListWidget;
		QPushButton*		m_pDownloading;
		QPushButton*		m_pPause;
		QPushButton*		m_pComplete;
	private:
		std::mutex			m_DownloadListMutex;
		DownloadWidgetMap	m_DownloadWidgetMap;
		uint8_t				m_CurChoose;
	private:
		QString				m_strStyle;
		QString				m_strLanguage;
	};
}
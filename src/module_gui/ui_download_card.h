/***********************************************************************/
/* 名称:下载页面组件											       */
/* 说明:显示下载状态和控制下载任务								       */
/* 创建时间:2021/12/09												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <stdint.h>
#include <string>
#include <QLabel>
#include <QWidget>
#include <QProgressBar>
#include <QPushButton>

namespace gui
{
	class TopRightButton :public QPushButton
	{
		Q_OBJECT
	public:
		TopRightButton(QWidget* Parent = Q_NULLPTR);
		void init(const QString& ImagePath);
	};

	class DownloadCard : public QWidget
	{
		Q_OBJECT
	public:
		DownloadCard(QWidget* Parent = Q_NULLPTR);
	public:
		static void load_qss();
		void init(int32_t FileSeq);
		void set_style(const QString& Style, const QString& Language);
		void set_file_info(const QString& FileName, uint64_t FileSize);
		void set_file_progress(uint64_t CurFileSize, uint64_t CurSpeed);
		void set_status(uint8_t Status);
		uint8_t status();
	private:
		void init_slots();
		std::string file_size(uint64_t FileSize);	//计算文件大小，以B/KB/MB/GB显示
	private:
		void do_start();
		void do_pause();
	signals:
		void status_change(int32_t FileSeq);
		void start_me(int32_t FileSeq);
		void pause_me(int32_t FileSeq);
		void delete_me(int32_t FileSeq);
		void folder_me(int32_t FileSeq);
		void link_me(int32_t FileSeq);
		void info_me(int32_t FileSeq);
	private://进度显示
		QProgressBar* m_pDownloadProgress;
		QLabel* m_pFileName;
		QLabel* m_pDownloadByte;
		QLabel* m_pDownloadSpeed;
	private://任务管理按钮
		QWidget* m_pToolBar;
		TopRightButton* m_pStartPause;
		TopRightButton* m_pDelete;
		TopRightButton* m_pFolder;
		TopRightButton* m_pLink;
		TopRightButton* m_pInfo;
	private://任务信息
		int32_t				m_FileSeq;
		uint64_t			m_FileSize;
		std::string			m_strFileSize;
		uint8_t				m_Status;		//任务状态
	private:
		QString				m_strStyle;
		QString				m_strLanguage;
	private:
		static QString		m_qssStyle;
	};
}
/***********************************************************************/
/* 名称:MyFile    												       */
/* 说明:展示和管理整个文件系统的逻辑页面							   */
/* 创建时间:2022/01/22												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QHBoxLayout>
#include "ui_file_manager.h"

namespace gui
{
	class PathButton :public QPushButton
	{
		Q_OBJECT
	public:
		PathButton(QWidget* Parent = Q_NULLPTR);
		void set_file_seq(int32_t FileSeq);
	signals:
		void button_clicked(int32_t FileSeq);
	private:
		int32_t m_FileSeq;
	};

	class MyFile :public QWidget
	{
		Q_OBJECT
		struct FileInfo
		{
			uint8_t			Type=0;
			uint64_t		FileSize=0;
			int32_t			Parent=0;
			std::string		FileName;
			std::string		WriteTime;
		};
	private:
		typedef std::unordered_map<int32_t, std::vector<int32_t>>			
											FolderMap;	//文件Seq->子文件
		typedef std::unordered_map<int32_t, FileInfo>						
											FileMap;	//文件Seq->文件信息
	public:
		MyFile(QWidget* Parent = Q_NULLPTR);
		void set_style(const QString& Style, const QString& Language);
		void load_qss();

	private:
		void init();
		void init_slots();


		//显示当前目录下的指定文件
		void show_file(int32_t FileSeq);

		//刷新路径选择的控件
		void refresh_path();

		//刷新当前目录下的文件
		void refresh();
	private:
		void resizeEvent(QResizeEvent* pEvent);
	signals:
		void add_file(int32_t FileSeq);
	private:
		int32_t					m_pCurFolder;
		QFrame*					m_pLineUp;
		QFrame*					m_pLineLow;
		FileListWidget*			m_pFileManager;
		QPushButton*			m_pBack;
		QPushButton*			m_pNext;
		QWidget*				m_pChoosePath;
		QHBoxLayout* m_pChoosePathLayout;
		std::mutex					m_MyFileMutex;
		FolderMap					m_FolderMap;
		FileMap						m_FileMap;
		std::vector<int32_t>		m_pPreFolder;
		std::vector<int32_t>		m_pNextFolder;
		std::vector<QPushButton*>	m_FolderPath;
		QString						m_qssStyle;
		QString						m_strStyle;
	};
}
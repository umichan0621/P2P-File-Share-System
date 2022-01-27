/***********************************************************************/
/* 名称:新建下载任务对话框											   */
/* 说明:新建下载任务												   */
/* 创建时间:2021/12/10												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include "ui_frameless_dialog.h"

namespace gui
{
	enum ADD_DIALOG
	{
		ADD_NULL = 0,
		ADD_FILE,
		ADD_PEER,
		ADD_SHARE
	};

	class AddDialog : public FramelessDialog
	{
		Q_OBJECT
	public:
		AddDialog(QWidget* Parent = Q_NULLPTR);
	public:
		void set_mode(uint8_t Mode);
		void set_path_download(const QString& DefaultPath);
		void set_path_share(const QString& DefaultPath);
		void set_style(const QString& Style, const QString& Language);
	private:
		void init_slots();
		void set_top_button();
	signals:
		void add_download_file(const QString& strLink, const QString& strPath);
		void add_share_file(const QString& strRemark, const QString& strPath);
	private://固定组件
		QPushButton*		m_pAddFile;
		QPushButton*		m_pAddShare;
		QPushButton*		m_pAddPeer;
		QFrame*				m_pLineLow;
		QWidget*			m_pBottom;
		QPushButton*		m_pCancel;
		QPushButton*		m_pSubmit;
	private://下载组件
		QWidget*			m_pDownload;
		QTextEdit*			m_pDownloadTextEdit;
		QLineEdit*			m_pDownloadLineEdit;
		QPushButton*		m_pDownloadSetPath;
		QLabel*				m_pDownloadSavePath;
	private://分享组件
		QWidget*			m_pShare;
		QTextEdit*			m_pShareTextEdit;
		QLineEdit*			m_pShareLineEdit;
		QPushButton*		m_pShareSetPath;
		QLabel*				m_pShareFilePath;
	private://Peer组件
		QWidget*			m_pPeer;
	private:
		QString				m_strStyle;
		QString				m_strLanguage;
	private:
		uint8_t				m_CurChoose;
	};
}
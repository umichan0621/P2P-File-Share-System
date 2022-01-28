/***********************************************************************/
/* 名称:List Component    											   */
/* 说明:UI-File Manager的Item组件						               */
/* 创建时间:2022/01/22												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QWidget>

namespace gui
{
	//ListWidget使用Icon模式下的组件
	class IconComponent :public QWidget
	{
		Q_OBJECT
	public:
		IconComponent(QWidget* Parent = Q_NULLPTR);
	public:
		void init(int32_t FileSeq,QString FileName,int8_t FileType);
		int32_t file_seq();
		QString file_name();
		void set_style(const QString& qssStyle, const QString& Style);
		void rename_start();
		void rename_over();
	private:
		//如果长度不够显示带省略号的文件名
		QString get_file_name();
		void init_slots();
	signals:
		void rename_file(int32_t FileSeq, const QString& strFileName);
	private:
		int32_t				m_FileSeq;
		QString				m_strFileName;
		QLabel*				m_pIcon;
		QLabel*				m_pFileName;
		QLineEdit*			m_pRename;
	};
}
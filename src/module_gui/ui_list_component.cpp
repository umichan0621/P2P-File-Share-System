#include "ui_list_component.h"
#include <QFile>
#include <QHBoxLayout>
#include <base/logger/logger.h>
#include <base/config.hpp>
#include <module_db/database.h>

namespace gui
{
	IconComponent::IconComponent(QWidget* Parent):
		QWidget(Parent),
		m_FileSeq(0),
		m_pIcon(new QLabel(this)),
		m_pFileName(new QLabel(this)),
		m_pRename(new QLineEdit(this))
	{}

	void IconComponent::init(int32_t FileSeq, QString FileName, int8_t FileType)
	{
		m_FileSeq = FileSeq;
		m_strFileName = FileName;
		m_pFileName->setGeometry(0, 90, 120, 20);
		m_pRename->setGeometry(0, 90, 120, 20);
		m_pFileName->setAlignment(Qt::AlignCenter);
		m_pRename->setAlignment(Qt::AlignCenter);
		m_pRename->hide();

		//设定文件/文件夹显示的图标
		if (STATUS_FOLDER == FileType || STATUS_FOLDER_SHARE == FileType)
		{
			m_pIcon->setGeometry(20, 10, 80, 80);
			m_pIcon->setProperty("FileType", "folder");
		}
		else if (STATUS_OFFLINE == FileType)
		{
			m_pIcon->setGeometry(30, 22, 60, 60);
			m_pIcon->setProperty("FileType", "file_offline");
		}
		else
		{
			m_pIcon->setGeometry(30, 22, 60, 60);
			m_pIcon->setProperty("FileType", "file");
		}

		m_pFileName->setText(get_file_name());
		m_pRename->setText(m_strFileName);
		init_slots();
	}

	int32_t IconComponent::file_seq()
	{
		return m_FileSeq;
	}

	void IconComponent::rename_start()
	{
		m_pRename->show();
		m_pRename->setFocus();
		m_pRename->selectAll();
	}

	void IconComponent::rename_over()
	{
		m_pRename->hide();
		QString NewFileName=m_pRename->text();
		if (NewFileName != m_strFileName)
		{
			m_strFileName = NewFileName;
			m_pFileName->setText(get_file_name());
			emit(rename_file(m_FileSeq, NewFileName));
		}
	}

	void IconComponent::set_style(const QString& qssStyle, const QString& Style)
	{
		m_pFileName->setProperty("FileName", Style);
		m_pFileName->setStyleSheet(qssStyle);
		m_pIcon->setStyleSheet(qssStyle);
		style()->unpolish(m_pFileName);
		style()->polish(m_pFileName);
	}

	void IconComponent::init_slots()
	{
		connect(m_pRename, &QLineEdit::returnPressed, this, [&]() 
			{
				rename_over();
			});
	}

	QString IconComponent::get_file_name()
	{
		QString FileName= m_strFileName;
		QFontMetrics Font(m_pFileName->font());
		int32_t FontSize = Font.width(FileName);
		if (FontSize > m_pFileName->width())
		{
			//返回一个带有省略号的字符串
			FileName = Font.elidedText(FileName, Qt::ElideRight, 100, Qt::TextShowMnemonic);
		}
		return FileName;
	}
}
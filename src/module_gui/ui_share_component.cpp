#include "ui_share_component.h"
#include <QFile>
#include <QHBoxLayout>

constexpr int32_t ROW_HEIGHT = 35;

namespace gui
{
	FileNameGroup::FileNameGroup(QWidget* Parent) :
		m_pCheckBox(new QCheckBox(this)),
		m_pLink(new QPushButton(this)),
		m_pDelete(new QPushButton(this)),
		m_pFolder(new QPushButton(this)),
		m_FileSeq(0)
	{}

	void FileNameGroup::init(int32_t FileSeq, QString FileName)
	{
		m_FileSeq = FileSeq;
		setFixedHeight(ROW_HEIGHT);
		//设置布局
		QHBoxLayout* pLayout = new QHBoxLayout(this);
		pLayout->setContentsMargins(0, 0, 0, 0);
		pLayout->addWidget(m_pCheckBox);
		pLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pLayout->addWidget(m_pLink);
		pLayout->addWidget(m_pDelete);
		pLayout->addWidget(m_pFolder);
		hide_button();
		m_pCheckBox->setText(FileName);
		//设置按钮样式
		QFile QssFile("qss/share_tree.qss");
		QssFile.open(QFile::ReadOnly);
		QString qssStyle = QssFile.readAll();

		m_pCheckBox->setStyleSheet(qssStyle);
		m_pLink->setStyleSheet(qssStyle);
		m_pDelete->setStyleSheet(qssStyle);
		m_pFolder->setStyleSheet(qssStyle);
		m_pLink->setProperty("Button", "link");
		m_pDelete->setProperty("Button", "close");
		m_pFolder->setProperty("Button", "folder");

		init_solts();
	}

	void FileNameGroup::init_solts()
	{
		connect(m_pLink, &QPushButton::clicked, this, [&]()
			{
				emit(link_me(m_FileSeq));
			});
		connect(m_pDelete, &QPushButton::clicked, this, [&]()
			{
				emit(delete_me(m_FileSeq));
			});
		connect(m_pFolder, &QPushButton::clicked, this,[&]()
			{
				emit(folder_me(m_FileSeq));
			});
	}

	void FileNameGroup::set_style(const QString& Style, const QString& Language)
	{
		m_pCheckBox->setProperty("CheckBox", Style);
		style()->unpolish(m_pCheckBox);
		style()->polish(m_pCheckBox);
	}

	void FileNameGroup::show_button()
	{
		m_pLink->show();
		m_pDelete->show();
		m_pFolder->show();
	}

	void FileNameGroup::hide_button()
	{
		m_pLink->hide();
		m_pDelete->hide();
		m_pFolder->hide();
	}

	TreeWidget::TreeWidget(QWidget* Parent)
		:QTreeWidget(Parent),
		m_pCurItem(nullptr)
	{
		setMouseTracking(true);
	}
	
	void TreeWidget::mouseMoveEvent(QMouseEvent* pEvent)
	{
		if (pEvent->y() <= 2 || //上界
			pEvent->y() >= height() - 37 || //下届
			pEvent->x() <= 2 || //左界
			pEvent->x() >= width() - 16)//右界
		{
			if (nullptr != m_pCurItem)
			{
				emit(itemLeft(m_pCurItem));
				m_pCurItem = nullptr;
			}
			return;
		}
		QTreeWidgetItem* pCurItem = itemAt(pEvent->x(), pEvent->y());
		if (nullptr == pCurItem)//当前在空item
		{
			if (nullptr != pCurItem)//之前不为空
			{
				emit(itemLeft(m_pCurItem));
				m_pCurItem = nullptr;
			}
		}
		else
		{
			if (nullptr == m_pCurItem)//之前在空的item
			{
				m_pCurItem = pCurItem;
				emit(itemEntered(m_pCurItem));
			}
			else if (m_pCurItem != pCurItem)//离开前一个
			{
				emit(itemLeft(m_pCurItem));
				m_pCurItem = pCurItem;
				emit(itemEntered(m_pCurItem));
			}
		}
	}

	QTreeWidgetItem* TreeWidget::itemCur()
	{
		return m_pCurItem;
	}

	void TreeWidget::reset() 
	{ 
		m_pCurItem = nullptr; 
	}

}
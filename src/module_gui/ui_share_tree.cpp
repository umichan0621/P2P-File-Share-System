#include "ui_share_tree.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QFile>
#include <base/config.hpp>
#include <base/logger/logger.h>
constexpr int32_t ROW_HEIGHT = 35;
QString gui::FileNameGroup::m_qssStyle = "";
#define CONNECT_BUTTON(_BUTTON,_FUNC) connect(_BUTTON, &QPushButton::clicked, this, _FUNC)

namespace gui
{
	FileNameGroup::FileNameGroup(QWidget* Parent) :
		m_pCheckBox(new QCheckBox(this)),
		m_pLink(new QPushButton(this)),
		m_pDelete(new QPushButton(this)),
		m_pFolder(new QPushButton(this)),
		m_FileSeq(0)
	{}

	FileNameGroup::~FileNameGroup()
	{
		//LOG_ERROR << "~";
		//m_pCheckBox->disconnect();
		//m_pLink->disconnect();
		//m_pDelete->disconnect();
		//m_pFolder->disconnect();
		//disconnect();
	}

	void FileNameGroup::load_qss()
	{
		QFile QssFile("qss/share_tree.qss");
		QssFile.open(QFile::ReadOnly);
		m_qssStyle = QssFile.readAll();
	}

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
		m_pCheckBox->setStyleSheet(m_qssStyle);
		m_pLink->setStyleSheet(m_qssStyle);
		m_pDelete->setStyleSheet(m_qssStyle);
		m_pFolder->setStyleSheet(m_qssStyle);
		m_pLink->setProperty("Button", "link");
		m_pDelete->setProperty("Button", "close");
		m_pFolder->setProperty("Button", "folder");

		init_solts();
	}

	void FileNameGroup::init_solts()
	{
		CONNECT_BUTTON(m_pLink, [&]()
			{
				emit(link_me(m_FileSeq));
			});
		CONNECT_BUTTON(m_pDelete, [&]()
			{
				emit(delete_me(m_FileSeq));
			});
		CONNECT_BUTTON(m_pFolder, [&]()
			{
				emit(folder_me(m_FileSeq));
			});
	}

	void FileNameGroup::set_style(const QString& Style, const QString& Language)
	{
		load_qss();
		m_pCheckBox->setStyleSheet(m_qssStyle);

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
			if (nullptr != m_pCurItem)//之前不为空
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
			else if (pCurItem != m_pCurItem)//离开前一个
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

	ShareTree::ShareTree(QWidget* Parent)
		:QWidget(Parent),
		m_pTree(new TreeWidget(this)),
		m_pCurGroup(nullptr)
	{
		init();
	}

	void ShareTree::init()
	{
		//为子组件读取样式，避免重复读文件
		FileNameGroup::load_qss();
		//设置布局
		QVBoxLayout* pLayout = new QVBoxLayout(this);
		pLayout->addWidget(m_pTree);
		pLayout->addSpacing(20);
		pLayout->setContentsMargins(0, 0, 0, 0);
		m_pTree->setColumnCount(5);                                    //设置列数
		//水平居左，垂直居中
		m_pTree->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		//表头自适应
		//m_pTable->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
		m_pTree->header()->setSectionResizeMode(QHeaderView::Stretch);
		m_pTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
		m_pTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
		m_pTree->setSelectionMode(QAbstractItemView::MultiSelection);
		m_pTree->header()->setDefaultSectionSize(ROW_HEIGHT);             //设置行高

		//m_pTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
		//m_pTable->horizontalHeader()->setHighlightSections(false);      //禁止标题变粗
		m_pTree->setFocusPolicy(Qt::NoFocus);                          //去除选中的虚线效果
		m_pTree->hideColumn(4);

		//
		 //滚动条样式
		QFile QssScrollFile("qss/scrollbar.qss");
		QssScrollFile.open(QFile::ReadOnly);
		m_pTree->verticalScrollBar()->setStyleSheet(QssScrollFile.readAll());
		init_slots();
	}


	void ShareTree::init_slots()
	{
		//连接item离开事件
		connect(m_pTree, &TreeWidget::itemLeft, m_pTree,
			[&](QTreeWidgetItem* pCurItem)
			{
				if (nullptr != pCurItem)
				{
					FileNameGroup* pGroup = (FileNameGroup*)m_pTree->itemWidget(pCurItem, 0);
					if (nullptr != pGroup)
					{
						pGroup->hide_button();
					}
				}
							
			});
		//连接item进入事件
		connect(m_pTree, &TreeWidget::itemEntered, m_pTree,
			[&](QTreeWidgetItem* pCurItem)
			{
				if (nullptr != pCurItem)
				{
					FileNameGroup* pGroup = (FileNameGroup*)m_pTree->itemWidget(pCurItem, 0);
					if (nullptr != pGroup)
					{
						pGroup->show_button();
					}
				}
			});
		//连接item单击事件
		connect(m_pTree, &QTreeWidget::itemClicked, m_pTree,
			[&](QTreeWidgetItem* pCurItem, int Col)
			{
				QCheckBox* pCurCheckBox = ((FileNameGroup*)m_pTree->itemWidget(pCurItem, 0))->m_pCheckBox;
				if (Qt::CheckState::Checked == pCurCheckBox->checkState())
				{
					pCurCheckBox->setCheckState(Qt::CheckState::Unchecked);
				}
				else
				{
					pCurCheckBox->setCheckState(Qt::CheckState::Checked);
				}
			});
		//向Gui添加新的分享文件
		connect(this, &ShareTree::new_share, this, 
			[&](int32_t FileSeq, const QString& FileName,const QString& Remark, 
				const QString& CreateTime, uint64_t UploadData)
			{
				std::unique_lock<std::mutex> Lock(m_Mutex);
				if (0 != m_pItemMap.count(FileSeq))
				{
					return;
				}
				//在最上面添加一行
				QTreeWidgetItem* pCurLine = new QTreeWidgetItem();
				m_pItemMap[FileSeq] = pCurLine;
				if (FileSeq % 111111111 == 0)
				{
					QTreeWidgetItem* pParentLine = m_pItemMap[1];
					pParentLine->addChild(pCurLine);
				}
				else
				{
					m_pTree->addTopLevelItem(pCurLine);
				}
				//初始化第0格内容
				FileNameGroup* pFileNameGroup = new FileNameGroup();
				pFileNameGroup->init(FileSeq, FileName);
				pFileNameGroup->set_style(m_strStyle, m_strLanguage);
				pFileNameGroup->setMouseTracking(true);
				m_pTree->setItemWidget(pCurLine, 0, pFileNameGroup);
				//设置分享时间
				pCurLine->setText(1, CreateTime);
				//设置上传流量
				pCurLine->setText(2, QString::number(UploadData) + "1232MB");
				//设置备注
				pCurLine->setText(3, Remark);
				//连接信号
				init_sub_slots(pFileNameGroup);
			});
	}

	void ShareTree::init_sub_slots(FileNameGroup* pGroup)
	{
		//复选框信号
		connect(pGroup->m_pCheckBox, &QCheckBox::stateChanged, m_pTree,
			[&](int32_t State)
			{
				QTreeWidgetItem* pCurItem = m_pTree->itemCur();
				if (nullptr == pCurItem)
				{
					return;
				}
				if (Qt::CheckState::Checked == State)
				{
					pCurItem->setSelected(true);//设置选中状态
					//给所有子Item设置
					for (int32_t i = 0; i < pCurItem->childCount(); ++i)
					{
						QTreeWidgetItem* pCurChild = pCurItem->child(i);
						QCheckBox* pCurCheckBox = ((FileNameGroup*)m_pTree->itemWidget(pCurChild, 0))->m_pCheckBox;
						pCurCheckBox->setCheckState(Qt::CheckState::Checked);
						pCurChild->setSelected(true);
					}
				}
				else if (Qt::CheckState::Unchecked == State)
				{
					pCurItem->setSelected(false);
					for (int32_t i = 0; i < pCurItem->childCount(); ++i)
					{
						QTreeWidgetItem* pCurChild = pCurItem->child(i);
						QCheckBox* pCurCheckBox = ((FileNameGroup*)m_pTree->itemWidget(pCurChild, 0))->m_pCheckBox;
						pCurCheckBox->setCheckState(Qt::CheckState::Unchecked);
						pCurChild->setSelected(false);
					}
					//给父Item设置
					QTreeWidgetItem* pParent = pCurItem->parent();
					if (nullptr != pParent)
					{
						QCheckBox* pCurCheckBox = ((FileNameGroup*)m_pTree->itemWidget(pParent, 0))->m_pCheckBox;
						pCurCheckBox->setCheckState(Qt::CheckState::Unchecked);
					}
				}
			});
		//获取链接信号
		connect(pGroup, &FileNameGroup::link_me, this, [&](int32_t FileSeq) 
			{
				emit(create_link(FileSeq));
			});
		//打开文件夹信号
		connect(pGroup, &FileNameGroup::folder_me, this, [&](int32_t FileSeq) 
			{
				emit(open_folder(FileSeq));
			});
		//删除文件信号
		connect(pGroup, &FileNameGroup::delete_me, this, [&](int32_t FileSeq) 
			{
				std::unique_lock<std::mutex> Lock(m_Mutex);
				if (0 != m_pItemMap.count(FileSeq))
				{
					QTreeWidgetItem* pCurItem = m_pItemMap[FileSeq];
					m_pItemMap.erase(FileSeq);

					if (nullptr != pCurItem)
					{
						delete pCurItem;
					}
					m_pTree->reset();
				}
				emit(delete_file(FileSeq));
			});
	}

	void ShareTree::set_style(const QString& Style, const QString& Language)
	{
		m_strStyle = Style;
		m_strLanguage = Language;

		m_pTree->setHeaderLabels(QStringList() 
			<< QString::fromLocal8Bit("文件名") 
			<< QString::fromLocal8Bit("创建时间")
			<< QString::fromLocal8Bit("已上传流量")
			<< QString::fromLocal8Bit("备注")
			<< "FileSeq");
		//设置滚动条样式
		QScrollBar* pScrollBar = m_pTree->verticalScrollBar();
		pScrollBar->setProperty("Style", Style);
		style()->unpolish(pScrollBar);
		style()->polish(pScrollBar);

		QFile QssFile("qss/share_tree.qss");
		QssFile.open(QFile::ReadOnly);
		QString QssStyle = QssFile.readAll();
		m_pTree->setStyleSheet(QssStyle);
		m_pTree->setProperty("TreeWidget", Style);
		style()->unpolish(m_pTree);
		style()->polish(m_pTree);
		//设置Header样式
		m_pTree->header()->setStyleSheet(QssStyle);
		m_pTree->header()->setProperty("TreeWidget", Style);
		style()->unpolish(m_pTree->header());
		style()->polish(m_pTree->header());
		//遍历Item然后设置样式
		QTreeWidgetItemIterator pIt(m_pTree);
		while (*pIt) 
		{
			((FileNameGroup*)m_pTree->itemWidget(*pIt, 0))->set_style(Style, Language);
			++pIt;
		}
	}
}
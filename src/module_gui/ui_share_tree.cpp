#include "ui_share_tree.h"
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <base/config.hpp>
#include <base/logger/logger.h>
#include <module_db/database.h>
#define SET_PROPERTY(QOBJ,PROPERTY,STYLE) QOBJ->setProperty(PROPERTY, STYLE);style()->unpolish(QOBJ);style()->polish(QOBJ)
constexpr int32_t ROW_HEIGHT = 35;

namespace gui
{
	ShareTree::ShareTree(QWidget* Parent)
		:QWidget(Parent),
		m_pTree(new TreeWidget(this)),
		m_pCurGroup(nullptr)
	{
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

		QFile qssFile("qss/share_tree.qss");
		qssFile.open(QFile::ReadOnly);
		QString qssStyle = qssFile.readAll();
		m_pTree->setStyleSheet(qssStyle);
		m_pTree->header()->setStyleSheet(qssStyle);
		 //滚动条样式
		QFile qssScrollBar("qss/scrollbar.qss");
		qssScrollBar.open(QFile::ReadOnly);
		m_pTree->verticalScrollBar()->setStyleSheet(qssScrollBar.readAll());
		init_slots();
	}

	void ShareTree::init_slots()
	{
		//连接item离开事件
		connect(m_pTree, &TreeWidget::itemLeft, m_pTree,
			[&](QTreeWidgetItem* m_pCurItem)
			{
				if (nullptr != m_pCurItem)
				{
					FileNameGroup* pGroup = (FileNameGroup*)m_pTree->itemWidget(m_pCurItem, 0);
					if (nullptr != pGroup)
					{
						pGroup->hide_button();
					}
				}
							
			});
		//连接item进入事件
		connect(m_pTree, &TreeWidget::itemEntered, m_pTree,
			[&](QTreeWidgetItem* m_pCurItem)
			{
				if (nullptr != m_pCurItem)
				{
					FileNameGroup* pGroup = (FileNameGroup*)m_pTree->itemWidget(m_pCurItem, 0);
					if (nullptr != pGroup)
					{
						pGroup->show_button();
					}
				}
			});
		//连接item单击事件
		connect(m_pTree, &QTreeWidget::itemClicked, m_pTree,
			[&](QTreeWidgetItem* m_pCurItem, int Col)
			{
				QCheckBox* pCurCheckBox = ((FileNameGroup*)m_pTree->itemWidget(m_pCurItem, 0))->m_pCheckBox;
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
		connect(this, &ShareTree::show_share, this,
			[&](int32_t FileSeq)
			{
				std::string FileName, Remark, CreateTime;
				uint64_t UploadData = 0;
				std::unique_lock<std::mutex> Lock(m_Mutex);
				if (0 != m_pItemMap.count(FileSeq))
				{
					return;
				}
				bool bRes = g_pDataBaseManager->select_file_info(FileSeq, FileName, UploadData, Remark, CreateTime);
				if (false == bRes)
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
				pFileNameGroup->init(FileSeq, QString::fromStdString(FileName));
				pFileNameGroup->set_style(m_strStyle, m_strLanguage);
				pFileNameGroup->setMouseTracking(true);
				m_pTree->setItemWidget(pCurLine, 0, pFileNameGroup);
				//设置分享时间
				pCurLine->setText(1, QString::fromStdString(CreateTime));
				//设置上传流量
				pCurLine->setText(2, QString::number(UploadData) + "1232MB");
				//设置备注
				pCurLine->setText(3, QString::fromStdString(Remark));
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
				QTreeWidgetItem* m_pCurItem = m_pTree->itemCur();
				if (nullptr == m_pCurItem)
				{
					return;
				}
				if (Qt::CheckState::Checked == State)
				{
					m_pCurItem->setSelected(true);//设置选中状态
					//给所有子Item设置
					for (int32_t i = 0; i < m_pCurItem->childCount(); ++i)
					{
						QTreeWidgetItem* pCurChild = m_pCurItem->child(i);
						QCheckBox* pCurCheckBox = ((FileNameGroup*)m_pTree->itemWidget(pCurChild, 0))->m_pCheckBox;
						pCurCheckBox->setCheckState(Qt::CheckState::Checked);
						pCurChild->setSelected(true);
					}
				}
				else if (Qt::CheckState::Unchecked == State)
				{
					m_pCurItem->setSelected(false);
					for (int32_t i = 0; i < m_pCurItem->childCount(); ++i)
					{
						QTreeWidgetItem* pCurChild = m_pCurItem->child(i);
						QCheckBox* pCurCheckBox = ((FileNameGroup*)m_pTree->itemWidget(pCurChild, 0))->m_pCheckBox;
						pCurCheckBox->setCheckState(Qt::CheckState::Unchecked);
						pCurChild->setSelected(false);
					}
					//给父Item设置
					QTreeWidgetItem* pParent = m_pCurItem->parent();
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
				clear_file(FileSeq);
				emit(delete_file(FileSeq));
			});
	}

	void ShareTree::clear_file(int32_t FileSeq)
	{
		std::unique_lock<std::mutex> Lock(m_Mutex);
		if (0 != m_pItemMap.count(FileSeq))
		{
			QTreeWidgetItem* m_pCurItem = m_pItemMap[FileSeq];
			m_pItemMap.erase(FileSeq);

			if (nullptr != m_pCurItem)
			{
				delete m_pCurItem;
			}
			m_pTree->reset();
		}
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

		QScrollBar* pScrollBar = m_pTree->verticalScrollBar();
		SET_PROPERTY(pScrollBar, "Style", Style);
		SET_PROPERTY(m_pTree, "TreeWidget", Style);
		SET_PROPERTY(m_pTree->header(), "TreeWidget", Style);

		//遍历Item然后设置样式
		QTreeWidgetItemIterator pIt(m_pTree);
		while (*pIt) 
		{
			((FileNameGroup*)m_pTree->itemWidget(*pIt, 0))->set_style(Style, Language);
			++pIt;
		}
	}
}
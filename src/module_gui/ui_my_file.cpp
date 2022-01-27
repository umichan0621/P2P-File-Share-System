#include "ui_my_file.h"
#include <QFile>
#include <QHBoxLayout>
#include <base/logger/logger.h>
#include <base/encoder.h>
#include <base/config.hpp>
#include <module_db/database.h>
#include "ui_list_component.h"
#pragma warning(disable:26812)

namespace gui
{
	constexpr int8_t KPATH_BUTTON_WIDTH = 105;

	PathButton::PathButton(QWidget* Parent) :
		QPushButton(Parent), 
		m_FileSeq(0)
	{
		connect(this, &PathButton::clicked, this, [&]()
			{
				emit(button_clicked(m_FileSeq));
			});
	}
	
	void PathButton::set_file_seq(int32_t FileSeq)
	{ 
		m_FileSeq = FileSeq; 
	}
	
	MyFile::MyFile(QWidget* Parent) :
		QWidget(Parent),
		m_pLineUp(new QFrame(this)),
		m_pLineLow(new QFrame(this)),
		m_pFileManager(new FileListWidget(this)),
		m_pChoosePath(new QWidget(this)),
		m_pChoosePathLayout(new QHBoxLayout(m_pChoosePath)),
		m_pCurFolder(0),
		m_pBack(new QPushButton(this)),
		m_pNext(new QPushButton(this))
	{
		load_qss();
		setMouseTracking(true);

		FileInfo Info = { 0 };
		Info.FileName = QString::fromLocal8Bit("我的文件").toStdString();
		m_FileMap[0] = Info;
		//分隔线设置
		m_pLineUp->setFrameShape(QFrame::HLine);
		m_pLineLow->setFrameShape(QFrame::HLine);
		m_pChoosePathLayout->setMargin(0);
		m_pBack->setGeometry(13, 25, 25, 25);
		m_pNext->setGeometry(46, 25, 25, 25);

		m_pChoosePath->setGeometry(70, 25, KPATH_BUTTON_WIDTH * 5, 25);
		m_pBack->setProperty("Button", "back");
		m_pNext->setProperty("Button", "next");
		m_pBack->setStyleSheet(m_qssStyle);
		m_pNext->setStyleSheet(m_qssStyle);

		init_slots();
		refresh_path();
	}



	void MyFile::init_slots()
	{
		//新建一个文件夹
		connect(m_pFileManager, &FileListWidget::create_folder, this, [&]()
			{
				std::lock_guard<std::mutex> Lock(m_MyFileMutex);
				int32_t FileSeq = g_pDataBaseManager->file_seq();

				bool bRes = g_pDataBaseManager->create_new_folder(
					FileSeq, m_pCurFolder, base::string_to_utf8("新建文件夹"));
				if (true == bRes)
				{
					FileInfo Info = { 0 };
					bRes = g_pDataBaseManager->select_file_info(FileSeq, Info.Parent,
						Info.Type, Info.FileSize, Info.FileName, Info.WriteTime);
					if (false == bRes)
					{
						return;
					}
					m_FileMap[FileSeq] = Info;
					m_FolderMap[Info.Parent].emplace_back(FileSeq);
					if (Info.Parent == m_pCurFolder)
					{
						refresh();
					}
				}
			});

		//删除一个文件/文件夹
		connect(m_pFileManager, &FileListWidget::delete_file, this, [&]
		(int32_t FileSeq,int32_t ItemSeq)
			{
				{//询问是否删除

				}
				std::lock_guard<std::mutex> Lock(m_MyFileMutex);
				if (0 != m_FileMap.count(FileSeq))
				{
					{//删除My File逻辑文件夹内的文件/文件夹
					//从上一级文件夹中删除
						int32_t Parent = m_FileMap[FileSeq].Parent;
						for (int32_t i = 0; i < m_FolderMap[Parent].size(); ++i)
						{
							if (m_FolderMap[Parent][i] == FileSeq)
							{
								std::swap(m_FolderMap[Parent][i], m_FolderMap[Parent].back());
								m_FolderMap[Parent].pop_back();
								break;
							}
						}
						//删除当前文件和所有子文件/文件夹
						delete_folder(FileSeq);

					}//删除My File逻辑文件夹内的文件/文件夹
					//删除Gui
					m_pFileManager->takeItem(ItemSeq);
				}
			});

		//移动文件到
		connect(m_pFileManager, &FileListWidget::move_file, this, [&]
			(int32_t FileSeq)
			{
				emit(file_move(FileSeq));
			});

		//打开文件夹
		connect(m_pFileManager, &FileListWidget::open_folder, this, [&]
		(int32_t FileSeq)
			{
				std::lock_guard<std::mutex> Lock(m_MyFileMutex);
				if (0 == m_FileMap.count(FileSeq))
				{
					return;
				}
				int8_t FileType=m_FileMap[FileSeq].Type;
				if (STATUS_FOLDER == FileType || STATUS_FOLDER_SHARE == FileType)
				{
					m_pPreFolder.push_back(m_pCurFolder);
					m_pCurFolder = FileSeq;
					refresh();
				}
				
			});

		//路径回退
		connect(m_pBack, &QPushButton::clicked, this, [&]() 
			{
				std::lock_guard<std::mutex> Lock(m_MyFileMutex);
				if (false == m_pPreFolder.empty())
				{
					m_pNextFolder.push_back(m_pCurFolder);
					m_pCurFolder = m_pPreFolder.back();
					m_pPreFolder.pop_back();
					refresh();
				}
			});

		//路径前进
		connect(m_pNext, &QPushButton::clicked, this, [&]() 
			{
				std::lock_guard<std::mutex> Lock(m_MyFileMutex);
				if (false == m_pNextFolder.empty())
				{
					m_pPreFolder.push_back(m_pCurFolder);
					m_pCurFolder = m_pNextFolder.back();
					m_pNextFolder.pop_back();
					refresh();
				}
			});


		//向MyFile添加一条文件记录
		connect(this, &MyFile::add_file, this, [&]
		(int32_t FileSeq)
			{
				std::lock_guard<std::mutex> Lock(m_MyFileMutex);
				if (0 == m_FileMap.count(FileSeq))
				{
					FileInfo Info = { 0 };
					bool bRes = g_pDataBaseManager->select_file_info(FileSeq, Info.Parent,
						Info.Type, Info.FileSize, Info.FileName, Info.WriteTime);
					if (false == bRes)
					{
						return;
					}
					m_FileMap[FileSeq] = Info;
					m_FolderMap[Info.Parent].emplace_back(FileSeq);
					if (Info.Parent == m_pCurFolder)
					{
						refresh();
					}
				}
			});

		connect(this, &MyFile::file_move_to, this, [&]
			(int32_t FileSeq, int32_t FileParent)
		{
				std::lock_guard<std::mutex> Lock(m_MyFileMutex);
				if (0 != m_FileMap.count(FileSeq))
				{
					int32_t OldParent = m_FileMap[FileSeq].Parent;
					m_FileMap[FileSeq].Parent= FileParent;
					m_FolderMap[FileParent].push_back(FileSeq);
					for (int32_t i = 0; i < m_FolderMap[OldParent].size(); ++i)
					{
						if (m_FolderMap[OldParent][i] == FileSeq)
						{
							std::swap(m_FolderMap[OldParent][i], m_FolderMap[OldParent].back());
							m_FolderMap[OldParent].pop_back();
							break;
						}
					}
					refresh();
				}
		});
	}

	void MyFile::set_style(const QString& Style, const QString& Language)
	{
		load_qss();
		m_strStyle = Style;
		m_pFileManager->setStyleSheet(m_qssStyle);
		m_pFileManager->setProperty("MyFile", Style);

		style()->unpolish(m_pFileManager);
		style()->polish(m_pFileManager);
		for (auto& pCurButton : m_FolderPath)
		{
			pCurButton->setProperty("Path", m_strStyle);
			pCurButton->setStyleSheet(m_qssStyle);
			style()->unpolish(pCurButton);
			style()->polish(pCurButton);
		}
		refresh();
	}

	void MyFile::load_qss()
	{
		QFile QssFile("qss/my_file.qss");
		QssFile.open(QFile::ReadOnly);
		m_qssStyle = QssFile.readAll();
	}

	Folder MyFile::folder_info()
	{
		std::lock_guard<std::mutex> Lock(m_MyFileMutex);
		return sub_folder(0);
	}

	Folder MyFile::sub_folder(int32_t FileSeq)
	{
		Folder CurFolder = { 0 };
		CurFolder.FileSeq = FileSeq;
		CurFolder.FileName = QString::fromStdString(m_FileMap[FileSeq].FileName);
		
		for (int32_t SubFile : m_FolderMap[FileSeq])
		{
			if (m_FileMap[SubFile].Type >= STATUS_FOLDER)
			{
				CurFolder.SubFolder.emplace_back(sub_folder(SubFile));
			}
		}
		std::sort(CurFolder.SubFolder.begin(), CurFolder.SubFolder.end(), 
			[](const Folder&F1, const Folder& F2)
			{
				return F1.FileName < F2.FileName;
			});
		return CurFolder;
	}

	void MyFile::refresh_path()
	{
		std::vector<int32_t> Path;
		int32_t CurFolder = m_pCurFolder;
		if (0 != CurFolder)
		{
			while (0 != CurFolder)
			{
				Path.push_back(CurFolder);
				CurFolder=m_FileMap[CurFolder].Parent;
			}
		}
		Path.push_back(CurFolder);

		for (auto& pCurButton : m_FolderPath)
		{
			if (nullptr != pCurButton)
			{
				disconnect(pCurButton);
				delete pCurButton;
			}
		}

		m_FolderPath.clear();
		std::reverse(Path.begin(), Path.end());
		QLayoutItem* pChild= m_pChoosePathLayout->takeAt(0);

		while (nullptr != pChild)
		{
			delete pChild;
			pChild = m_pChoosePathLayout->takeAt(0);
		}
		
		for (int32_t i = 0; i <Path.size(); ++i)
		{
			int32_t FileSeq = Path[i];
			PathButton* pCurButton = new PathButton(m_pChoosePath);
			m_pChoosePathLayout->addWidget(pCurButton);
			pCurButton->setFixedHeight(25);
			pCurButton->set_file_seq(FileSeq);
			pCurButton->show();
			pCurButton->setProperty("Path", m_strStyle);
			pCurButton->setStyleSheet(m_qssStyle);
			QString FileName = QString::fromStdString(m_FileMap[FileSeq].FileName);
			QFontMetrics Font(pCurButton->font());
			int32_t FontSize = Font.width(FileName);
			if (FontSize > 150)
			{
				FileName = Font.elidedText(FileName, Qt::ElideRight, 150);
			}
			FileName += " >";
			pCurButton->setText(FileName);

			m_FolderPath.push_back(pCurButton);
			connect(pCurButton, &PathButton::button_clicked, this, [&](int32_t FileSeq)
				{
					m_pPreFolder.push_back(m_pCurFolder);
					m_pNextFolder.clear();
					if (FileSeq != m_pCurFolder)
					{
						m_pCurFolder = FileSeq;
						refresh();
					}
				});
		}
		m_pChoosePathLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		//int32_t Len = 0;
		//for (int32_t i = 0; i < m_FolderPath.size(); ++i)
		//{
		//	Len+= m_FolderPath[i].wid
		//	//LOG_ERROR << m_FolderPath[i]->width();
		//}
		//LOG_ERROR << Len;

	}

	void MyFile::refresh()
	{
		int32_t Count = m_pFileManager->count();
		for (int32_t i = 0; i<Count; ++i)
		{
			delete m_pFileManager->takeItem(0);
		}
		//排序
		std::vector<int32_t> VecSort;
		for (auto FileSeq : m_FolderMap[m_pCurFolder])
		{
			VecSort.push_back(FileSeq);
		}
		std::sort(VecSort.begin(), VecSort.end(), [&]
		(const int32_t& FileSeq1, const int32_t& FileSeq2)
			{
				FileInfo& Info1 = m_FileMap[FileSeq1];
				FileInfo& Info2 = m_FileMap[FileSeq2];
				//文件夹优先显示
				int8_t FileType1 = Info1.Type >= STATUS_FOLDER ? STATUS_FOLDER : STATUS_OFFLINE;
				int8_t FileType2 = Info2.Type >= STATUS_FOLDER ? STATUS_FOLDER : STATUS_OFFLINE;
				if (FileType1 != FileType2)
				{
					return FileType1 > FileType2;
				}
				//按文件名排序
				return m_FileMap[FileSeq1].FileName < m_FileMap[FileSeq2].FileName;
			});

		for (auto FileSeq : VecSort)
		{
			show_file(FileSeq);
		}
		refresh_path();
	}

	void MyFile::delete_folder(int32_t FileSeq)
	{
		for (auto& SubFileSeq : m_FolderMap[FileSeq])
		{
			delete_folder(SubFileSeq);
		}
		m_FileMap.erase(FileSeq);
		m_FolderMap.erase(FileSeq);
		//向外部发送删除的信号
		emit(delete_file(FileSeq));
	}

	void MyFile::show_file(int32_t FileSeq)
	{
		if (0 == m_FileMap.count(FileSeq))
		{
			return;
		}
		QListWidgetItem* pItem = new QListWidgetItem();
		pItem->setSizeHint(QSize(120, 120));
		m_pFileManager->addItem(pItem);
		IconComponent* pIconComponent = new IconComponent();
		QString FileName = QString::fromStdString(m_FileMap[FileSeq].FileName);
		pIconComponent->set_style(m_qssStyle, m_strStyle);
		pIconComponent->init(FileSeq, FileName, m_FileMap[FileSeq].Type);
		m_pFileManager->setItemWidget(pItem, pIconComponent);
		connect(pIconComponent, &IconComponent::rename_file, this, [&]
		(int32_t FileSeq,const QString& strFileName) 
			{
				if (0 == m_FileMap.count(FileSeq))
				{
					return;
				}
				std::string FileName= strFileName.toStdString();
				bool bRes=g_pDataBaseManager->update_file_name(FileSeq, FileName);
				if (true == bRes)
				{
					m_FileMap[FileSeq].FileName = strFileName.toStdString();
					refresh();
				}
			});
	}

	void MyFile::resizeEvent(QResizeEvent* pEvent)
	{
		m_pLineUp->setGeometry(13, 24, width() - 35, 2);
		m_pLineLow->setGeometry(13, 50, width() - 35, 2);
		m_pFileManager->setGeometry(3, 52, width() - 6, height() - 52);
	}
}
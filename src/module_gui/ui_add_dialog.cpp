#include "ui_add_dialog.h"
#include <QVBoxLayout>
#include <QStyle>
#include <QScrollBar>
#include <QFileDialog>
#include <base/logger/logger.h>
#define SET_PROPERTY(QOBJ,PROPERTY,STYLE) QOBJ->setProperty(PROPERTY, STYLE);style()->unpolish(QOBJ);style()->polish(QOBJ)

namespace gui
{
	AddDialog::AddDialog(QWidget* parent)
		: FramelessDialog(parent),
		//固定组件
		m_pAddFile(new QPushButton(m_pDialog)),
		m_pAddShare(new QPushButton(m_pDialog)),
		m_pAddPeer(new QPushButton(m_pDialog)),
		m_pLine(new QFrame(m_pDialog)),
		m_pBottom(new QWidget(m_pDialog)),
		m_pCancel(new QPushButton(m_pBottom)),
		m_pSubmit(new QPushButton(m_pBottom)),
		//下载组件
		m_pDownload(new QWidget(m_pDialog)),
		m_pDownloadTextEdit(new QTextEdit(m_pDownload)),
		m_pDownloadLineEdit(new QLineEdit(m_pDownload)),
		m_pDownloadSetPath(new QPushButton(m_pDownload)),
		m_pDownloadSavePath(new QLabel(m_pDownload)),
		//分享组件
		m_pShare(new QWidget(m_pDialog)),
		m_pShareTextEdit(new QTextEdit(m_pShare)),
		m_pShareLineEdit(new QLineEdit(m_pShare)),
		m_pShareSetPath(new QPushButton(m_pShare)),
		m_pShareFilePath(new QLabel(m_pShare)),
		//Peer组件
		m_pPeer(new QWidget(m_pDialog)),
		m_CurChoose(ADD_NULL)
	{
		//分隔线设置
		m_pLine->setFrameShape(QFrame::HLine);
		m_pLine->lower();
		//设置按钮
		m_pSubmit->setCursor(QCursor(Qt::PointingHandCursor));
		m_pCancel->setCursor(QCursor(Qt::PointingHandCursor));
		m_pAddFile->setCursor(QCursor(Qt::PointingHandCursor));
		m_pAddShare->setCursor(QCursor(Qt::PointingHandCursor));
		m_pAddPeer->setCursor(QCursor(Qt::PointingHandCursor));
		m_pDownloadSetPath->setCursor(QCursor(Qt::PointingHandCursor));
		//设置固定布局
		int32_t Width = 600;
		int32_t Height = 270;
		int32_t Bounadry = 20;
		//上侧，高度占0-50
		m_pAddFile->setGeometry(Bounadry, 20, 80, 32);
		m_pAddShare->setGeometry(Bounadry + 80, 20, 80, 32);
		m_pAddPeer->setGeometry(Bounadry + 160, 20, 80, 32);
		m_pLine->setGeometry(20, 50, 560, 2);
		//下侧，高度占200-270
		m_pBottom->setGeometry(0, Height - 70, Width, 70);
		m_pCancel->setGeometry(Width - 160, 20, 60, 28);
		m_pSubmit->setGeometry(Width - 80, 20, 60, 28);
		//中间，高度占50-200
		//下载组件
		m_pDownload->setGeometry(0, 50, Width, 150);
		m_pDownloadTextEdit->setGeometry(Bounadry, 15, Width - 2 * Bounadry, 65);
		m_pDownloadLineEdit->setGeometry(Bounadry + 100, 100, Width - 185, 26);
		m_pDownloadSetPath->setGeometry(Width - 65, 100, 45, 26);
		m_pDownloadSavePath->setGeometry(Bounadry, 100, 100, 26);
		//分享组件
		m_pShare->setGeometry(0, 50, Width, 150);
		m_pShareTextEdit->setGeometry(Bounadry, 15, Width - 2 * Bounadry, 65);
		m_pShareLineEdit->setGeometry(Bounadry + 100, 100, Width - 185, 26);
		m_pShareSetPath->setGeometry(Width - 65, 100, 45, 26);
		m_pShareFilePath->setGeometry(Bounadry, 100, 100, 26);
		//Peer组件
		m_pPeer->setGeometry(0, 50, Width, 150);

		QFile QssTotal("qss/dialog.qss");
		QssTotal.open(QFile::ReadOnly);
		QString qssStyle = QssTotal.readAll();
		QFile qssButton("qss/button_style.qss");
		qssButton.open(QFile::ReadOnly);
		QString qssButtonStyle = qssButton.readAll();
		QFile TextEditScrollBar("qss/scrollbar.qss");
		TextEditScrollBar.open(QFile::ReadOnly);
		QString qssScrollBarStyle = TextEditScrollBar.readAll();
		QFile qssLine("qss/split_line_style.qss");
		qssLine.open(QFile::ReadOnly);
		QString qssLineStyle = qssLine.readAll();
		//设置样式
		m_pDownload->setStyleSheet(qssStyle);
		m_pShare->setStyleSheet(qssStyle);
		m_pPeer->setStyleSheet(qssStyle);
		//对话框背景
		m_pDialog->setStyleSheet(qssStyle);
		//对话框底部
		m_pBottom->setStyleSheet(qssStyle);
		//设置多行输入框
		m_pDownloadTextEdit->setStyleSheet(qssStyle);
		m_pShareTextEdit->setStyleSheet(qssStyle);
		//设置单行输入框
		m_pDownloadLineEdit->setStyleSheet(qssStyle);
		m_pShareLineEdit->setStyleSheet(qssStyle);
		//设置打开文件路径按钮
		m_pDownloadSetPath->setStyleSheet(qssStyle);
		m_pShareSetPath->setStyleSheet(qssStyle);
		//设置字体
		m_pDownloadSavePath->setStyleSheet(qssStyle);
		m_pShareFilePath->setStyleSheet(qssStyle);
		//分隔线
		m_pLine->setStyleSheet(qssLineStyle);
		//设置多行输入框滚动条
		m_pDownloadTextEdit->verticalScrollBar()->setStyleSheet(qssScrollBarStyle);
		m_pShareTextEdit->verticalScrollBar()->setStyleSheet(qssScrollBarStyle);
		//设置取消键
		m_pCancel->setStyleSheet(qssButtonStyle);
		//设置提交按钮样式
		m_pSubmit->setStyleSheet(qssButtonStyle);
		//设置按钮
		m_pAddFile->setStyleSheet(qssButtonStyle);
		m_pAddShare->setStyleSheet(qssButtonStyle);
		m_pAddPeer->setStyleSheet(qssButtonStyle);
		m_pAddFile->setProperty("FontSize", "15");
		m_pAddShare->setProperty("FontSize", "15");
		m_pAddPeer->setProperty("FontSize", "15");
		//初始化槽函数
		init_slots();
	}

	void AddDialog::set_mode(uint8_t Mode)
	{
		if (ADD_FILE == Mode)
		{
			emit(m_pAddFile->clicked(true));
		}
		else if (ADD_SHARE == Mode)
		{
			emit(m_pAddShare->clicked(true));
		}
		else
		{
			emit(m_pAddPeer->clicked(true));
		}
	}

	void AddDialog::set_path_download(const QString& DefaultPath)
	{
		m_pDownloadLineEdit->setText(DefaultPath);
	}

	void AddDialog::set_path_share(const QString& DefaultPath)
	{
		m_pShareLineEdit->setText(DefaultPath);
	}

	void AddDialog::set_style(const QString& Style, const QString& Language)
	{
		m_strStyle = Style;
		m_strLanguage = Language;
		m_pAddFile->setText(QString::fromLocal8Bit("下载文件"));
		m_pAddShare->setText(QString::fromLocal8Bit("分享文件"));
		m_pAddPeer->setText(QString::fromLocal8Bit("添加节点"));
		m_pSubmit->setText(QString::fromLocal8Bit("提 交"));
		m_pCancel->setText(QString::fromLocal8Bit("取 消"));
		m_pDownloadSavePath->setText(QString::fromLocal8Bit("存储路径："));
		m_pDownloadTextEdit->setPlaceholderText(QString::fromLocal8Bit("添加多个文件/文件夹链接时，确保每行只有一个链接"));
		m_pShareFilePath->setText(QString::fromLocal8Bit("文件路径："));
		m_pShareTextEdit->setPlaceholderText(QString::fromLocal8Bit("给分享的文件/文件夹设置备注"));

		//设置控件属性并刷新
		SET_PROPERTY(m_pDialog, "Background", Style);
		SET_PROPERTY(m_pBottom, "Bottom", Style);
		SET_PROPERTY(m_pLine, "Line", Style);
		SET_PROPERTY(m_pCancel, "Cancel", Style);
		SET_PROPERTY(m_pSubmit, "Confirm", Style);
		SET_PROPERTY(m_pDownloadTextEdit, "TextEdit", Style);
		SET_PROPERTY(m_pShareTextEdit, "TextEdit", Style);
		SET_PROPERTY(m_pDownloadTextEdit->verticalScrollBar(), "Style", Style);
		SET_PROPERTY(m_pShareTextEdit->verticalScrollBar(), "Style", Style);
		SET_PROPERTY(m_pDownloadLineEdit, "LineEdit", Style);
		SET_PROPERTY(m_pShareLineEdit, "LineEdit", Style);
		SET_PROPERTY(m_pDownloadSetPath, "SetPath", Style);
		SET_PROPERTY(m_pShareSetPath, "SetPath", Style);
		SET_PROPERTY(m_pDownloadSavePath, "LabelText", Style);
		SET_PROPERTY(m_pShareFilePath, "LabelText", Style);
		SET_PROPERTY(m_pAddFile, "ButtonStyle", Style);
		SET_PROPERTY(m_pAddShare, "ButtonStyle", Style);
		SET_PROPERTY(m_pAddPeer, "ButtonStyle", Style);

		//设置顶部按钮样式
		set_top_button();
	}

	void AddDialog::set_top_button()
	{
		if (ADD_FILE == m_CurChoose)
		{
			SET_PROPERTY(m_pAddFile, "ButtonStatus", "choose");
			SET_PROPERTY(m_pAddShare, "ButtonStatus", "unchoose");
			SET_PROPERTY(m_pAddPeer, "ButtonStatus", "unchoose");
		}
		else if (ADD_SHARE == m_CurChoose)
		{
			SET_PROPERTY(m_pAddShare, "ButtonStatus", "choose");
			SET_PROPERTY(m_pAddFile, "ButtonStatus", "unchoose");
			SET_PROPERTY(m_pAddPeer, "ButtonStatus", "unchoose");
		}
		else if (ADD_PEER == m_CurChoose)
		{
			SET_PROPERTY(m_pAddPeer, "ButtonStatus", "choose");
			SET_PROPERTY(m_pAddShare, "ButtonStatus", "unchoose");
			SET_PROPERTY(m_pAddFile, "ButtonStatus", "unchoose");
		}
	}

	void AddDialog::init_slots()
	{
		//上侧按钮[下载文件]
		connect(m_pAddFile, &QPushButton::clicked, this, [&]()
			{
				if (ADD_FILE == m_CurChoose)
				{
					return;
				}
				m_CurChoose = ADD_FILE;
				m_pShare->hide();
				m_pPeer->hide();
				m_pDownload->show();
				set_top_button();
			});
		//上侧按钮[分享文件]
		connect(m_pAddShare, &QPushButton::clicked, this, [&]()
			{
				if (ADD_SHARE == m_CurChoose)
				{
					return;
				}
				m_CurChoose = ADD_SHARE;
				m_pDownload->hide();
				m_pPeer->hide();
				m_pShare->show();
				set_top_button();
			});
		//上侧按钮[添加节点]
		connect(m_pAddPeer, &QPushButton::clicked, this, [&]()
			{
				if (ADD_PEER == m_CurChoose)
				{
					return;
				}
				m_CurChoose = ADD_PEER;
				m_pDownload->hide();
				m_pShare->hide();
				m_pPeer->show();
				set_top_button();
			});
		connect(m_pDownloadSetPath, &QPushButton::clicked, this, [&]()
			{
				QString strDownloadPath = QFileDialog::getExistingDirectory(
					this,
					QString::fromLocal8Bit("设置下载路径"),
					m_pDownloadLineEdit->text(),
					QFileDialog::ShowDirsOnly);
				if ("" != strDownloadPath)
				{
					m_pDownloadLineEdit->setText(strDownloadPath);
				}
			});
		connect(m_pShareSetPath, &QPushButton::clicked, this, [&]()
			{
				QString strShareFile = QFileDialog::getOpenFileName(
					this,
					QString::fromLocal8Bit("选择上传的文件"),
					m_pShareLineEdit->text());
				if ("" != strShareFile)
				{
					m_pShareLineEdit->setText(strShareFile);
				}
			});
		connect(m_pCancel, &QPushButton::clicked, this, [&]()
			{
				hide();
			});
		connect(m_pSubmit, &QPushButton::clicked, this, [&]()
			{
				if (ADD_FILE == m_CurChoose)
				{
					QString strLink = m_pDownloadTextEdit->toPlainText();
					QString strPath = m_pDownloadLineEdit->text();
					if ("" == strLink || "" == strPath)
					{
						LOG_ERROR << "FAIL";
						return;
					}
					emit(add_download_file(strLink, strPath));
					m_pDownloadTextEdit->clear();
				}
				else if (ADD_SHARE == m_CurChoose)
				{
					QString strRemark = m_pShareTextEdit->toPlainText();
					QString strPath = m_pShareLineEdit->text();
					if ("" == strPath)
					{
						LOG_ERROR << "FAIL";
						return;
					}
					emit(add_share_file(strRemark, strPath));
					m_pShareTextEdit->clear();
				}
				hide();
			});
	}
}
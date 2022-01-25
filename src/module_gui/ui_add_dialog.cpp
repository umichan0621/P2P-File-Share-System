#include "ui_add_dialog.h"
#include <QVBoxLayout>
#include <QStyle>
#include <QScrollBar>
#include <QFileDialog>
#include <base/logger/logger.h>
#define CONNECT_BUTTON(_BUTTON,_FUNC) connect(_BUTTON, &QPushButton::clicked, this, _FUNC)

namespace gui
{
    AddDialog::AddDialog(QWidget* parent)
        : FramelessDialog(parent),
        //固定组件
        m_pAddFile(new QPushButton(m_pDialog)),
        m_pAddShare(new QPushButton(m_pDialog)),
        m_pAddPeer(new QPushButton(m_pDialog)),
        m_pLineLow(new QFrame(m_pDialog)),
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
        init();
    }

    void AddDialog::init()
    {
        //分隔线设置
        m_pLineLow->setFrameShape(QFrame::HLine);
        m_pLineLow->lower();
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
        m_pLineLow->setGeometry(20, 50, 560, 2);
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
        //设置样式
        m_pDownload->setStyleSheet("background-color:transparent;");
        m_pShare->setStyleSheet("background-color:transparent;");
        m_pPeer->setStyleSheet("background-color:transparent;");
        QFile QssTotal("qss/dialog.qss");
        QssTotal.open(QFile::ReadOnly);
        QString StyleTotal = QssTotal.readAll();
        //对话框背景
        m_pDialog->setStyleSheet(StyleTotal);
        //对话框底部
        m_pBottom->setStyleSheet(StyleTotal);
        //分隔线
        m_pLineLow->setStyleSheet(StyleTotal);
        //设置取消键
        m_pCancel->setStyleSheet(StyleTotal);
        //设置提交按钮样式
        m_pSubmit->setStyleSheet(StyleTotal);
        //设置多行输入框
        m_pDownloadTextEdit->setStyleSheet(StyleTotal);
        m_pShareTextEdit->setStyleSheet(StyleTotal);
        //设置多行输入框滚动条
        QFile TextEditScrollBar("qss/scrollbar.qss");
        TextEditScrollBar.open(QFile::ReadOnly);
        QString QssTextEditScrollBar = TextEditScrollBar.readAll();
        m_pDownloadTextEdit->verticalScrollBar()->setStyleSheet(QssTextEditScrollBar);
        m_pShareTextEdit->verticalScrollBar()->setStyleSheet(QssTextEditScrollBar);
        //设置单行输入框
        m_pDownloadLineEdit->setStyleSheet(StyleTotal);
        m_pShareLineEdit->setStyleSheet(StyleTotal);
        //设置打开文件路径按钮
        m_pDownloadSetPath->setStyleSheet(StyleTotal);
        m_pShareSetPath->setStyleSheet(StyleTotal);
        //设置字体
        m_pDownloadSavePath->setStyleSheet(StyleTotal);
        m_pShareFilePath->setStyleSheet(StyleTotal);
        //设置按钮
        m_pAddFile->setStyleSheet(StyleTotal);
        m_pAddShare->setStyleSheet(StyleTotal);
        m_pAddPeer->setStyleSheet(StyleTotal);
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
        m_pDownloadTextEdit->setPlaceholderText(QString::fromLocal8Bit("添加多个文件MD5时，确保每行只有一个文件MD5"));
        m_pShareFilePath->setText(QString::fromLocal8Bit("文件路径："));
        m_pShareTextEdit->setPlaceholderText(QString::fromLocal8Bit("给分享的文件/文件夹设置备注"));
        
        //设置控件属性并刷新
        m_pDialog->setProperty("Background" ,Style);
        style()->unpolish(m_pDialog);
        style()->polish(m_pDialog);
        m_pBottom->setProperty("Bottom" , Style);
        style()->unpolish(m_pBottom);
        style()->polish(m_pBottom);
        m_pLineLow->setProperty("Line" , Style);
        style()->unpolish(m_pLineLow);
        style()->polish(m_pLineLow);
        m_pCancel->setProperty("Cancel" , Style);
        style()->unpolish(m_pCancel);
        style()->polish(m_pCancel);
        m_pSubmit->setProperty("Submit" , Style);
        style()->unpolish(m_pSubmit);
        style()->polish(m_pSubmit);
        m_pDownloadTextEdit->setProperty("TextEdit"  , Style);
        style()->unpolish(m_pDownloadTextEdit);
        style()->polish(m_pDownloadTextEdit);
        m_pShareTextEdit->setProperty("TextEdit", Style);
        style()->unpolish(m_pShareTextEdit);
        style()->polish(m_pShareTextEdit);
        m_pDownloadTextEdit->verticalScrollBar()->setProperty("Style", Style);
        style()->unpolish(m_pDownloadTextEdit);
        style()->polish(m_pDownloadTextEdit);
        m_pShareTextEdit->verticalScrollBar()->setProperty("Style", Style);
        style()->unpolish(m_pShareTextEdit);
        style()->polish(m_pShareTextEdit);
        m_pDownloadLineEdit->setProperty("LineEdit" , Style);
        style()->unpolish(m_pDownloadLineEdit);
        style()->polish(m_pDownloadLineEdit);
        m_pShareLineEdit->setProperty("LineEdit" , Style);
        style()->unpolish(m_pShareLineEdit);
        style()->polish(m_pShareLineEdit);
        m_pDownloadSetPath->setProperty("SetPath" , Style);
        style()->unpolish(m_pDownloadSetPath);
        style()->polish(m_pDownloadSetPath);
        m_pShareSetPath->setProperty("SetPath" , Style);
        style()->unpolish(m_pShareSetPath);
        style()->polish(m_pShareSetPath);
        m_pDownloadSavePath->setProperty("LabelText" , Style);
        style()->unpolish(m_pDownloadSavePath);
        style()->polish(m_pDownloadSavePath);
        m_pShareFilePath->setProperty("LabelText" , Style);
        style()->unpolish(m_pShareFilePath);
        style()->polish(m_pShareFilePath);
        //设置切换的布局
       
        //导入顶部按钮样式


        //设置顶部按钮样式
        set_top_button();
    }

    void AddDialog::set_top_button()
    {
        if (ADD_FILE == m_CurChoose)
        {
            m_pAddFile->setProperty("SelectButton", "choose_" + m_strStyle);
            m_pAddShare->setProperty("SelectButton", "unchoose_" + m_strStyle);
            m_pAddPeer->setProperty("SelectButton", "unchoose_" + m_strStyle);

        }
        else if (ADD_SHARE == m_CurChoose)
        {
            m_pAddShare->setProperty("SelectButton", "choose_" + m_strStyle);
            m_pAddFile->setProperty("SelectButton", "unchoose_" + m_strStyle);
            m_pAddPeer->setProperty("SelectButton", "unchoose_" + m_strStyle);
        }
        else if (ADD_PEER == m_CurChoose)
        {
            m_pAddPeer->setProperty("SelectButton", "choose_" + m_strStyle);
            m_pAddFile->setProperty("SelectButton", "unchoose_" + m_strStyle);
            m_pAddShare->setProperty("SelectButton", "unchoose_" + m_strStyle);
        }
        style()->unpolish(m_pAddShare);
        style()->polish(m_pAddShare);
        style()->unpolish(m_pAddFile);
        style()->polish(m_pAddFile);
        style()->unpolish(m_pAddPeer);
        style()->polish(m_pAddPeer);
    }

    void AddDialog::init_slots()
    {
        //上侧按钮[下载文件]
        CONNECT_BUTTON(m_pAddFile, [&]()
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
        CONNECT_BUTTON(m_pAddShare, [&]()
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
        CONNECT_BUTTON(m_pAddPeer, [&]() 
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
        CONNECT_BUTTON(m_pDownloadSetPath, [&]() 
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
        CONNECT_BUTTON(m_pShareSetPath, [&]()
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
        CONNECT_BUTTON(m_pCancel, [&]() 
            {
                hide(); 
            });
        CONNECT_BUTTON(m_pSubmit, [&]() 
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
#include "ui_frameless_dialog.h"

FramelessDialog::FramelessDialog(QWidget* parent): 
    QWidget(parent), 
    m_pDialog(new QWidget(this))
{
    setAttribute(Qt::WA_StyledBackground, true);
    m_pDialog->setAttribute(Qt::WA_StyledBackground, true);
}

void FramelessDialog::set_size(int32_t Width, int32_t Height, int32_t ParentWidth, int32_t ParentHeight)
{
    resize(ParentWidth, ParentHeight);
    //对话框后面加深
    setStyleSheet("background:rgb(0,0,0,50%);");
    int32_t DeltaX = (width() - Width) >> 1;
    int32_t DeltaY = (height() - Height) /3;
    m_pDialog->setGeometry(DeltaX, DeltaY, Width, Height);
}

void FramelessDialog::mousePressEvent(QMouseEvent* pEvent)
{
    //点击对话框外侧关闭
    int32_t PosX = pEvent->x(), PosY = pEvent->y();
    if (PosX< m_pDialog->x() || PosY< m_pDialog->y() ||
        PosX>m_pDialog->x() + m_pDialog->width() ||
        PosY>m_pDialog->y() + m_pDialog->height())
    {
        hide();
    }
}
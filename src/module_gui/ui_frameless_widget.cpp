#include "ui_frameless_widget.h"
#include <windows.h>
#include <windowsx.h>

FramelessWidget::FramelessWidget(QWidget* parent)
    : QWidget(parent),
    m_Boundary(0),
    m_MoveHeight(0)//默认无法拖动和拉伸
{
    //setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Window |Qt::FramelessWindowHint);//设置为无边框窗口
}

void FramelessWidget::set_boundary(int32_t Boundary, int32_t MoveHeight)
{
    m_Boundary = Boundary;
    m_MoveHeight = MoveHeight;
}


bool FramelessWidget::nativeEvent(const QByteArray& EventType, void* pMessage, long* pResult)
{
    MSG* pMsg = (MSG*)pMessage;
    switch (pMsg->message)
    {
    case WM_NCHITTEST:
        int PosX = GET_X_LPARAM(pMsg->lParam) - this->frameGeometry().x();
        int PosY = GET_Y_LPARAM(pMsg->lParam) - this->frameGeometry().y();
        if (childAt(PosX, PosY) == 0)
        {
            //拖拽生效区域
            if (PosY <= m_MoveHeight)
            {
                *pResult = HTCAPTION;
            }
        }
        else 
        {
            return false;
        }
        if (PosX < m_Boundary && PosY >(height() - m_Boundary) && PosY < height())
            *pResult = HTBOTTOMLEFT;//左下
        else if (PosX > (width() - m_Boundary) && PosX < width() && PosY >(height() - m_Boundary) && PosY < height())
            *pResult = HTBOTTOMRIGHT;//右下
        else if (PosX < m_Boundary && PosY < m_Boundary)
            *pResult = HTTOPLEFT;//左上
        else if (PosX > (width() - m_Boundary) && PosX < width() && PosY < m_Boundary)
            *pResult = HTTOPRIGHT;//右上
        else if (PosX < m_Boundary)
            *pResult = HTLEFT;//左
        else if (PosX > (width() - m_Boundary) && PosX < width())
            *pResult = HTRIGHT;//右
        else if (PosY < m_Boundary)
            *pResult = HTTOP;//上
        else if (PosY > (height() - m_Boundary) && PosY < height())
            *pResult = HTBOTTOM;//下
        return true;
    }
    return false;
}

/***********************************************************************/
/* 名称:QT无边框窗口												   */
/* 说明:无边框扁平化窗口										       */
/* 创建时间:2021/12/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <QWidget>

class FramelessWidget: public QWidget
{
	Q_OBJECT
public:
	FramelessWidget(QWidget* Parent = Q_NULLPTR);
protected:
	void set_boundary(int32_t Boundary, int32_t MoveHeight);
    bool nativeEvent(const QByteArray& EventType, void* pMessage, long* pResult) override;
private:
	int32_t		m_Boundary;		//调整拉伸生效的范围
	int32_t		m_MoveHeight;	//调整拖动生效的范围
};
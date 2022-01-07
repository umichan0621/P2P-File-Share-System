/***********************************************************************/
/* 名称:QT无边框对话框												   */
/* 说明:无边框扁平化对话框，背景加深								   */
/* 创建时间:2021/12/10												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <QWidget>
#include <QMouseEvent>
class FramelessDialog : public QWidget
{
	Q_OBJECT
public:
	FramelessDialog(QWidget* Parent = Q_NULLPTR);
	void set_size(int32_t Width, int32_t Height,int32_t ParentWidth,int32_t ParentHeight);
private:
	void mousePressEvent(QMouseEvent* pEvent)override;
protected:
	QWidget*	m_pDialog;
};
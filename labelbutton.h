#ifndef LABELBUTTON_H
#define LABELBUTTON_H

#include <QLabel>

class LabelButton : public QLabel {
	Q_OBJECT

public:
	LabelButton(int i, int j, QWidget* parent = 0);

signals:
	void clicked(int, int, Qt::MouseButton);

protected:
	void mousePressEvent(QMouseEvent* event);

private:
	int loc_i, loc_j;
};

#endif

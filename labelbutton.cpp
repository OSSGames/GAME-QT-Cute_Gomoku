#include <QtGui>

#include "labelbutton.h"

LabelButton::LabelButton(int i, int j, QWidget *parent)
	: QLabel(parent), loc_i(i), loc_j(j) { }

void LabelButton::mousePressEvent (QMouseEvent *event) {
	clicked(loc_i, loc_j, event->button());
}


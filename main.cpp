#include <QtGui>
#include <QApplication>

#include "gomoku.h"

QTextStream cout(stdout);

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Gomoku w;
	w.show();
	a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
	return a.exec();
}

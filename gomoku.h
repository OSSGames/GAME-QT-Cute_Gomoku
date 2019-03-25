#ifndef GOMOKU_H
#define GOMOKU_H

#include<QtGui/QWidget>
#include<QTimer>
#include "labelbutton.h"
#include "ui_gomoku.h"
#include "gomokugamenode.h"

class Gomoku : public QWidget
{
	Q_OBJECT

public:
	Gomoku(QWidget *parent = 0);
	~Gomoku();

	static const int searchDepth = 3;
	
private:
	void setCheatMode(bool cm);
	void setPlayersSide(bool first);
	void clearGameBoard();
	void aiMove();
	bool applyMoveElem(const MoveElem& me);
	bool rewindOne();
	bool redoOne();

signals:
	void aiEvalTerminated();
	
private slots:
	void firstSelected();
	void secondSelected();
	void locClicked(int, int, Qt::MouseButton);
	void backwardClicked();
	void forwardClicked();
	void clearClicked();
	void saveClicked();
	void loadClicked();
	void CMStatusChanged(int);
	void animateLastAIPiece();
	void animateFive();

private:
	Ui::gomokuClass ui;
	QPixmap blackP, whiteP;
	LabelButton *loc[15][15];
	bool playerFirst;
	bool playersTurn;
	bool cheatMode;
	GomokuGameNode currentGameNode;
	vector<MoveElem> moveRecord;
	vector<MoveElem> rewindRecord;
	
	Loc lastAIMove;
	int lastPieceStat;
	vector<Loc> five;
	int fiveStat;
	QTimer animat, animat_sub;
};

#endif // GOMOKU_H

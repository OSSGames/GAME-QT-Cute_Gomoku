
#include<iostream>
#include<QTextStream>
#include<QFile>
#include "error.h"
#include "gomoku.h"

using namespace std;

Gomoku::Gomoku(QWidget *parent) : QWidget(parent) {
	blackP.load(":/new/prefix1/blackp.png");
	whiteP.load(":/new/prefix1/whitep.png");

	ui.setupUi(this);
	this->adjustSize();
	
	for (int i = 0; i < 15; i++) for (int j = 0; j < 15; j++) {
		loc[i][j] = new LabelButton(i, j, ui.gameBoard);
		loc[i][j]->setGeometry(23 + j*35, 18 + i*35, 35, 35);
		loc[i][j]->setPixmap(0);
	}

#ifdef DEBUG
	ui.checkBoxCheatMode->setEnabled(true);
#endif

	this->cheatMode = false;
	setPlayersSide(true);
	this->playersTurn = true;
	this->currentGameNode.setTurn(true);
	
	connect(ui.firstButton, SIGNAL(clicked()), this, SLOT(firstSelected()));
	connect(ui.secondButton, SIGNAL(clicked()), this, SLOT(secondSelected()));
	connect(ui.backwardButton, SIGNAL(clicked()), this, SLOT(backwardClicked()));
	connect(ui.forwardButton, SIGNAL(clicked()), this, SLOT(forwardClicked()));
	connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearClicked()));
	connect(ui.saveButton, SIGNAL(clicked()), this, SLOT(saveClicked()));
	connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadClicked()));
	connect(ui.checkBoxCheatMode, SIGNAL(stateChanged(int)), this, SLOT(CMStatusChanged(int)));
	for (int i = 0; i < 15; i++) for (int j = 0; j < 15; j++) {
		connect(loc[i][j], SIGNAL(clicked(int, int, Qt::MouseButton)),
				this, SLOT(locClicked(int, int, Qt::MouseButton)));
	}
	connect(&animat, SIGNAL(timeout()), this, SLOT(animateLastAIPiece()));
	connect(&animat, SIGNAL(timeout()), this, SLOT(animateFive()));
}

Gomoku::~Gomoku() { }

void Gomoku::setCheatMode(bool cm) {
	this->cheatMode = cm;
	ui.checkBoxCheatMode->setChecked(cm);
}

void Gomoku::setPlayersSide(bool first) {
	this->playerFirst = first;
	if (first) ui.firstButton->setChecked(true);
	else ui.secondButton->setChecked(true);
	ui.playersPiece->setPixmap((first ? blackP : whiteP));
}

void Gomoku::clearGameBoard() {
	for (Loc loc1(0, 0); loc1.i < 15; ++loc1) {
		loc[loc1.i][loc1.j]->setPixmap(0);
	}
}

void Gomoku::aiMove() {
	QString msg;
	
	this->playersTurn = false;
	
	ui.messageBox->setText("AI thinking...");

	ui.pbAIThinking->setEnabled(true);
	ui.pbAIThinking->setTextVisible(true);
	
	ui.loadButton->setEnabled(false);
	ui.saveButton->setEnabled(false);
	
	this->animat.stop();
	
	ui.gameBoard->repaint();
	ui.messageBox->repaint();

	int score;
	Loc move;
	score = this->currentGameNode.eval_ext(ui.pbAIThinking, searchDepth, move);
	
	ui.loadButton->setEnabled(true);
	ui.saveButton->setEnabled(true);

	if (!move.isValid()) {
		if (move.i == move.j) {
			ui.messageBox->setText("Draw!");
			return;
		}
		
		ui.pbAIThinking->setEnabled(false);
		ui.pbAIThinking->setTextVisible(false);

		emit aiEvalTerminated();
	
		return;
	}

	int side = (this->currentGameNode.isBlacksTurn() ? 1 : -1);
	this->applyMoveElem(MoveElem(side, move));
	this->rewindRecord.clear();
	ui.forwardButton->setEnabled(false);

	if(this->currentGameNode.gameFinished()) {
		ui.messageBox->setText("AI won!");

		ui.pbAIThinking->setEnabled(false);
		ui.pbAIThinking->setTextVisible(false);

		this->currentGameNode.getFive(this->five);
		this->lastAIMove.i = -1;
		this->fiveStat = 0;
		this->animat.start(1000);
		
		return;
	}

	this->playersTurn = true;
	ui.messageBox->setText("Your turn.");
	
	ui.pbAIThinking->setEnabled(false);
	ui.pbAIThinking->setTextVisible(false);

	#ifdef DEBUG
	if (this->currentGameNode.isBlacksTurn()) cerr << "black's turn\n";
	else cerr << "white's turn\n";
	this->currentGameNode.printPttns(cerr, true);
#endif

	this->lastAIMove = move;
	this->lastPieceStat = 0;
	this->animat.start(1000);
}

bool Gomoku::applyMoveElem(const MoveElem& me) {
	if (me.getSide() != 1 && me.getSide() != -1) {
		this->clearGameBoard();
		this->currentGameNode.clear();
		this->setCheatMode(false);
		this->playersTurn = false;
		if (me.getSide() >= 0) setPlayersSide(true);
		else setPlayersSide(false);

		ui.messageBox->setText("");

		this->moveRecord.clear();
		ui.backwardButton->setEnabled(false);
		
		this->five.clear();
		this->animat.stop();
		
		return true;
	}
	
	if (this->currentGameNode.get(me.getLoc()) != 0) return false;
	if ((this->currentGameNode.isBlacksTurn() && me.getSide() == -1) &&
			(!this->currentGameNode.isBlacksTurn() && me.getSide() == 1)) {
		return false;
	}
	if (this->currentGameNode.gameFinished()) return false;

	Loc loc_ = me.getLoc();
	if (me.getSide() == 1) loc[loc_.i][loc_.j]->setPixmap(blackP);
	else loc[loc_.i][loc_.j]->setPixmap(whiteP);
	this->currentGameNode.move(loc_);
	
	this->moveRecord.push_back(me);
	ui.backwardButton->setEnabled(true);
	
	return true;
}

bool Gomoku::rewindOne() {
	if ((this->playersTurn && this->moveRecord.size() < 2) ||
			(!this->playersTurn && this->moveRecord.empty())) return false;
	
	MoveElem rewElem;
	Loc rewLoc;

	do {
		if (this->moveRecord.empty()) error("Gomoku::rewindOne");
		rewElem = this->moveRecord.back();
		this->moveRecord.pop_back();
		this->rewindRecord.push_back(rewElem);

		rewLoc = rewElem.getLoc();
		this->currentGameNode.put(rewLoc, 0);
		this->loc[rewLoc.i][rewLoc.j]->setPixmap(0);
		this->currentGameNode.setTurn((rewElem.getSide() > 0 ? true : false));
	} while (this->playerFirst != this->currentGameNode.isBlacksTurn());

	if (moveRecord.empty()) this->animat.stop();
	else {
		this->lastAIMove = this->moveRecord.back().getLoc();
	}
	
	this->currentGameNode.rebuildPttnDb();

	return true;
}

bool Gomoku::redoOne() {
	if (this->rewindRecord.size() < 2) return false;
	
	MoveElem rewElem;
	Loc rewLoc;

	rewElem = this->rewindRecord.back();
	this->rewindRecord.pop_back();

	this->applyMoveElem(rewElem);

	rewElem = this->rewindRecord.back();
	this->rewindRecord.pop_back();

	this->applyMoveElem(rewElem);

	if (this->rewindRecord.size() < 2) ui.forwardButton->setEnabled(false);
	this->lastAIMove = this->moveRecord.back().getLoc();

	return true;
}

void Gomoku::firstSelected() {
	if (this->cheatMode) {
		setPlayersSide(true);
		
		return;
	}

	if (this->currentGameNode.isBlank()) {
		setPlayersSide(true);
		this->playersTurn = true;
		ui.forwardButton->setEnabled(false);
	}
	else {
		if (!this->playerFirst) {
			ui.secondButton->setChecked(true);
		}
	}
}

void Gomoku::secondSelected() {
	if (this->cheatMode) {
		setPlayersSide(false);
		
		return;
	}

	if (this->currentGameNode.isBlank()) {
		setPlayersSide(false);
		ui.forwardButton->setEnabled(true);
	}
	else {
		if (this->playerFirst) {
			ui.firstButton->setChecked(true);
		}
	}
}

void Gomoku::locClicked(int i, int j, Qt::MouseButton b) {
	QString msg;
	QTextStream cmsg(&msg);

	if (this->cheatMode) {
		Loc loc_(i, j);
		if (b == Qt::LeftButton) {
			loc[i][j]->setPixmap((playerFirst ? blackP : whiteP));
			currentGameNode.put(loc_, (playerFirst ? 1 : -1));
		}
		else if (b == Qt::RightButton) {
			loc[i][j]->setPixmap(0);
			currentGameNode.put(loc_, 0);
		}
		
		return;
	}

	if (!this->playersTurn) return;
	if (this->currentGameNode.isBlank() && !this->playerFirst) return;
	if (this->currentGameNode.gameFinished()) return;
	if (b != Qt::LeftButton) return;
	Loc move(i, j);
	if (currentGameNode.get(move) != 0) return;
#ifdef DEBUG
	cerr << "player's made his/her move.\n";
#endif	
	
	applyMoveElem(MoveElem((this->playerFirst ? 1 : -1), move));
	this->rewindRecord.clear();
	ui.forwardButton->setEnabled(false);
	ui.firstButton->setEnabled(false);
	ui.secondButton->setEnabled(false);
	this->playersTurn = false;

	if(this->currentGameNode.gameFinished()) {
		ui.messageBox->setText("You won!");

		this->currentGameNode.getFive(this->five);
		this->lastAIMove.i = -1;
		this->fiveStat = 0;
		this->animat.start(1000);

		return;
	}

	aiMove();

	return;
}

void Gomoku::backwardClicked() {
	if (!this->playersTurn) this->currentGameNode.terminateEval();
	if (this->rewindOne()) ui.forwardButton->setEnabled(true);
	this->five.clear();
	this->playersTurn = true;
	ui.messageBox->setText("Your Turn");

	if (this->moveRecord.size() < 2) ui.backwardButton->setEnabled(false);
}

void Gomoku::forwardClicked() {
	QString msg;
	QTextStream cmsg(&msg);
	int score;
	Loc move;
#ifdef DEBUG
	cerr << "Forward clicked\n";
#endif

	if (this->cheatMode) {
		this->currentGameNode.rebuildPttnDb();
		score = this->currentGameNode.getStaticScore();
		cerr << "static score = " << score << endl;
		this->currentGameNode.printPttns(cerr, false);

		return;
	}
	
	if (this->currentGameNode.isBlank()) {
		if (this->playerFirst) return;
		
		ui.forwardButton->setEnabled(false);
		ui.firstButton->setEnabled(false);
		ui.secondButton->setEnabled(false);
		aiMove();

		return;
	}
	
	if (!this->playersTurn) return;
	
	this->redoOne();
	if (this->rewindRecord.size() < 2) ui.forwardButton->setEnabled(false);

	if (this->currentGameNode.gameFinished()) {
		ui.messageBox->setText("Game finished.");
		this->currentGameNode.getFive(this->five);
		this->lastAIMove.i = -1;
		this->fiveStat = 0;
		this->animat.start(1000);
	}

}

void Gomoku::clearClicked() {
	if (!this->playersTurn) this->currentGameNode.terminateEval();
	this->applyMoveElem(MoveElem((this->playerFirst ? 1 : -1)*10, Loc(0,0)));
	if (this->playerFirst) this->playersTurn = true;
	else {
		this->playersTurn = false;
		ui.forwardButton->setEnabled(true);
	}

	ui.firstButton->setEnabled(true);
	ui.secondButton->setEnabled(true);
}

void Gomoku::saveClicked() {
	QFile ofile("savegame.cgg");
	ofile.open(QFile::WriteOnly);
	QTextStream ofout(&ofile);

	ofout << (this->playerFirst ? 10 : -10) << '\t';
	ofout << 0 << '\t' << 0 << endl;
	
	vector<MoveElem>::const_iterator iRecord;
	for (iRecord = this->moveRecord.begin(); iRecord != this->moveRecord.end(); ++iRecord) {
		ofout << iRecord->getSide() << '\t';
		Loc loc_ = iRecord->getLoc();
		ofout << loc_.i << '\t' << loc_.j << endl;
	}

	ofile.close();
	
	ui.messageBox->setText("Game saved.");
}

void Gomoku::loadClicked() {
	QFile ifile("savegame.cgg");
	if (!ifile.open(QFile::ReadOnly)) {
		cerr << "file opening failure\n";
		return;
	}
	int side;
	Loc loc_;

	while(!ifile.atEnd()) {
		QByteArray line = ifile.readLine();
		if (line.length() < 5) break;
		QTextStream lin(&line);
		lin >> side;
		lin >> loc_.i;
		lin >> loc_.j;
		
		MoveElem me(side, loc_);
		this->applyMoveElem(me);
	}

	ifile.close();
	ui.messageBox->setText("Game loaded.");
	
	if (this->currentGameNode.isBlacksTurn() == this->playerFirst) this->playersTurn = true;
	else this->aiMove();
}

void Gomoku::CMStatusChanged(int) {
	if (ui.checkBoxCheatMode->isChecked()) this->cheatMode = true;
	else this->cheatMode = false;
}

void Gomoku::animateLastAIPiece() {
	if (!this->lastAIMove.isValid()) return ;

	int& i = this->lastAIMove.i;
	int& j = this->lastAIMove.j;
	
	if (this->lastPieceStat == 0) {
		this->loc[i][j]->setGeometry(
				23 + j*35, 16 + i*35, 35, 35);
		this->lastPieceStat = 1;
		this->animat_sub.singleShot(100, this, SLOT(animateLastAIPiece()));
	}
	else {
		this->loc[i][j]->setGeometry(
				23 + j*35, 18 + i*35, 35, 35);
		this->lastPieceStat = 0;
	}
}

void Gomoku::animateFive() {
	if (this->five.empty()) return;
	
	vector<Loc>::const_iterator iv;
	if (this->fiveStat == 0) {
		for (iv = five.begin(); iv != five.end(); ++iv) {
			const int& i = iv->i;
			const int& j = iv->j;
			
			this->loc[i][j]->setGeometry(
					22 + j*35, 18 + i*35, 35, 35);
		}
		this->fiveStat = 1;
		this->animat_sub.singleShot(100, this, SLOT(animateFive()));
	}
	else if (this->fiveStat == 1) {
		for (iv = five.begin(); iv != five.end(); ++iv) {
			const int& i = iv->i;
			const int& j = iv->j;
			
			this->loc[i][j]->setGeometry(
					24 + j*35, 18 + i*35, 35, 35);
		}
		this->fiveStat = 2;
		this->animat_sub.singleShot(100, this, SLOT(animateFive()));
	}
	else {
		for (iv = five.begin(); iv != five.end(); ++iv) {
			const int& i = iv->i;
			const int& j = iv->j;
			
			this->loc[i][j]->setGeometry(
					23 + j*35, 18 + i*35, 35, 35);
		}
		this->fiveStat = 0;
	}
}

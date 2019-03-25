#ifndef GOMOKUGAMENODE_H_
#define GOMOKUGAMENODE_H_

#include<iostream>
#include<set>
#include<map>
#include<QApplication>
#include<QProgressBar>
#include "gomokugame.h"

using namespace std;

typedef map<Loc, GomokuGameNode> ChildrenMap;
typedef ChildrenMap::iterator cm_iter;
const cm_iter cm_iter_null = (cm_iter)0;

typedef multimap<int, cm_iter, greater<int> > ChildrenRanking;

class GomokuGameNode {
	
public:
	GomokuGameNode();
	GomokuGameNode(const GomokuGameNode* ggn, const Loc& loc_);
	virtual ~GomokuGameNode();

	bool isBlank() const;
	void clear();

	inline int get(const Loc& loc_) const {
		return gameBoard.get(loc_);
	}
	
	void put(const Loc& loc_, int value_);
	
	bool isBlacksTurn() const;
	void setTurn(bool blacksTurn_);
	bool gameFinished() const;
	int getStaticScore() const;
	int_pair getNumStones() const;
	
	bool move(Loc& loc_);
	int eval_ext(QProgressBar* pb, int searchDepth, Loc& loc_);
	void terminateEval();
	void rebuildPttnDb();
	void getFive(vector<Loc>& five) const;
	
	void printPttns(ostream &os, bool onlyImportantPttns = false) const;
	
	static const int pttnScore[];
	static const int bigPositive = 35000;
	static const int bigNegative = -35000;

private:
	void scanPttn(SliceCoord& sc);
	void updatePttnDb(const Loc& loc_);
	void calcStaticScore();
	bool verifyPttnDb();
	int generateChildren(const PttnKey& pk, set<Loc> & generatedLoc,
		ChildrenMap& children, ChildrenRanking& childrenRanking);
	int generateChildrenNeighbor(int range, set<Loc>& generatedLoc,
			ChildrenMap& children, ChildrenRanking& childrenRanking);
	int generateChildrenRest(set<Loc>& generatedLoc,
			ChildrenMap& children, ChildrenRanking& childrenRanking);
	int eval(int searchDepth, int threshold);

	bool blacksTurn;
	bool pttnDbObsolete;
	bool game, sureGame;

	GomokuGameBoard gameBoard;
	PttnDb pttnDb;
	int staticScore;
	
	const GomokuGameNode *parent;
	
	bool stopNow;
};

bool operator<(const GomokuGameNode & ggn1, const GomokuGameNode & ggn2);

#endif /*GOMOKUGAMENODE_H_*/

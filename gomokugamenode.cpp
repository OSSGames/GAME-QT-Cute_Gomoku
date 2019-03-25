#include<deque>
#include "error.h"
#include "gomokugamenode.h"

GomokuGameNode::GomokuGameNode() {
	this->blacksTurn = true;
	this->staticScore = 0;
	this->game = false;
	this->sureGame = false;
	this->parent = 0;
	this->pttnDbObsolete = false;
	
}

GomokuGameNode::GomokuGameNode(const GomokuGameNode* ggn, const Loc& loc_) :
	gameBoard(ggn->gameBoard) {
	if (ggn->game) error("GomokuGameNode::GomokuGameNode: can't make a child node from a finished game.");
	if (ggn->pttnDbObsolete) {
		error("GomokuGameNode::GomokuGameNode: can't make a child node from a node with an obsolete pttnDb.");
	}
	if (!loc_.isValid()) error("GomokuGameNode::GomokuGameNode: loc is invalid.");
	if (ggn->get(loc_) != 0) error("GomokuGameNode::GomokuGameNode: loc isn't empty.");

	this->blacksTurn = !ggn->blacksTurn;
	this->parent = ggn;
	this->pttnDbObsolete = ggn->pttnDbObsolete;
	this->game = false;
	this->sureGame = ggn->sureGame;

	PttnDb::const_iterator iPttn;
	for (iPttn = ggn->pttnDb.begin(); iPttn != ggn->pttnDb.end(); ++iPttn) {
		const PttnKey &pk = iPttn->first;
		const PttnLoc &pl = iPttn->second;
		if (!ggsOnline(pl.direc, pl.nth, loc_)) {
			this->pttnDb.insert(make_pair(pk, pl));
		}
	}

	this->gameBoard.put(loc_, (ggn->blacksTurn ? 1 : -1));
	this->updatePttnDb(loc_);
}

GomokuGameNode::~GomokuGameNode() { }

bool GomokuGameNode::isBlank() const {
	for (Loc loc_(0,0); loc_.i < 15; ++loc_) {
		if (gameBoard.get(loc_) != 0) return false;
	}
	
	return true;
}

void GomokuGameNode::clear() {
	if (this->parent != 0) error("GomokuGameNode::clear: can't clear a dependent node");
	
	this->stopNow = true;
	
	this->gameBoard.clear();
	this->pttnDb.clear();
	this->pttnDbObsolete = false;
	this->staticScore = 0;
	this->blacksTurn = true;
	this->game = false;
	this->sureGame = false;
#ifdef DEBUG
	cerr << "node has cleared\n";
#endif
}

void GomokuGameNode::put(const Loc& loc_, int value_) {
	gameBoard.put(loc_, value_);
	this->pttnDbObsolete = true;
}

bool GomokuGameNode::isBlacksTurn() const {
	return this->blacksTurn;
}

void GomokuGameNode::setTurn(bool blacksTurn_) {
	this->blacksTurn = blacksTurn_;
}

bool GomokuGameNode::gameFinished() const {
	return this->game;
}

int GomokuGameNode::getStaticScore() const {
	return this->staticScore;
}

int_pair GomokuGameNode::getNumStones() const {
	int black = 0, white = 0;
	int_pair p;
	
	for (Loc loc_(0, 0); loc_.i < 15; ++loc_) {
		if (gameBoard.get(loc_) > 0) ++black;
		else if (gameBoard.get(loc_) < 0) ++white;
	}
	return make_pair(black, white);
}

bool GomokuGameNode::move(Loc& loc_) {
	if (this->get(loc_) != 0) return false;
	
	this->gameBoard.put(loc_, (blacksTurn ? 1 : -1));
	if (!this->game) this->blacksTurn = !blacksTurn;
	this->rebuildPttnDb();
	
	return true;
}

int GomokuGameNode::eval_ext(QProgressBar *pb, int searchDepth, Loc& bestMove) {
	stopNow = false;

	pb->setValue(0);
	qApp->processEvents();
	if (stopNow) {
		bestMove.i = -1;
		bestMove.j = 1;
		cerr << "exiting GomokuGameNode::eval_ext\n";
		return 0;
	}

	if (this->pttnDbObsolete) this->rebuildPttnDb();
	int_pair p = this->getNumStones();

	if (this->isBlank() && this->blacksTurn) {
		bestMove.i = 7;
		bestMove.j = 7;
		
		return 0;
	}
	else {
		if (p.first == 1 && p.second == 0 && !this->blacksTurn &&
				this->get(Loc(7, 7)) == 1) {
			bestMove.i = 6;
			bestMove.j = 6;
				
			return 0;
		}
		else if (p.first == 2 && p.second == 1 && !this->blacksTurn &&
				this->get(Loc(7, 7)) == 1 &&
				this->get(Loc(6, 6)) == -1) {
			if (this->get(Loc(8, 6)) == 1) {
				bestMove.i = 9;
				bestMove.j = 7;
				
				return 0;
			}
			else if (this->get(Loc(6, 8)) == 1) {
				bestMove.i = 7;
				bestMove.j = 9;
				
				return 0;
			}
		}
		else if (p.first == 1 && p.second == 1 && this->blacksTurn &&
				this->get(Loc(7, 7)) == 1 &&
				this->get(Loc(6, 6)) == -1) {
			bestMove.i = 6;
			bestMove.j = 8;
			
			return 0;
		}
	}

	PttnKey pk;
	PttnDb::const_iterator iPttnDbLb;
/*
	pk.side = 1;
	pk.kindPttn = Pttn::KOSAN;
	iPttnDbLb = pttnDb.upper_bound(pk);
	if (pttnDb.begin() != iPttnDbLb) {
#ifdef DEBUG
		cerr << "Boosting Search\n";
#endif
		searchDepth += 1;
	}
*/
	if (p.first + p.second >= 10) searchDepth += 1;
	
	int opt, e, numGenerated = 0;
	if (this->blacksTurn) opt = bigNegative;
	else opt = bigPositive;

	ChildrenMap children;
	ChildrenRanking childrenRanking;
	set<Loc> generatedLoc;

	cm_iter iChild, mChild = cm_iter_null;
	PttnDb::iterator iPttnDb;
	ChildrenRanking::iterator iRanking;
	int numProcessed = 0;
	int progress;

	int mySide = (this->blacksTurn ? 1 : -1);

	pk.side = mySide;
	pk.kindPttn = Pttn::TATSUSHI;
	numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

	pk.kindPttn = Pttn::SHI;
	numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

	if (numGenerated == 0) {
		pk.side = - mySide;
		pk.kindPttn = Pttn::TATSUSHI;
		numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

		pk.kindPttn = Pttn::SHI;
		numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

		if (numGenerated == 0) {
			pk.kindPttn = Pttn::SAN;
			pk.side = mySide;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

			if (numGenerated == 0) {
				pk.kindPttn = Pttn::SAN;
				pk.side = -mySide;
				numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

				pk.side = mySide;
				pk.kindPttn = Pttn::KOSAN;
				if (numGenerated == 0 ||	this->pttnDb.find(pk) != this->pttnDb.end()) {
					numGenerated += generateChildrenRest(generatedLoc, children, childrenRanking);
				}
			}
		}
	}

//	numGenerated = generateChildrenRest(generatedLoc, children, childrenRanking);

	if (numGenerated == 0) {
		bestMove.i = bestMove.j = -1;
		return 0;
	}
	
	if (numGenerated == 1) {
		bestMove = children.begin()->first;
		return 0;
	}

	for (iRanking = childrenRanking.begin();
		iRanking != childrenRanking.end(); ++iRanking) {
		iChild = iRanking->second;
		GomokuGameNode &ggn = iChild->second;
#ifdef DEBUG
		cerr << "(" << (iChild->first).i << "," << (iChild->first).j << ")";
#endif
		e = ggn.eval(searchDepth - 1, opt);
#ifdef DEBUG
		cerr << e;
#endif

		if ((this->blacksTurn && e > opt) || (!this->blacksTurn && e < opt)) {
#ifdef DEBUG
			cerr << " opt updated";
#endif
			opt = e;
			mChild = iChild;
		}
#ifdef DEBUG
		cerr << endl;
#endif
		++numProcessed;
		progress = (int)(100.0 * numProcessed / numGenerated);

		pb->setValue(progress);
		qApp->processEvents();
		if (stopNow) {
			bestMove.i = -1;
			bestMove.j = 1;
			cerr << "exiting GomokuGameNode::eval_ext\n";
			return 0;
		}
	}

	if(mChild == cm_iter_null) error("GomokuGameNode::eval_ext: eval didn't work well...");
	bestMove = mChild->first;
#ifdef DEBUG
	cerr << "AI's made its move. opt = " << opt << endl;
#endif
	return opt;
}

void GomokuGameNode::terminateEval() {
	stopNow = true;
}

void GomokuGameNode::printPttns(ostream &os, bool onlyImportantPttns) const {
	os << "(" << this->staticScore << ") ";
	if (!pttnDb.empty()) {
		if (onlyImportantPttns) os << "only important pttns ";
		PttnDb::const_iterator ip;
		for(ip = pttnDb.begin(); ip != pttnDb.end(); ++ip) {
			const PttnKey &pk = ip->first;
			const PttnLoc &pl = ip->second;
			if (onlyImportantPttns && pk.kindPttn > Pttn::KOSAN) continue;
			cerr << "(" << pk.side << "," << pk.kindPttn << ")";
			cerr << "(" << pl.direc << "," << pl.nth << ",";
			cerr << pl.begin << "," << pl.end << ") ";
		}
		cerr << endl;
	}
}

void GomokuGameNode::rebuildPttnDb() {
	this->pttnDb.clear();
	this->game = false;
	this->sureGame = false;

	SliceCoord sc;
	this->scanPttn(sc);

	this->pttnDbObsolete = false;
	this->calcStaticScore();
}

void GomokuGameNode::getFive(vector<Loc>& five) const {
	PttnKey pk;
	PttnDb::const_iterator ip, iplb, ipub;
	

	pk.kindPttn = Pttn::GO;
	for (pk.side = -1; pk.side <= 1; pk.side += 2) {
		if ((ip = this->pttnDb.find(pk)) != this->pttnDb.end()) {
			const PttnLoc& pl = ip->second;
			Loc loc0 = GgSliceP0(pl.direc, pl.nth);
			Loc loc = loc0 + GgSlice::dirc[pl.direc] * pl.begin;
			Loc locEnd = loc0 + GgSlice::dirc[pl.direc] * pl.end;
			
			for ( ; loc != locEnd; loc += GgSlice::dirc[pl.direc]) five.push_back(loc);
		}
	}
}

void GomokuGameNode::scanPttn(SliceCoord& sc) {
	Pttn pe;
	ggs_iter first, last;
	deque<ggs_iter_pair> sliceRange;
	PttnKey pk;
	PttnLoc pl;

	do {
		GgSlice slice(this, sc);
		for (int side = -1; side <= 1; side+= 2) {
			sliceRange.clear();
			sliceRange.push_front(make_pair(slice.begin(), slice.end()));
			sliceRange.push_front(ggs_iter_pair_null);

			for (int iPass = 0; iPass < Pttn::numPasses; ++iPass) {
				while(sliceRange.back() != ggs_iter_pair_null) {
					first = sliceRange.back().first;
					last = sliceRange.back().second;
					sliceRange.pop_back();

					while((pe = Pttn(side, iPass, first, last)).found()) {
						if(pe.kindPttn != Pttn::BLANK) {
							pk.side = pe.side;
							pk.kindPttn = pe.kindPttn;
							
							pl.direc = sc.getDirec();
							pl.nth = sc.getNth();
							pl.begin = pe.begin;
							pl.end = pe.end;
							if (!pk.isValid()) error("GomokuGameNode::scanPttn: invalid PttnKey");
							if (!pl.isValid()) {
								error("GomokuGameNode::scanPttn: invalid PttnLoc");
							}
							this->pttnDb.insert(make_pair(pk, pl));
						}
						if (pe.begin >= 5) {
							sliceRange.push_front(make_pair(first, first + pe.begin));
						}
						first+= pe.end;
					}

					if (last - first >= 5) {
						sliceRange.push_front(make_pair(first, last));
					}
				}
				sliceRange.pop_back();
				sliceRange.push_front(ggs_iter_pair_null);
			}
		}
	} while (sc.next());
}

void GomokuGameNode::updatePttnDb(const Loc& loc_) {
	if (this->pttnDbObsolete) {
		error("GomokuGameNode::updatePttnDb: can't update an obsolete pttnDn.");
	}
	SliceCoordConstrained scc(loc_);
	this->scanPttn(scc);
	this->calcStaticScore();
}

const int GomokuGameNode::pttnScore[8] = {0, 10000, 10000,  60, 50, 10, 5, 1};
//                               { BLANK, GO, TATSUSHI, SHI, SAN, KOSAN, NI, ICHI }

void GomokuGameNode::calcStaticScore() {
	// now its my turn, just before I make my move...
	
	PttnKey pk1;
	PttnDb::const_iterator ip, iplb, ipub;
	
	int mySide = (this->blacksTurn ? 1 : -1);

	// if I have GO
	pk1.side = mySide;
	pk1.kindPttn = Pttn::GO;
	if (this->pttnDb.find(pk1) != this->pttnDb.end()) {
		this->staticScore = pk1.side * 30000;
		this->game = this->sureGame = true;
	}
	
	// if Enemy has GO
	pk1.side = - mySide;
	pk1.kindPttn = Pttn::GO;
	if (this->pttnDb.find(pk1) != this->pttnDb.end()) {
		if (this->game) error("GomokuGameNode::calcStaticScore: contradiction");
		this->staticScore = pk1.side * 30000;
		this->game = this->sureGame = true;
	}

	if (this->game) return;
	
	// if I have TATSUSHI
	pk1.side = mySide;
	pk1.kindPttn = Pttn::TATSUSHI;
	if (this->pttnDb.find(pk1) != this->pttnDb.end()) {
		this->sureGame = true;
		this->staticScore = pk1.side * 29999;
		return;
	}

	// if I have SHI
	pk1.kindPttn = Pttn::SHI;
	if (this->pttnDb.find(pk1) != this->pttnDb.end()) {
		this->sureGame = true;
		this->staticScore = pk1.side * 29999;
		return;
	}

	// if Enemy has TATSUSHI
	pk1.side = - mySide;
	pk1.kindPttn = Pttn::TATSUSHI;
	if (this->pttnDb.find(pk1) != this->pttnDb.end()) {
		this->sureGame = true;
		this->staticScore = pk1.side * 29998;
		return;
	}

	// if Enemy doesn't have SHI and I have SAN
	bool enemyHasShi = false;
	
	pk1.side = - mySide;
	pk1.kindPttn = Pttn::SHI;
	if (this->pttnDb.find(pk1) == this->pttnDb.end()) { // enemy doesn't have SHI
		pk1.side = mySide;
		pk1.kindPttn = Pttn::SAN;
		if (this->pttnDb.find(pk1) != this->pttnDb.end()) { // I have SAN
			this->sureGame = true;
			this->staticScore = mySide * 29997;
			return;
		}
	}
	else enemyHasShi = true;

	vector<PttnDb::const_iterator> shiSan;

	// if Enemy has shisan or sansan
	
	bool enemyHasSanSan = false;
	
	pk1.side = -mySide;
	pk1.kindPttn = Pttn::SHI;
	if ((iplb = this->pttnDb.lower_bound(pk1)) != this->pttnDb.end()) {
		ipub = this->pttnDb.upper_bound(pk1);
		for (ip = iplb; ip != ipub; ++ip) shiSan.push_back(ip);
	}

	pk1.side = -mySide;
	pk1.kindPttn = Pttn::SAN;
	if ((iplb = this->pttnDb.lower_bound(pk1)) != this->pttnDb.end()) {
		ipub = this->pttnDb.upper_bound(pk1);
		for (ip = iplb; ip != ipub; ++ip) shiSan.push_back(ip);
	}

	if (shiSan.size() >= 2) {
		vector<PttnDb::const_iterator>::const_iterator i, j;
		for (i = shiSan.begin(); i != shiSan.end() - 1; ++i) {
			for (j = i + 1; j != shiSan.end(); j++) {
				if (indepPttns((*i)->second, (*j)->second, this)) {
					if (((*i)->first).kindPttn == Pttn::SHI ||
							((*j)->first).kindPttn == Pttn::SHI) {
						this->sureGame = true;
						this->staticScore = - mySide * 29996;
					
						return;
					}
					else enemyHasSanSan = true;
				}
			}
		}
	}

	if (enemyHasSanSan) {
		bool iHaveKosanAL = false;
		pk1.side = mySide;
		pk1.kindPttn = Pttn::SAN;
		if (this->pttnDb.find(pk1) != this->pttnDb.end()) iHaveKosanAL = true;
		else {
			pk1.side = mySide;
			pk1.kindPttn = Pttn::KOSAN;
			if (this->pttnDb.find(pk1) != this->pttnDb.end()) iHaveKosanAL = true;
		}
		
		if (!iHaveKosanAL) {
			this->sureGame = true;
			this->staticScore = - mySide * 29996;
		}
	}

	
	this->staticScore = 0;
	pk1.side = - 1;
	pk1.kindPttn = Pttn::SHI;
	iplb = this->pttnDb.lower_bound(pk1);
	for(ip = iplb; ip != pttnDb.end(); ++ip) {
		const PttnKey &pk = ip->first;
		this->staticScore += pk.side * GomokuGameNode::pttnScore[pk.kindPttn];
	}

	
/*	
	pk1.side = -mySide;
	pk1.kindPttn = Pttn::SHI;
	iplb = this->pttnDb.lower_bound(pk1);

	pk1.side = mySide;
	pk1.kindPttn = Pttn::NI;
	ipub = this->pttnDb.upper_bound(pk1);

	int cBlack = 0, cWhite = 0;
	while (iplb != ipub) {
		const PttnKey &pk = iplb->first;
		if (pk.side > 0) ++cBlack;
		else ++cWhite;
			
		++iplb;
	}

	if (cBlack > 1) this->staticScore += 20*(cBlack - 1) * (cBlack - 1);
	if (cWhite > 1) this->staticScore -= 20*(cWhite - 1) * (cWhite - 1);
*/
}

bool GomokuGameNode::verifyPttnDb() {
	PttnDb pttnDbBackUp;
	PttnDb::iterator iPttnDb;
	
	for (iPttnDb = this->pttnDb.begin(); iPttnDb != this->pttnDb.end(); ++iPttnDb) {
		pttnDbBackUp.insert(*iPttnDb);
	}
	this->pttnDb.clear();
	this->rebuildPttnDb();
	
	if(pttnDb.size() != pttnDbBackUp.size()) return false;

	for (iPttnDb = this->pttnDb.begin(); iPttnDb != this->pttnDb.end(); ++iPttnDb) {
		const PttnKey &pk = iPttnDb->first;
		if (pttnDbBackUp.find(pk) == pttnDbBackUp.end()) return false;
	}
	return true;
}

int GomokuGameNode::generateChildren(const PttnKey& pk, set<Loc>& generatedLoc,
		ChildrenMap& children, ChildrenRanking& childrenRanking) {
	PttnDb::iterator iPttnDb, lbpk, ubpk;
	ChildrenMap::iterator iChild;
	
	lbpk = pttnDb.lower_bound(pk);
	if (lbpk == pttnDb.end()) return 0;
	else ubpk = pttnDb.upper_bound(pk);

	int numGenerated = 0;
	for (iPttnDb = lbpk; iPttnDb != ubpk; ++iPttnDb) {
		PttnLoc &pl = iPttnDb->second;
		
		Loc loc0 = GgSliceP0(pl.direc, pl.nth);
		Loc locBegin = loc0 +  GgSlice::dirc[pl.direc] * pl.begin;
		Loc locEnd = loc0 +  GgSlice::dirc[pl.direc] * pl.end;
		
		for (Loc loc_ = locBegin; loc_ != locEnd; loc_ += GgSlice::dirc[pl.direc]) {
			if (!loc_.isValid()) {
				error("GomokuGameNode::generateChildren: invalid loc_");
			}
			if (this->get(loc_) == 0 && generatedLoc.find(loc_) == generatedLoc.end()) {
				++numGenerated;
				generatedLoc.insert(loc_);
				GomokuGameNode ggn(this, loc_);
				int s = ggn.getStaticScore();
				if (!this->blacksTurn) s = -s;
				iChild = children.insert(children.end(), make_pair(loc_, ggn));
				childrenRanking.insert(make_pair(s, iChild));
			}
		}
	}
	return numGenerated;
}

int GomokuGameNode::generateChildrenNeighbor(int range, set<Loc>& generatedLoc,
		ChildrenMap& children, ChildrenRanking& childrenRanking) {
	int numGenerated = 0;
	ChildrenMap::iterator iChild;
	
	for (Loc loc_(0, 0); loc_.i != 15; ++loc_) {
		if (this->get(loc_) == 0) continue;
		
		for (int direc = 0; direc < 4; ++direc) {
			Loc locPlus, locMinus;
			locPlus = locMinus = loc_;

			for(int k = 1; k <= range; ++k) {
				if (!(locPlus += GgSlice::dirc[direc]).isValid()) break;
				
				if (generatedLoc.find(locPlus) == generatedLoc.end() &&
						this->get(locPlus) == 0) {
					++numGenerated;
					generatedLoc.insert(locPlus);

					GomokuGameNode ggn(this, locPlus);
					int s = ggn.getStaticScore();
					if (!this->blacksTurn) s = -s;
					iChild = children.insert(children.end(), make_pair(locPlus, ggn));
					childrenRanking.insert(make_pair(s, iChild));
				}
			}
			for(int k = 1; k <= range; ++k) {
				if (!(locMinus -= GgSlice::dirc[direc]).isValid()) break;
				
				if (generatedLoc.find(locMinus) == generatedLoc.end() &&
						this->get(locMinus) == 0) {
					++numGenerated;
					generatedLoc.insert(locMinus);

					GomokuGameNode ggn(this, locMinus);
					int s = ggn.getStaticScore();
					if (!this->blacksTurn) s = -s;
					iChild = children.insert(children.end(), make_pair(locMinus, ggn));
					childrenRanking.insert(make_pair(s, iChild));
				}
			}
		}
	}
	
	return numGenerated;
}

int GomokuGameNode::generateChildrenRest(set<Loc>& generatedLoc,
		ChildrenMap& children, ChildrenRanking& childrenRanking) {
	int numGenerated = 0;
	ChildrenMap::iterator iChild;
	
	for (Loc loc_(0, 0); loc_.i != 15; ++loc_) {
		if (this->get(loc_) != 0 ||
				generatedLoc.find(loc_) != generatedLoc.end()) continue;
		
		++numGenerated;
		generatedLoc.insert(loc_);

		GomokuGameNode ggn(this, loc_);
		int s = ggn.getStaticScore();
		if (!this->blacksTurn) s = -s;
		iChild = children.insert(children.end(), make_pair(loc_, ggn));
		childrenRanking.insert(make_pair(s, iChild));
	}
	
	return numGenerated;
}

int discScore(int score) {
	if (score > 10000) return score - 1;
	else if (score < -10000) return score + 1;
	return score;
}

int GomokuGameNode::eval(int searchDepth, int threshold) {
	static const int numPasses = 7;
	
	if (this->game || this->sureGame || searchDepth == 0) {
//		if (!this->verifyPttnDb()) {
//			cerr << "pttnDb invalid\n";
//		}
		
		return discScore(this->staticScore);
	}

	int opt, e, iPass, numGenerated, nm, ne;
	if (this->blacksTurn) opt = bigNegative;
	else opt = bigPositive;
	int mySide = (this->blacksTurn ? 1 : -1);

	ChildrenMap children;
	ChildrenRanking childrenRanking;
	set<Loc> generatedLoc;
	
	cm_iter iChild, mChild = cm_iter_null;
	bool thresholdHit = false;
	bool childExists = false;
	PttnDb::iterator iPttnDb;
	ChildrenRanking::iterator iRanking;
	PttnKey pk;
	
	if (threshold == bigPositive || threshold == bigNegative) iPass = numPasses - 1;
	else iPass = 0;

	for ( ; iPass < numPasses; ++iPass) {
		numGenerated = 0;
		switch (iPass) {
		case 0:
			pk.side = mySide;
			pk.kindPttn = Pttn::TATSUSHI;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

			pk.kindPttn = Pttn::SHI;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

			if (numGenerated > 0) {
				iPass = numPasses;
				break;
			}

			pk.side = - mySide;
			pk.kindPttn = Pttn::TATSUSHI;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);
			
			pk.kindPttn = Pttn::SHI;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

			if (numGenerated > 0) {
				iPass = numPasses;
				break;
			}

			break;

		case 1:
			pk.kindPttn = Pttn::SAN;
			pk.side = mySide;
			numGenerated += nm = generateChildren(pk, generatedLoc, children, childrenRanking);

			if (numGenerated > 0) {
				iPass = numPasses;
				break;
			}

			pk.side = -mySide;
			numGenerated += ne = generateChildren(pk, generatedLoc, children, childrenRanking);

			pk.side = mySide;
			pk.kindPttn = Pttn::KOSAN;
			if (ne > 0 &&	nm == 0 && this->pttnDb.find(pk) == this->pttnDb.end()) {
				iPass = numPasses;
				break;
			}

			break;
			
		case 2:
			pk.kindPttn = Pttn::KOSAN;
			pk.side = mySide;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);
			pk.side = - mySide;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);
			
			break;
			
		case 3:

			pk.kindPttn = Pttn::NI;
			pk.side = 1;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);
			pk.side = -1;
			numGenerated += generateChildren(pk, generatedLoc, children, childrenRanking);

			break;
			
		case 4:
			numGenerated += generateChildrenNeighbor(1, generatedLoc,
					children, childrenRanking);

			break;
			
		case 5:
			numGenerated += generateChildrenNeighbor(5, generatedLoc,
					children, childrenRanking);

			break;

		default:
			numGenerated += generateChildrenRest(generatedLoc, children, childrenRanking);
		}
		
		if (numGenerated == 0) continue;
		childExists = true;
		
		for (iRanking = childrenRanking.begin();
		iRanking != childrenRanking.end(); ++iRanking) {
			iChild = iRanking->second;
			GomokuGameNode &ggn = iChild->second;
				
			e = ggn.eval(searchDepth - 1, opt);
			if ((this->blacksTurn && e > opt) || (!this->blacksTurn && e < opt)) {
				opt = e;
				mChild = iChild;
				
				if((this->blacksTurn && opt > threshold) ||
						(!this->blacksTurn && opt < threshold)) {
#ifdef DEBUG
//					cerr << "threshold cut " << searchDepth << " " << iPass << endl;
#endif
					thresholdHit = true;
					break;
				}
			}
		}
		children.clear();
		childrenRanking.clear();
		
		if(thresholdHit) break;
	}
#ifdef DEBUG
//		if (iPass > 6) cerr << "no threshold cut " << searchDepth << endl;
#endif		
	if(childExists && mChild == cm_iter_null) error("GomokuGameNode::eval: eval didn't work well...");
	return discScore(opt);
}

bool operator<(const GomokuGameNode & ggn1, const GomokuGameNode & ggn2) {
	return (ggn1.getStaticScore() < ggn2.getStaticScore());
}

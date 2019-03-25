#include<map>
#include<functional>
#include "error.h"
#include "gomokugamenode.h"

using namespace std;

bool Loc::isValid() const {
	return (0 <= i && i < 15 && 0 <= j && j < 15);
}

const Loc& Loc::operator++() {
	j++;
	if (j == 15) {
		i++;
		j = 0;
	}
	return *this;
}

const Loc& Loc::operator=(const Loc& loc_) {
	i = loc_.i;
	j = loc_.j;
		
	return *this;
}
	
const Loc& Loc::operator+=(const Loc& loc_) {
	i += loc_.i;
	j += loc_.j;
	
	return *this;
}
	
const Loc& Loc::operator-=(const Loc& loc_) {
	i -= loc_.i;
	j -= loc_.j;
	
	return *this;
}

Loc Loc::operator+(const Loc& loc_) {
	Loc t(*this);
	t += loc_;
	return(t);
}

const Loc& Loc::operator*=(int m) {
	i *= m;
	j *= m;
		
	return *this;
}
	
Loc Loc::operator*(int m) const {
	return(Loc(*this) *= m);
}

bool operator<(const Loc& l, const Loc& m) {
	return((l.i << 8) + l.j < (m.i << 8) + m.j);
}

bool operator!=(const Loc& l, const Loc& m) {
	return((l.i << 8) + l.j != (m.i << 8) + m.j);
}

GomokuGameBoard::GomokuGameBoard() {
	for (int i = 0; i < 15; ++i) for (int j = 0; j < 15; ++j) this->boardData[i][j] = 0;
}

GomokuGameBoard::GomokuGameBoard(const GomokuGameBoard& ggb) {
	for (int i = 0; i < 15; ++i) for (int j = 0; j < 15; ++j) {
		this->boardData[i][j] = ggb.boardData[i][j];
	}
}

void GomokuGameBoard::clear() {
	for (int i = 0; i < 15; ++i) for (int j = 0; j < 15; ++j) this->boardData[i][j] = 0;
}


SliceCoord::SliceCoord()
: direc(0), nth(GgSlice::effectiveRange[direc].first) { }

SliceCoord::SliceCoord(int direc_, int nth_)
: direc(direc_), nth(nth_) {
	if (!(0 <= direc && direc < 4 && 
			GgSlice::effectiveRange[direc].first <= nth && 
			nth < GgSlice::effectiveRange[direc].second)) {
		error("SliceCoord::SliceCoord");
	}
}

SliceCoord::SliceCoord(const SliceCoord& sc)
: direc(sc.direc), nth(sc.nth) {
	if (!(0 <= direc && direc < 4 && 
			GgSlice::effectiveRange[direc].first <= nth && 
			nth < GgSlice::effectiveRange[direc].second)) {
		error("SliceCoord::SliceCoord");
	}
}

int SliceCoord::getDirec() const {
	return direc;
}

int SliceCoord::getNth() const {
	return nth;
}

void SliceCoord::set(int direc_, int nth_) {
	this->direc = direc_;
	this->nth = nth_;

	if (!(0 <= direc && direc < 4 && 
			GgSlice::effectiveRange[direc].first <= nth && 
			nth < GgSlice::effectiveRange[direc].second)) {
		error("SliceCoord::set");
	}
}

bool SliceCoord::next() {
	bool increment = true;
	
	++nth;
	
	if(nth == GgSlice::effectiveRange[direc].second) {
		if(++direc == 4) {
			increment = false;
			direc = 0;
		}
		nth = GgSlice::effectiveRange[direc].first;
	}

	return increment;
}

bool operator==(const SliceCoord& s, const SliceCoord& t) {
	return((s.getDirec() ==t.getDirec()) && (s.getNth() == t.getNth()));
}

bool operator<(const SliceCoord& s, const SliceCoord& t) {
	return((s.getDirec() << 8) + s.getNth() < (t.getDirec() << 8) + t.getNth());
}

SliceCoordConstrained::SliceCoordConstrained(const Loc& changedLoc) : SliceCoord() {
	int direc, nth;
	
	direc = 0;
	nth = changedLoc.i + changedLoc.j;
	if (GgSlice::effectiveRange[direc].first <= nth &&
			nth < GgSlice::effectiveRange[direc].second) {
		SCChain.push_back(SliceCoord(direc, nth));
	}
	
	direc = 1;
	nth = changedLoc.i;
	SCChain.push_back(SliceCoord(direc, nth));

	direc = 2;
	nth = 14 + changedLoc.i - changedLoc.j;
	if (GgSlice::effectiveRange[direc].first <= nth &&
			nth < GgSlice::effectiveRange[direc].second) {
		SCChain.push_back(SliceCoord(direc, nth));
	}

	direc = 3;
	nth = changedLoc.j;
	SCChain.push_back(SliceCoord(direc, nth));

	this->iSCChain = 0;
	this->set(SCChain[this->iSCChain].getDirec(), SCChain[this->iSCChain].getNth());
}

bool SliceCoordConstrained::next() {
	bool increment = true;
	
	++this->iSCChain;
	if (this->iSCChain == SCChain.size()) {
		this->iSCChain = 0;
		increment = false;
	}

	this->set(SCChain[this->iSCChain].getDirec(), SCChain[this->iSCChain].getNth());

	return increment;
}

void detectPttn5(int target, int side, ggs_iter first, ggs_iter last,
		bool& foundMyStone, int& kindPttn, int& begin, int& end) {
	ggs_iter s = first, sp, t, u;

	if (target < 1 || 4 < target) error("bad scan");

	while(true) {
		if ((s = find(s, last, side)) < last) foundMyStone = true;
//		cerr << "SHI\n";
		if (s > last - target) break;
		sp = t = s;
		u = s + 5;
		
		if (u >= last) {
			int pb = u - last;
			t = s - pb;
			u = last;
			if (t < first || count(t, s, 0) < pb) break;
		}
		
		++s;
		int cMyStone = 1;
		bool foundEnemyStone = false;

		for ( ; s < u; ++s) {
			if (*s == -side) {
				foundEnemyStone = true;
				break;
			}
			if (*s == side) ++cMyStone;
			if (s + target - cMyStone > u) break;
		}

		if (foundEnemyStone) {
			if (cMyStone != target) continue;
			int bm = u - s;
			if (t - bm < first) continue;
			if (count(t - bm, t, 0) != bm) continue;

			t-= bm;
		}

		if (cMyStone == target) {
			if (target == 4)	kindPttn = Pttn::SHI;
			else if (target == 3) kindPttn = Pttn::KOSAN;
			else if (target == 2) kindPttn = Pttn::NI;
			else kindPttn = Pttn::ICHI;

			if (t > first && *(t - 1) == 0 && *(s - 1) == 0) {
				--t;
				--s;
			}
			
			begin = t - first;
			end = s - first;

			return;
		}

		s = sp + 1;
	}
}

Pttn::Pttn(int side_, int pass, ggs_iter first, ggs_iter last) : side(side_) {
	if (last - first < 5) {
		kindPttn = 0;
		begin = 0;
		end = last - first;
		
		return;
	}

	ggs_iter s = first, t, u;
	int cMyStone;
	bool foundEnemyStone;
	bool foundMyStone = false;
	begin = end = 0;

	switch (pass) {
	case 0: // detect GO
		while (true) {
			if ((s = find(s, last, side)) < last) foundMyStone = true;
//			cerr << "GO\n";
			if (s > last - 5) break;
			t = s;
			cMyStone = 1;
			while(++s != last && *s == side) cMyStone++;
			if(cMyStone >= 5) {
				kindPttn = Pttn::GO;
				begin = t - first;
				end = s - first;
				
				return;
			}
		}
		break;
		
	case 1: // detect TATSUSHI
		if(*(s++) == side) foundMyStone = true;
		while(true) {
			if ((s = find(s, last, side)) < last) foundMyStone = true;
//			cerr << "TATSUSHI\n";
			if (s > last - 5) break;
			if (*(t = s - 1) != 0) {
				s += 2;
				continue;
			}
			
			if (*(u = s + 4) != 0) {
				if(--t < first || *t != 0 || *(--u) != 0) {
					s += 2;
					continue;
				}
			}
			
			cMyStone = 1;
			++s;
			foundEnemyStone = false;
			for ( ; s < u; ++s) {
				if (*s == -side) {
					foundEnemyStone = true;
					break;
				}
				if (*s == side) ++cMyStone;
			}
			
			if (foundEnemyStone) continue;

			if (cMyStone == 4) {
				kindPttn = Pttn::TATSUSHI;
				begin = t - first;
				end = u + 1 - first;
				
				return;
			}
		}

		break;
		
	case 2: // detect SHI
		detectPttn5(4, side, first, last, foundMyStone, kindPttn, begin, end);
		if (begin != end) return;
		break;

	case 3: // SAN
		if(*(s++) == side) foundMyStone = true;;
		while(true) {
			if ((s = find(s, last, side)) < last) foundMyStone = true;
//			cerr << "SAN\n";
			if (s > last - 5) break;
			if (*(t = s - 1) != 0) {
				++s;
				continue;
			}
			
			if (*(u = s + 4) != 0) {
				if(--t < first || *t != 0 || *(--u) != 0) {
					++s;
					continue;
				}
			}
			
			cMyStone = 1;
			++s;
			foundEnemyStone = false;
			for ( ; s < u; ++s) {
				if (*s == -side) {
					foundEnemyStone = true;
					break;
				}
				if (*s == side) ++cMyStone;
			}
			
			if (foundEnemyStone) continue;

			if(cMyStone == 3) {
				kindPttn = Pttn::SAN;
				begin = t - first;
				end = u + 1 - first;
				
				return;
			}
		}

		break;
		
	default: // Pass 4..6 detect KOSAN, NI, ICHI
		detectPttn5(7 - pass, side, first, last, foundMyStone, kindPttn, begin, end);
		if (begin != end) return;
		break;

	}

	if (foundMyStone) {
		kindPttn = Pttn::BLANK;
		begin = last - first;
		end = last - first;
	}
	else {
		kindPttn = Pttn::BLANK;
		begin = 0;
		end = last - first;
	}
}

bool Pttn::found() const {
	return (begin < end);
}

const Loc GgSlice::dirc[] = { Loc(-1, 1), Loc(0, 1), Loc(1, 1), Loc(1, 0) };
const int GgSlice::numSlices[] = { 29, 15, 29, 15 };
const int_pair GgSlice::effectiveRange[] = {
	int_pair(4, 25),
	int_pair(0, 15),
	int_pair(4, 25),
	int_pair(0, 15)
};

Loc GgSliceP0(int direc, int nth) {
	Loc l;

	switch (direc) {
	case 0:
		if (nth < 15) {
			l.i = nth;
			l.j = 0;
		}
		else {
			l.i = 14;
			l.j = nth - 14;
		}

		break;
	case 1:
		l.i = nth;
		l.j = 0;
		
		break;
	case 2:
		if (nth < 15) {
			l.i = 0;
			l.j = 14 - nth;
		}
		else {
			l.i = nth - 14;
			l.j = 0;
		}
		
		break;
	case 3:
		l.i = 0;
		l.j = nth;
	}

	return l;
}

bool ggsOnline(int direc, int nth, const Loc& loc_) {
	Loc l = GgSliceP0(direc, nth);
	
	int di = loc_.i - l.i;
	int dj = loc_.j - l.j;
	
	switch (direc) {
	case 0: return(di + dj == 0);
	case 1: return(di == 0);
	case 2: return(di == dj);
	case 3: return(dj == 0);
	}
	
	error("ggsOnline");
	return false;
}

GgSlice::GgSlice(GomokuGameNode* gg, int direc, int nth) : vector<int>() {
	GgSlice_(gg, direc, nth);
}

GgSlice::GgSlice(GomokuGameNode* gg, const SliceCoord& sc) : vector<int>() {
	GgSlice_(gg, sc.getDirec(), sc.getNth());
}

void GgSlice::GgSlice_(GomokuGameNode* gg, int direc, int nth) {
	Loc l;
	
	l = GgSliceP0(direc, nth);
	
	while (l.isValid()) {
		this->push_back(gg->get(l));
		l += dirc[direc];
	}
}

bool operator<(const PttnKey& pk1, const PttnKey& pk2) {
	return((pk1.kindPttn << 8) + (pk1.side + 1) < (pk2.kindPttn << 8) + (pk2.side + 1));
}

bool PttnKey::isValid() const {
	return ((side == 1 || side == -1) && 0 <= kindPttn && kindPttn <= 7);
}
bool PttnLoc::isValid() const {
	if (direc < 0 || 4 <= direc || nth < 0 || begin < 0 || end <= begin) return false;
	return true;
}

int sliceFunc(const Loc& loc_, int direc, int nth) {
	switch (direc) {
	case 0: return loc_.i + loc_.j - nth;
	case 1: return loc_.i - nth;
	case 2: return loc_.i + 14 - loc_.j - nth;
	case 3: return loc_.j - nth;
	}
	return 0;
}

Loc sliceIntersect(int direc1, int nth1, int direc2, int nth2) {
	if (direc1 == direc2) error("sliceIntersect");

	if (direc1 > direc2) {
		int direcTmp = direc1;
		int nthTmp = nth1;
		direc1 = direc2;
		nth1 = nth2;
		direc2 = direcTmp;
		nth2 = nthTmp;
	}
	
	Loc r;
	
	switch (direc1) {
	case 0:
		switch (direc2) {
		case 1:
			r.i = nth2;
			r.j = nth1 - nth2;
			
			return r;
			
		case 2:
			if ((nth1 + nth2) % 2 == 0) {
				r.i = (nth1 + nth2 - 14) / 2;
				r.j = (nth1 - nth2 + 14) / 2;
				
				return r;
			}
			
			r.i = -1;
			r.j = -1;
			
			return r;
			
		case 3:
			r.i = nth1 - nth2;
			r.j = nth2;
			
			return r;
			
		default: error("sliceIntersect");
		}
		
	case 1:
		switch (direc2) {
		case 2:
			r.i = nth1;
			r.j = nth1 - nth2 + 14;
			
			return r;
			
		case 3:
			r.i = nth1;
			r.j = nth2;
			
			return r;
			
		default: error("sliceIntersect");
		}
		
	case 2:
		if (direc2 == 3) {
			r.i = nth1 + nth2 - 14;
			r.j = nth2;
			
			return r;
		}
	}

	error("sliceIntersect");
	return r;
}

bool indepPttns(const PttnLoc& pttnLoc1, const PttnLoc& pttnLoc2, GomokuGameNode* ggn) {
	if (pttnLoc1.direc == pttnLoc2.direc) {
		if (pttnLoc1.nth == pttnLoc2.nth) {
			const PttnLoc *pttnLocLeft;
			const PttnLoc *pttnLocRight;
			
			if (pttnLoc1.begin <= pttnLoc2.begin) {
				pttnLocLeft = &pttnLoc1;
				pttnLocRight = &pttnLoc2;
			}
			else {
				pttnLocLeft = &pttnLoc2;
				pttnLocRight = &pttnLoc1;
			}
			
			if (pttnLocLeft->end <= pttnLocRight->begin) return true;
			GgSlice slice(ggn, pttnLocLeft->direc, pttnLocLeft->nth);
			GgSlice::const_iterator iSliceBegin = slice.begin() + pttnLocRight->begin;
			GgSlice::const_iterator iSliceEnd = slice.begin() + pttnLocLeft->end;
			if (count(iSliceBegin, iSliceEnd, 0) == 0) return true;
			return false;
		}
		return true;
	}

	int vBegin, vEnd;
	Loc locBegin, locEnd;
	
	locBegin = GgSliceP0(pttnLoc1.direc, pttnLoc1.nth) +
	                  GgSlice::dirc[pttnLoc1.direc] * pttnLoc1.begin;
	locEnd = GgSliceP0(pttnLoc1.direc, pttnLoc1.nth) +
	                GgSlice::dirc[pttnLoc1.direc] * pttnLoc1.end;
	
	vBegin = sliceFunc(locBegin, pttnLoc2.direc, pttnLoc2.nth);
	vEnd = sliceFunc(locEnd, pttnLoc2.direc, pttnLoc2.nth);

	if (vBegin > 0) {
		if (vEnd < 0) {
			Loc is = sliceIntersect(pttnLoc1.direc, pttnLoc1.nth, pttnLoc2.direc, pttnLoc2.nth);
			if (is.isValid() && ggn->get(is) == 0) return false;	
			return true;
		}
		return true;
	}
	else if (vBegin < 0) {
		if (vEnd > 0) {
			Loc is = sliceIntersect(pttnLoc1.direc, pttnLoc1.nth, pttnLoc2.direc, pttnLoc2.nth);
			if (is.isValid() && ggn->get(is) == 0) return false;	
			return true;
		}
		return true;
	}

	if (ggn->get(vBegin) == 0) return false;
	return true;
}

MoveElem::MoveElem() : side(0) { }

MoveElem::MoveElem(int side_, const Loc& loc_) : side(side_), loc(loc_) { }

MoveElem::MoveElem(const MoveElem& me) : side(me.getSide()), loc(me.getLoc()) { }

int MoveElem::getSide() const {
	return side;
}

const Loc& MoveElem::getLoc() const {
	return loc;
}

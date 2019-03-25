#ifndef GOMOKUGAME_H_
#define GOMOKUGAME_H_

//#define DEBUG

#ifdef DEBUG
#include<iostream>
#endif

#include<utility>
#include<vector>
#include<deque>
#include<map>

using namespace std;

struct Loc {
	Loc(int i_ = 0, int j_ = 0) : i(i_), j(j_) { }
	Loc(const Loc& loc_) : i(loc_.i), j(loc_.j) { }
	virtual ~Loc() { }

	void set(int i_, int j_) { i = i_; j = j_; }
	bool isValid() const;

	const Loc& operator++();
	const Loc& operator=(const Loc& loc_);
	const Loc& operator+=(const Loc& loc_);
	const Loc& operator-=(const Loc& loc_);
	Loc operator+(const Loc& loc_);
	const Loc& operator*=(int m);
	Loc operator*(int m) const;

	int i, j;
};

bool operator<(const Loc& l, const Loc& m);
bool operator!=(const Loc& l, const Loc& m);

class GomokuGameBoard {
public:
	GomokuGameBoard();
	GomokuGameBoard(const GomokuGameBoard& ggb);
	virtual ~GomokuGameBoard() { }

	inline int get(const Loc& loc_) const {
		return boardData[loc_.i][loc_.j];
	}

	inline void put(const Loc& loc_, int v) {
		boardData[loc_.i][loc_.j] = (short int)v;
	}
	
	void clear();
	
private:
	short int boardData[15][15];
};

class SliceCoord {
public:
	SliceCoord();
	SliceCoord(int direc_, int nth_);
	SliceCoord(const SliceCoord& sc);
	virtual ~SliceCoord() { }
	
	int getDirec() const;
	int getNth() const;
	void set(int direc_, int nth_);
	
	virtual bool next();

private:
	int direc;
	int nth;
};

bool operator==(const SliceCoord& s, const SliceCoord& t);
bool operator<(const SliceCoord& s, const SliceCoord& t);

class SliceCoordConstrained : public SliceCoord {
public:
	SliceCoordConstrained(const Loc& changedLoc);
	virtual ~SliceCoordConstrained() { }
	
	bool next();
	
private:
	vector<SliceCoord> SCChain;
	size_t iSCChain;
};

typedef pair<int, int> int_pair;
Loc GgSliceP0(int direc, int nth);
bool ggsOnline(int direc, int nth, const Loc& loc_);

class GomokuGameNode;

class GgSlice : public vector<int> {
public:
	GgSlice(GomokuGameNode* gg, int direc, int nth);
	GgSlice(GomokuGameNode* gg, const SliceCoord& sc);
	virtual ~GgSlice() { }

	static const Loc dirc[];
	static const int numSlices[];
	static const int_pair effectiveRange[];
	
private:
	void GgSlice_(GomokuGameNode* gg, int direc, int nth);
};

typedef GgSlice::const_iterator ggs_iter;
typedef pair<ggs_iter, ggs_iter> ggs_iter_pair;
const ggs_iter_pair ggs_iter_pair_null = make_pair((ggs_iter)0, (ggs_iter)0);

struct Pttn {
	Pttn() : side(0), kindPttn(0), begin(0), end(0) { }
	Pttn(int side_, int kindPttn_, int begin_, int end_)
	: side(side_), kindPttn(kindPttn_), begin(begin_), end(end_) { }

	Pttn(const Pttn& pe)
	: side(pe.side), kindPttn(pe.kindPttn), begin(pe.begin), end(pe.end) { }

	Pttn(int side, int pass, ggs_iter first, ggs_iter last);
	
	bool found() const;
	
	int side;
	int kindPttn;
	int begin;
	int end;
	
	static const int numPasses = 7;
	
	static const int BLANK = 0;
	static const int GO = 1;
	static const int TATSUSHI = 2;
	static const int SHI = 3;
	static const int SAN = 4;
	static const int KOSAN = 5;
	static const int NI = 6;
	static const int ICHI = 7;
};

struct PttnKey {
	bool isValid() const;
	
	int side;
	int kindPttn;
};

bool operator<(const PttnKey& pk1, const PttnKey& pk2);

struct PttnLoc {
	bool isValid() const;
	
	int direc;
	int nth;
	int begin;
	int end;
};

typedef multimap<PttnKey, PttnLoc> PttnDb;

bool indepPttns(const PttnLoc& pttnLoc1, const PttnLoc& pttnLoc2, GomokuGameNode* ggn);

class MoveElem {
public:
	MoveElem();
	MoveElem(int side_, const Loc& loc_);
	MoveElem(const MoveElem& me);
	virtual ~MoveElem() { }
	
	int getSide() const;
	const Loc& getLoc() const;
	
private:
	int side;
	Loc loc;
};

#endif /*GOMOKUGAME_H_*/

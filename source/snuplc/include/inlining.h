#ifndef __SnuPL_INLINING_H__
#define __SnuPL_INLINING_H__

#include <iostream>
#include <vector>

#include "ir.h"
#include "fnode.h"

#define INLINING_CALL 1
#define INLINING_CALL_IN_LOOP 5
#define INLINING_CALL_IN_SUB 3

#define INLINING_SCORE_LEN(X) 1/(float)X

using namespace std;

class Inlining {
	public:
		Inlining(void);
		~Inlining(void);
		void parseTAC(CModule *module);
		void calculateScore();
		void calculate(fNode* node, int base_score);
		void doOneStep();
		fNode* FindSymbol(const CSymbol* symbol);
		virtual ostream&  print(ostream &out, int indent=0) const;

	protected:
		CModule* _module;
		vector<fNode*> _nodes;

};
ostream& operator<<(ostream &out, const Inlining &t);

ostream& operator<<(ostream &out, const Inlining *t);

#endif

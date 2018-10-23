#ifndef __SnuPL_INLINING_H__
#define __SnuPL_INLINING_H__

#include <iostream>
#include <vector>

#include "ir.h"
#include "fnode.h"

using namespace std;

class Inlining {
	public:
		Inlining(void);
		~Inlining(void);
		void parseTAC(CModule *module);
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

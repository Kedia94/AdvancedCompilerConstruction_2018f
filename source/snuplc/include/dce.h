#ifndef __SnuPL_DeadCodeE_H__
#define __SnuPL_DeadCodeE_H__

#include <iostream>
#include <vector>
#include "ir.h"

using namespace std;

/* Dead Code Elimination */
class DeadCodeE {
	public:
		DeadCodeE(void);
		~DeadCodeE(void);
        DeadCodeE(CModule *module);

        void eliminateDeadCode(void);
		virtual ostream&  print(ostream &out, int indent=0) const;

	protected:
    	void eliminateDeadCode(CCodeBlock* codeblock);
		CModule* _module;

};
ostream& operator<<(ostream &out, const DeadCodeE &t);

ostream& operator<<(ostream &out, const DeadCodeE *t);

#endif

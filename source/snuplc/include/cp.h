#ifndef __SnuPL_ConstantP_H__
#define __SnuPL_ConstantP_H__

#include <iostream>
#include <vector>
#include "ir.h"

using namespace std;

class ConstantP {
	public:
		ConstantP(void);
		~ConstantP(void);
		ConstantP(CModule *module);
        void doConstantPropagation();
		virtual ostream&  print(ostream &out, int indent=0) const;

	protected:
		CModule* _module;
        void doConstantPropagation(CCodeBlock* codeblock);

};
ostream& operator<<(ostream &out, const ConstantP &t);

ostream& operator<<(ostream &out, const ConstantP *t);

#endif

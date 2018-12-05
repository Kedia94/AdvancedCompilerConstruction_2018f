#ifndef __SnuPL_BasicBlock_H__
#define __SnuPL_BasicBlock_H__

#include <iostream>
#include <vector>
#include "ir.h"

using namespace std;

typedef struct VariableDef_ {
    int defNum;
    string variableName;
    CTacInstr* instr;
    bool isConst;
    int value;
//    BasicBlock* inBlock;
} VariableDef;

class BasicBlock {
	public:
		BasicBlock(CTacInstr* header, int header_id);
		~BasicBlock(void);
        
        void addInstr(CTacInstr* instr);
        const list<CTacInstr*>& GetInstr(void) const;
        CTacInstr* GetHeader(void);
        void addPred(BasicBlock* predecessor1);
        const list<BasicBlock*>& getPred(void) const;

        void addLiveConstDef(string str, int value);
        void unionLiveConstDef(void);
        //const map<string, int> getLiveConstDef(void) const;
        list<VariableDef *> listDef;
        //map<string, int> _liveConstDef;
        unsigned int IN;
        unsigned int OUT;
        unsigned int gen;
        unsigned int kill;

	protected:
		CTacInstr* _header;
        int _header_id; // (*it)->GetId()
        list<CTacInstr*> Instr_list;
        list<BasicBlock*> _predecessor;

};

#endif

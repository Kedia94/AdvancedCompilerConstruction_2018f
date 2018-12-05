#include "basicblock.h"
#include "ir.h"

//------------------------------------------------------------------------------
// fNode
//

BasicBlock::BasicBlock(CTacInstr* header, int header_id)
{
	_header = header;
	_header_id = header_id;
}

BasicBlock::~BasicBlock(void)
{
}


void BasicBlock::addInstr(CTacInstr* instr)
{
    Instr_list.push_back(instr);
}

void BasicBlock::addPred(BasicBlock* predecessor)
{
    _predecessor.push_back(predecessor);
}

const list<BasicBlock*>& BasicBlock::getPred(void) const
{
    return _predecessor;
}

const list<CTacInstr*>& BasicBlock::GetInstr(void) const
{
  return Instr_list;
}

CTacInstr* BasicBlock::GetHeader(void)
{
    return _header;
}
/*
void BasicBlock::addLiveConstDef(string str, int value){
    _liveConstDef.insert (pair<string,int>(str,value) );
}
void BasicBlock::unionLiveConstDef(){
    
    for (list<BasicBlock*>::const_iterator it_bb = getPred().begin(); it_bb != getPred().end(); it_bb++) 
    {
        map<string, int> liveConstDef = (*it_bb)->_liveConstDef;
        for (map<string,int>::iterator it=liveConstDef.begin(); it!=liveConstDef.end(); ++it)
        {
            _liveConstDef.insert(pair<string,int>(it->first,it->second));    
        }
    }
}

const map<string, int> BasicBlock::getLiveConstDef(void) const{
    return _liveConstDef;
}
*/
 
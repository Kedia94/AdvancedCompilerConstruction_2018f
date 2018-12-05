/* Dead Code Elimination */

#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include "dce.h"
#include "basicblock.h"

DeadCodeE::DeadCodeE(void)
{
}

DeadCodeE::~DeadCodeE(void)
{
}

DeadCodeE::DeadCodeE(CModule *module)
{
    _module = module;
}


void DeadCodeE::eliminateDeadCode(void)
{
    //main module
    eliminateDeadCode(_module->GetCodeBlock());

    //procedures
    vector<CScope*> child = _module->GetSubscopes();
	for (int i=0; i<child.size(); i++)
	{
		eliminateDeadCode(child[i]->GetCodeBlock());
	}
}

void DeadCodeE::eliminateDeadCode(CCodeBlock* codeblock)
{
    list<BasicBlock*> BBlist;

    bool change = true;

    while(change)
    {
        codeblock->CleanupControlFlow();
        change= false;
        BBlist.clear();

        list<CTacInstr*> ops = codeblock->GetInstr();

        BasicBlock* bb = NULL;
        BasicBlock* prev_bb = NULL;
        CTacInstr* prev_inst = NULL;

        for (list<CTacInstr*>::const_iterator it = ops.begin(); it != ops.end(); it++) {
            
            if( it == ops.begin() || (*it)->GetOperation() == opLabel)
            {
                prev_bb = bb;
                bb = new BasicBlock(*it,  (*it)->GetId());
                bb->IN=0;
                bb->OUT=0;
                bb->gen=0;
                bb->kill=0;
                
                if(prev_inst != NULL && !(prev_inst->IsBranch()) )
                {
                    bb->addPred(prev_bb);
                }
                bb->addInstr(*it);
                prev_inst = (*it);
                BBlist.push_back(bb);
            }
            else{
                bb->addInstr(*it);
                prev_inst = (*it);
            }
        }

        for (list<BasicBlock*>::const_iterator it_bb = BBlist.begin(); it_bb != BBlist.end(); it_bb++) {
            list<CTacInstr*> bb_ops = (*it_bb)->GetInstr();
            for (list<CTacInstr*>::const_iterator it = bb_ops.begin(); it != bb_ops.end(); it++) {
                if((*it)->IsBranch()){
                    CTac* dst= (*it)->GetDest();
                    if (CTacLabel* dstName = dynamic_cast<CTacLabel*>(dst))
                    {
                        for (list<BasicBlock*>::const_iterator the_bb = BBlist.begin(); the_bb != BBlist.end(); the_bb++) {
                            if(CTacLabel* labelName = dynamic_cast<CTacLabel*>((*the_bb)->GetHeader()))
                            {
                                if( labelName->GetLabel() == dstName->GetLabel() )
                                {
                                    (*the_bb)->addPred(*it_bb);
                                    break;
                                }
                            }
                        }    
                    }
                }
                 
            }
        }

        for (list<BasicBlock*>::iterator it_bb = BBlist.begin(); it_bb != BBlist.end(); it_bb++) {
            list<CTacInstr*> bb_ops = (*it_bb)->GetInstr();
            bool remove = false;
            for (list<CTacInstr*>::iterator it = bb_ops.begin(); it != bb_ops.end(); it++) {
                if(remove){
                    cout << "[DCE]Remove  :" << (*it) << endl;
                    codeblock->DelInstr((*it)->GetId());
                    change= true;
                    break;
                }
                if( (*it)->GetOperation() == opGoto){
                    remove = true;
                }

            }            
        }
    }
}

ostream& DeadCodeE::print(ostream &out, int indent) const
{

}


ostream& operator<<(ostream &out, const DeadCodeE &t)
{
  return t.print(out);
}

ostream& operator<<(ostream &out, const DeadCodeE *t)
{
  return t->print(out);
}



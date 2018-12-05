#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include "cp.h"
#include "basicblock.h"

ConstantP::ConstantP(void)
{

}

ConstantP::~ConstantP(void)
{

}

ConstantP::ConstantP(CModule *module)
{
	_module = module;
}
void ConstantP::doConstantPropagation(){
    //main module
    doConstantPropagation(_module->GetCodeBlock());

    //procedures
    vector<CScope*> child = _module->GetSubscopes();
	for (int i=0; i<child.size(); i++)
	{
		doConstantPropagation(child[i]->GetCodeBlock());
	}
}
void ConstantP::doConstantPropagation(CCodeBlock* codeblock)
{
    list<BasicBlock*> BBlist;
    bool change = true;

    cout << "\n[CODE BLOCK] : " << codeblock->GetName() << endl;
    while(change)
    {
        codeblock->CleanupControlFlow();
        change= false;
        BBlist.clear();

        list<CTacInstr*> ops = codeblock->GetInstr();

        BasicBlock* bb = NULL;
        BasicBlock* prev_bb = NULL;
        CTacInstr* prev_inst = NULL;

        //cout << "[Make Blocks]" <<endl;
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

        //cout << "[Make pred...]" <<endl;
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
                                    //cout << (*it_bb)->GetHeader() << " -> " << (*the_bb)->GetHeader() << endl;
                                    (*the_bb)->addPred(*it_bb);
                                    break;
                                }
                            }
                        }    
                    }
                }
                 
            }
        }

        // Make VariableDefinition
        int defcount = 0;
        list<VariableDef *> listDef_tot;
        for (list<BasicBlock*>::const_iterator it_bb = BBlist.begin(); it_bb != BBlist.end(); it_bb++) {

            list<CTacInstr*> bb_ops = (*it_bb)->GetInstr();
            for (list<CTacInstr*>::const_iterator it = bb_ops.begin(); it != bb_ops.end(); it++) {
                if ((*it)->GetOperation() == opAssign){
                    
                    CTac* dst= (*it)->GetDest();

                    if (CTacName* dstName = dynamic_cast<CTacName*>(dst))
                    {
                        //CTacAddr* src= (*it)->GetSrc(1);
                        //if (CTacConst* ctConst = dynamic_cast <CTacConst*>(src))
                        {
                            VariableDef* cd = new VariableDef();
                            cd->defNum= defcount;
                            cd->instr= (*it);
                            cd->variableName=  dstName->GetSymbol()->GetName();

                            if (CTacConst* ctConst = dynamic_cast <CTacConst*>((*it)->GetSrc(1))){
                                cd->isConst = true;
                                cd->value = ctConst->GetValue();
                            }
                            (*it_bb)->listDef.push_back(cd);
                            listDef_tot.push_back(cd);

                            defcount++;
                        }
                    }            
                }
            }
        }
        //cout << "Definition num = "<<defcount << endl;

        // Make gen/kill
        for (list<BasicBlock*>::const_iterator it_bb = BBlist.begin(); it_bb != BBlist.end(); it_bb++) {

            for (list<VariableDef*>::const_iterator it_def = (*it_bb)->listDef.begin(); it_def != (*it_bb)->listDef.end(); it_def++) {
                (*it_bb)->gen |= 1 << (*it_def)->defNum;

                for(list<BasicBlock*>::const_iterator the_bb = BBlist.begin(); the_bb != BBlist.end(); the_bb++){
                    for (list<VariableDef*>::const_iterator the_def = (*the_bb)->listDef.begin(); the_def != (*the_bb)->listDef.end(); the_def++) {
                        if ((*the_bb) != (*it_bb) && (*it_def)->variableName == (*the_def)->variableName) {
                            (*it_bb)->kill |= 1 << (*the_def)->defNum;      
                        }
                    }
                }
            }
            //printf("gen : %x\n",  (*it_bb)->gen);
            //printf("kill : %x\n",  (*it_bb)->kill);
        }


        // Make reachingDefinition

        for (list<BasicBlock*>::const_iterator it_bb = BBlist.begin(); it_bb != BBlist.end(); it_bb++)
            (*it_bb)->OUT = 0;
        bool RDchange= true;
        while(RDchange){
            RDchange= false;
            int Blocknum=0;
            for (list<BasicBlock*>::const_iterator it_bb = BBlist.begin(); it_bb != BBlist.end(); it_bb++) {
                Blocknum++;
                for( list<BasicBlock*>::const_iterator pred_bb = (*it_bb)->getPred().begin(); pred_bb != (*it_bb)->getPred().end(); pred_bb++)
                {
                    (*it_bb)->IN |= (*pred_bb)->OUT;
                }
                unsigned int temp = (*it_bb)->gen | ((*it_bb)->IN & (~(*it_bb)->kill) );
                if( temp != (*it_bb)->OUT){
                    RDchange= true;
                    (*it_bb)->OUT = temp;
                }
                //printf("Block(%d) : IN= %x\n",Blocknum, (*it_bb)->IN);
                //printf("Block(%d) : OUT= %x\n",Blocknum, (*it_bb)->OUT);
            }
        }

        map<string, int> liveDefinition;
        list<string> removeList;

        for (list<BasicBlock*>::const_iterator it_bb = BBlist.begin(); it_bb != BBlist.end(); it_bb++){
            liveDefinition.clear();
            unsigned int reachingdefinition = (*it_bb)->OUT & (*it_bb)->IN;

            //printf("reachingdefinition = %x\n", reachingdefinition);
            for(int i=0; i< defcount ; i++){
                list<VariableDef *>::iterator it = listDef_tot.begin();
                advance(it, i);
                if(reachingdefinition & (1<<(*it)->defNum))
                {
                    if((*it)->isConst)
                    {
                        string variable = (*it)->variableName;
                        if( liveDefinition.find(variable) != liveDefinition.end() )
                        {
                            liveDefinition.erase(variable);
                            //cout << "def erase num= " << i << " inst : " <<  (*it)->instr <<endl;
                        }
                        else{
                            liveDefinition[variable] = (*it)->value;
                            //cout << "def num= " << i << " inst : " <<  (*it)->instr <<endl;
                        }
                    }
                    else{
                        string variable = (*it)->variableName;
                        removeList.push_back(variable);
                    }
                }
            }
            for (list<string>::const_iterator str = removeList.begin(); str != removeList.end(); str++){
                if(liveDefinition.find(*str) != liveDefinition.end())
                {
                    liveDefinition.erase(*str);
                    //cout << "def erase " << *str << endl;
                }
            }

            //전체 def list 중에서 it_bb의 reachingdefinition 에서 live 하면서, const인 것들
            //cout << "Header : " << (*it_bb)->GetHeader() << " num of pred : "<< (*it_bb)->getPred().size() << endl;

            list<CTacInstr*> bb_ops = (*it_bb)->GetInstr();
            for (list<CTacInstr*>::const_iterator it = bb_ops.begin(); it != bb_ops.end(); it++) {         
                CTacAddr* src1= (*it)->GetSrc(1);
                if (CTacName* ctName = dynamic_cast <CTacName*>(src1)){
                    string variable = ctName->GetSymbol()->GetName();
                    if( liveDefinition.find(variable) != liveDefinition.end()){
                        //replace it
                        cout<< "[CF] Replace: variable =  " << variable <<" constant = " <<liveDefinition[variable] <<endl;
                        CTacInstr* newInstr = new CTacInstr((*it)->GetOperation(),(*it)->GetDest(), new CTacConst(liveDefinition[variable]), (*it)->GetSrc(2));
                        //cout << newInstr << endl;
                        codeblock->RepInstr((*it)->GetId(), newInstr);
                        
                        change= true;
                        continue;              
                    }
                }           
                CTacAddr* src2= (*it)->GetSrc(2);
                if (CTacName* ctName = dynamic_cast <CTacName*>(src2)){
                    string variable = ctName->GetSymbol()->GetName();
                    if( liveDefinition.find(variable) != liveDefinition.end()){
                        //replace it
                        cout<< "[CF] Replace: variable =  " << variable <<" constant = " <<liveDefinition[variable] <<endl;
                        CTacInstr* newInstr = new CTacInstr((*it)->GetOperation(),(*it)->GetDest(), (*it)->GetSrc(1), new CTacConst(liveDefinition[variable]));
                        //cout << newInstr << endl;
                        codeblock->RepInstr((*it)->GetId(), newInstr);

                        change= true;
                        continue;                            
                    }
                }
                
                //processing operations if we replace a variable with the constant above.

                enum EOperation op = (*it)->GetOperation();
                if( op >= opAdd && op <= opOr ){ //IsRelOp
                    CTacAddr* src1= (*it)->GetSrc(1);
                    CTacAddr* src2= (*it)->GetSrc(2);
                    if (CTacConst* ctConst1 = dynamic_cast <CTacConst*>(src1)){
                        if (CTacConst* ctConst2 = dynamic_cast <CTacConst*>(src1)){
                            int opResult;
                            switch(op){
                                case opAdd:
                                    opResult = ctConst1->GetValue() + ctConst2->GetValue();
                                    break;
                                case opSub:
                                    opResult = ctConst1->GetValue() - ctConst2->GetValue();
                                    break;                                 
                                case opMul:
                                    opResult = ctConst1->GetValue() * ctConst2->GetValue();
                                    break;                                 
                                case opDiv:
                                    opResult = ctConst1->GetValue() / ctConst2->GetValue();
                                    break;                                 
                                case opAnd:
                                    opResult = ctConst1->GetValue() & ctConst2->GetValue();
                                    break;                                 
                                case opOr:
                                    opResult = ctConst1->GetValue() | ctConst2->GetValue();
                                    break;                                 
                            }
                            cout << "[CF] Folding: "<< (*it) << endl;
                            CTacInstr* newInstr = new CTacInstr(opAssign ,(*it)->GetDest(), new CTacConst(opResult));
                            codeblock->RepInstr((*it)->GetId(), newInstr);                           
                            change= true;  
                        }
                    }
                }          
            }
        }
        // remove definition unused
        if(change == false){
            int usedDef= 0;
            for (list<CTacInstr*>::const_iterator it = ops.begin(); it != ops.end(); it++) {
                for(int i=0; i< defcount ; i++){
                    list<VariableDef *>::iterator def_it = listDef_tot.begin();
                    advance(def_it, i);

                    if ((*def_it)->instr == (*it))
                        continue;

                    CTacAddr* src1= (*it)->GetSrc(1);
                    if (CTacName* ctName = dynamic_cast <CTacName*>(src1)){
                        string variable = ctName->GetSymbol()->GetName();
                        if( variable == (*def_it)->variableName )
                            usedDef |=  (1<<(*def_it)->defNum);
                    }           
                    CTacAddr* src2= (*it)->GetSrc(2);
                    if (CTacName* ctName = dynamic_cast <CTacName*>(src2)){
                        string variable = ctName->GetSymbol()->GetName();
                        if( variable == (*def_it)->variableName )
                            usedDef |=  (1<<(*def_it)->defNum);

                    }
                }
            }

            for(int i=defcount-1; i >=0 ; i--){
                if ( (1<<i & usedDef) == 0)
                {
                    list<VariableDef *>::iterator def_it = listDef_tot.begin();
                    advance(def_it, i);

                    cout << "[CF] Remove : " << (*def_it)->instr << endl;
                    codeblock->DelInstr((*def_it)->instr->GetId());
                    change = true;
                }
            }
        }

        // remove decided if_condition
        if(change == false){    
            for (list<CTacInstr*>::const_iterator it = ops.begin(); it != ops.end(); it++) {

                if( IsRelOp( (*it)->GetOperation() ) ){
                    CTacAddr* src1= (*it)->GetSrc(1);
                    CTacAddr* src2= (*it)->GetSrc(2);
                    if (CTacConst* ctConst1 = dynamic_cast <CTacConst*>(src1)){
                        if (CTacConst* ctConst2 = dynamic_cast <CTacConst*>(src2)){
                            bool opResult;
                            switch((*it)->GetOperation()){
                                case opEqual:
                                    opResult = ctConst1->GetValue() == ctConst2->GetValue() ? true : false;
                                    break;
                                case opNotEqual:
                                    opResult = ctConst1->GetValue() != ctConst2->GetValue() ? true : false;
                                    break;                                 
                                case opLessThan:
                                    opResult = ctConst1->GetValue() < ctConst2->GetValue() ? true : false;
                                    break;                                 
                                case opLessEqual:
                                    opResult = ctConst1->GetValue() <= ctConst2->GetValue() ? true : false;
                                    break;                                 
                                case opBiggerThan:
                                    opResult = ctConst1->GetValue() > ctConst2->GetValue() ? true : false;
                                    break;                                 
                                case opBiggerEqual:
                                    opResult = ctConst1->GetValue() >= ctConst2->GetValue() ? true : false;
                                    break;                                 
                            }

                            if(opResult){
                                CTacInstr* newInstr = new CTacInstr(opGoto ,(*it)->GetDest());
                                codeblock->RepInstr((*it)->GetId(), newInstr);
                                cout << "[CF]Replace RelOp: "<< newInstr << endl;
                                
                                it++;
                                cout << "[CF]Remove RelOp: "<< (*it) << endl;
                                codeblock->DelInstr((*it)->GetId());
                                
                            }else{
                                cout << "[CF]Remove RelOp: "<< (*it) << endl;
                                codeblock->DelInstr((*it)->GetId());
                            }
                          
                            change= true;
                            break;
                        }
                    }

                }
                 
            }
        }

    }
}

ostream& ConstantP::print(ostream &out, int indent) const
{

}


ostream& operator<<(ostream &out, const ConstantP &t)
{
  return t.print(out);
}

ostream& operator<<(ostream &out, const ConstantP *t)
{
  return t->print(out);
}



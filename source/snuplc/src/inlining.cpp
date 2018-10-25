#include <cassert>
#include <cmath>
#include <map>

#include "inlining.h"

Inlining::Inlining(void)
{
	_module = NULL;
	_param_iter = 0;
	_original_length = -1;
}

Inlining::~Inlining(void)
{
	for (int i=0; i<_nodes.size(); i++)
	{
		delete _nodes[i];
	}
	_nodes.clear();
}

void Inlining::parseTAC(CModule *module)
{
	_module = module;
	
	int cb_len = _module->GetCodeBlock()->GetInstr().size();

	fNode* tNode = new fNode(cb_len, _module->GetName(), module);
	_nodes.push_back(tNode);
	
	vector<CScope*> child = _module->GetSubscopes();
	for (int i=0; i<child.size(); i++)
	{
		cb_len = child[i]->GetCodeBlock()->GetInstr().size();
		tNode = new fNode(cb_len, child[i]->GetName(), child[i]);
		_nodes.push_back(tNode);
	}

	for (int i=0; i<_nodes.size(); i++)
	{
		tNode = _nodes[i];
		CCodeBlock* tcb = tNode->GetModule()->GetCodeBlock();
		list<CTacInstr*> ops = tcb->GetInstr();

		int loop_level = 0;
		vector<int> while_label;
		while_label.clear();

		for (list<CTacInstr*>::const_iterator it = ops.begin(); it != ops.end(); it++) {
			if ((*it)->GetOperation() == opCall)
			{
				CTac* Fun = NULL;
				if ((*it)->GetSrc(1) != NULL)
					Fun = (*it)->GetSrc(1);
				else
					Fun = (*it)->GetDest();

				const CSymbol* tsym = NULL;
				if (CTacName* t = dynamic_cast<CTacName*>(Fun))
					tsym = t->GetSymbol();

				if (tsym)
				{
					fNode* tempNode = FindSymbol(tsym);
					if (tempNode)
					{
						fCallNode* tLoop = new fCallNode(tempNode, loop_level);
						_nodes[i]->AddChild(tLoop);
					}
				}
			}
			else if ((*it)->GetOperation() == opLabel)
			{
				CTacLabel* lab = dynamic_cast<CTacLabel*>(*it);
				assert(lab);
				string label = lab->GetLabel();

				size_t found = label.find('_');
				if (found != string::npos)
				{
					/* while of if start */
					int num = stoi(label.substr(0, found));
					if (label.substr(found+1).compare("while_cond") == 0)
					{
						loop_level++;
						while_label.push_back(num);
					}
					else if (label.substr(found+1).compare("while_body") == 0)
					{
					}
					else if (label.substr(found+1).compare("if_true") == 0)
					{
					}
					else if (label.substr(found+1).compare("if_false") == 0)
					{
					}
				}
				else
				{
					/* while of if end */
					int num = stoi(label);
					for (int i=0; i<while_label.size(); i++)
					{
						if (while_label[i] == num + 1)
						{
							loop_level--;
							while_label.erase(while_label.begin() + i);
							break;
						}
					}
				}
			}
		}
	}
	_original_length = GetCBSize();
}

void Inlining::calculateScore()
{
	assert(_nodes.size() > 0);

	calculate(_nodes[0], INLINING_CALL);

	for (int i=1; i<_nodes.size(); i++)
		calculate(_nodes[i], INLINING_CALL_IN_SUB);
}

void Inlining::calculate(fNode* node, int base_score)
{
	int child_len = node->GetChildLen();

	for (int i=0; i<child_len; i++)
	{
		fCallNode* tNode = node->GetChild(i);

		float score = INLINING_SCORE_LEN((tNode->GetNode()->GetCodeLen()+1));
		score *= base_score;
		score *= pow((float)INLINING_CALL_IN_LOOP, (float)tNode->GetLoopLevel());
		tNode->SetScore(score);
	}
}

void Inlining::doOneStep()
{
	int Peek = PeekCBSize();
	cout<<"original: "<<_original_length<<" / predicted: "<<Peek<<endl;
	if (_original_length == Peek)
		return;
	if (INLINING_LIMIT(_original_length) < Peek)
	{
		cout<<"Size limit: Do not inlining" << endl;
		return;
	}
	float max_score = -1;
	int max_index = -1;
	int max_child_index = -1;

	for (int i=0; i<_nodes.size(); i++)
	{
		fNode* tNode = _nodes[i];
		for (int j=0; j<tNode->GetChildLen(); j++)
		{
			if (tNode->GetChild(j)->GetScore() > max_score)
			{
				max_score = tNode->GetChild(j)->GetScore();
				max_index = i;
				max_child_index = j;
			}

		}
	}

	// Inline (max_index, max_child_index)
	CScope* parent = _nodes[max_index]->GetModule();
	CScope* target = _nodes[max_index]->GetChild(max_child_index)->GetNode()->GetModule();

	CCodeBlock* tcb = parent->GetCodeBlock();
	list<CTacInstr*> ops = tcb->GetInstr();
	CCodeBlock* ttcb = target->GetCodeBlock();
	list<CTacInstr*> tops = ttcb->GetInstr();

	int loop_level = 0;
	vector<int> while_label;
	while_label.clear();

	list<CTacInstr*>::const_iterator it;
	const CSymbol* tsym;
	int iter_j = 0;
	for (it = ops.begin(); it != ops.end(); it++) {
		if ((*it)->GetOperation() == opCall)
		{
			CTac* Fun = NULL;
			if ((*it)->GetSrc(1) != NULL)
				Fun = (*it)->GetSrc(1);
			else
				Fun = (*it)->GetDest();

			tsym = NULL;
			if (CTacName* t = dynamic_cast<CTacName*>(Fun))
				tsym = t->GetSymbol();

			if (tsym)
			{
				fNode* tempNode = FindSymbol(tsym);
				if (tempNode)
				{
					if (max_child_index == iter_j)
					{
						break;
					}
					iter_j++;
				}
			}
		}
	}
	// it : call of what we have to inline
	const CSymProc* proc = dynamic_cast<const CSymProc*>(tsym);
	cout<<"parent: "<<parent->GetName()<<endl;
	cout<<"target: "<<proc<<" "<<target->GetName()<<endl;
	
	assert(proc);

	vector<string> parm_vector;

	// Check Function Arguments
	const CSymParam* parm;
	int paramnum = proc->GetNParams();
	for (int i=0; i<paramnum; i++){
		parm = proc->GetParam(i);
		parm_vector.push_back(parm->GetName());
	}

	map<string, CSymLocal*> inline_vars;
	
	// Check Local Variables of target and put it on Symtab of parent
	CSymtab* symtab = target->GetSymbolTable();
	CSymtab* par_symtab = parent->GetSymbolTable();
	vector<CSymbol*> syms = symtab->GetSymbols();
	for (int i=0; i<syms.size(); i++)
	{
		string tName = "i" + to_string(_param_iter) + "_" + syms[i]->GetName();
		CSymLocal* tempLocal = new CSymLocal(tName, syms[i]->GetDataType());
		inline_vars[syms[i]->GetName()] = tempLocal;
	}

	for (map<string, CSymLocal*>::iterator tt=inline_vars.begin(); tt != inline_vars.end(); tt++)
	{
		par_symtab->AddSymbol(tt->second);
		cout<<tt->first<<" "<<tt->second->GetName()<<" " <<tt->second->GetDataType()<<endl;
	}

	//it : call location of target calling in parent
	
	list<CTacInstr*>::const_iterator parm_it = it;
	cout<<tcb<<endl;

	// Copy function body of target of inlining
	vector<CTacInstr*> new_instr_vector;

	for (list<CTacInstr*>::const_iterator tparm_it = tops.begin(); tparm_it != tops.end();tparm_it++)
	{
		CTacInstr* tinstr = *tparm_it;
		EOperation top = tinstr->GetOperation();
		CTac* tdst = tinstr->GetDest();
		if (tdst)
		{
			if (CTacName* ctname = dynamic_cast<CTacName*>(tdst))
			{
				if (inline_vars.find(ctname->GetSymbol()->GetName()) != inline_vars.end())
				{
					tdst = new CTacName(inline_vars[ctname->GetSymbol()->GetName()]);
				}
			}
		}

		CTacAddr* tsrc1 = tinstr->GetSrc(1);
		if (tsrc1)
		{
			if (CTacName* ctname = dynamic_cast<CTacName*>(tsrc1))
			{
				if (inline_vars.find(ctname->GetSymbol()->GetName()) != inline_vars.end())
				{
					tsrc1 = new CTacName(inline_vars[ctname->GetSymbol()->GetName()]);
				}
			}
		}

		CTacAddr* tsrc2 = tinstr->GetSrc(2);
		if (tsrc2)
		{
			if (CTacName* ctname = dynamic_cast<CTacName*>(tsrc2))
			{
				if (inline_vars.find(ctname->GetSymbol()->GetName()) != inline_vars.end())
				{
					tsrc2 = new CTacName(inline_vars[ctname->GetSymbol()->GetName()]);
				}
			}
		}
		CTacInstr* newInstr = new CTacInstr(top, tdst, tsrc1, tsrc2);
		new_instr_vector.push_back(newInstr);
	}

	// Replace 
	// Change all 'param a <- b' to 'assign t* <- b'
	for (int i=0; i<paramnum; i++)
	{
		while ((*parm_it)->GetOperation() != opParam)
		{
			parm_it--;
		}

		list<CTacInstr*>::const_iterator t_parm = parm_it--;
		CTacInstr* newInstr = new CTacInstr(opAssign, new CTacName(inline_vars[parm_vector[i]]), (*t_parm)->GetSrc(1));
		tcb->RepInstr((*t_parm)->GetId(), newInstr);
	}

	for (int i=0; i<new_instr_vector.size()-1; i++)
	{
		tcb->InsInstr((*it)->GetId(), new_instr_vector[i]);
	}

	// Remove return and call(*it) to assign
	if ((*it)->GetDest())
	{
		CTacInstr* newInstr = new CTacInstr(opAssign, (*it)->GetDest(), new_instr_vector[new_instr_vector.size()-1]->GetSrc(1));
		cout<<"======================="<<endl;
		cout<<*it<<endl;
		tcb->RepInstr((*it)->GetId(), newInstr);
	}
	else
	{
		tcb->DelInstr((*it)->GetId());
	}
	delete new_instr_vector[new_instr_vector.size()-1];
	cout<<tcb<<endl;




	inline_vars.clear();

	// Remove child
	// double free or corruption (out) XXX
//	_nodes[max_index]->RemoveChild(max_child_index);


	// recalculate parameters after inlining
	// LEN update
	for (int i=0; i<_nodes.size(); i++)
	{
		_nodes[i]->SetCodeLen(_nodes[i]->GetModule()->GetCodeBlock()->GetInstr().size());
	}

	// TODO: Child update with loop level
	calculateScore();


	_param_iter++;
}

int Inlining::GetCBSize()
{
	int size = 0;
	for (int i=0; i<_nodes.size(); i++)
		size += _nodes[i]->GetCodeLen();

	return size;
}

int Inlining::PeekCBSize()
{
	int size = GetCBSize();

	float max_score = -1;
	int max_index = -1;
	int max_child_index = -1;

	for (int i=0; i<_nodes.size(); i++)
	{
		fNode* tNode = _nodes[i];
		for (int j=0; j<tNode->GetChildLen(); j++)
		{
			if (tNode->GetChild(j)->GetScore() > max_score)
			{
				max_score = tNode->GetChild(j)->GetScore();
				max_index = i;
				max_child_index = j;
			}

		}
	}

	if (max_index != -1)
		size += _nodes[max_index]->GetChild(max_child_index)->GetNode()->GetCodeLen();

	return size;
}

fNode* Inlining::FindSymbol(const CSymbol* symbol)
{
	if (!symbol)
		return NULL;

	string name = symbol->GetName();

	for (int i=0; i<_nodes.size(); i++)
	{
		string tName = _nodes[i]->GetName();
		if (name.compare(tName) == 0)
		{
			return _nodes[i];
		}
	}
	return NULL;
}

ostream& Inlining::print(ostream &out, int indent) const
{
	string ind(indent, ' ');
	out << ind << "[[ " << endl;

	for (int i=0; i<_nodes.size(); i++)
	{
		_nodes[i]->print(out, indent+2);
	}
	out << ind << "]]" << endl;

	return out;
}

ostream& operator<<(ostream &out, const Inlining &t)
{
  return t.print(out);
}

ostream& operator<<(ostream &out, const Inlining *t)
{
  return t->print(out);
}


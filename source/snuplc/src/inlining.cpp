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

	// Create Main fNode
	fNode* tNode = new fNode(cb_len, _module->GetName(), module);
	_nodes.push_back(tNode);
	
	// Create fNode of other functions
	vector<CScope*> child = _module->GetSubscopes();
	for (int i=0; i<child.size(); i++)
	{
		cb_len = child[i]->GetCodeBlock()->GetInstr().size();
		tNode = new fNode(cb_len, child[i]->GetName(), child[i]);
		_nodes.push_back(tNode);
	}

	// Create fCallNode for all functions
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
					/* START of while or if */
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
					/* END of while or if */
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

int Inlining::doOneStep()
{
//	cout<<this<<endl;
	// Check do inlining or not
	int Peek = PeekCBSize();
	cout<<"original: "<<_original_length<<" / predicted: "<<Peek<<endl;
	if (_original_length == Peek)
	{
		cout<<"Nothing left to inline"<<endl<<endl;
		return -1;
	}

	if (INLINING_LIMIT(_original_length) < Peek)
	{
		cout<<"Size limit: Do not inlining" << endl<<endl;
		return -1;
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
	if (max_index < 0)
	{
		cout<<"Nothing left to inline"<<endl<<endl;
		return -1;
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
//	cout<<"target: "<<proc<<" "<<target->GetName()<<endl<<endl;
	
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
//		cout<<tt->first<<" "<<tt->second->GetName()<<" " <<tt->second->GetDataType()<<endl;
	}

	//it : call location of target calling in parent
	
	list<CTacInstr*>::const_iterator parm_it = it;
//	cout<<tcb<<endl;
	cout<<"Inline "<<*it<<endl<<endl;
	CTac* tCallDst = NULL;
	// Analyze Call
	{
		CTacInstr *tempCall = *it;
		tCallDst = tempCall->GetDest();
//		if (tCallDst == NULL)
//			cout<<"No return" <<endl;
//		else
//			cout<<"Return" <<endl;
	}

	// Copy function body of target of inlining
	vector<CTacInstr*> new_instr_vector;

	inline_labels.clear();

	for (list<CTacInstr*>::const_iterator tparm_it = tops.begin(); tparm_it != tops.end();tparm_it++)
	{
		CTacInstr* tinstr = *tparm_it;
		//cout<<tinstr<<endl;

		// If label
		if (tinstr)
		{
			if (CTacLabel* ctlabel = dynamic_cast<CTacLabel*>(tinstr))
			{
				tinstr = GetLabel(tcb, ctlabel);

//				cout<<tinstr<<endl;
				new_instr_vector.push_back(tinstr);
//				cout<<"1-----------------"<<endl;
				continue;
			}
		}

		// If not label
		EOperation top = tinstr->GetOperation();

		// If Return
		if (top == opReturn)
		{
//			cout<<"Return"<<endl;

			CTacAddr* tsrc1 = tinstr->GetSrc(1);
			if (CTacTemp* cttemp = dynamic_cast <CTacTemp*>(tsrc1))
			{
                if (inline_vars.find(cttemp->GetSymbol()->GetName()) != inline_vars.end())
                {
                    tsrc1 = new CTacTemp(inline_vars[cttemp->GetSymbol()->GetName()]);
//                    cout<<inline_vars[cttemp->GetSymbol()->GetName()]<<endl;
                }
			}
				else if (CTacReference* ctref = dynamic_cast <CTacReference*>(tsrc1))
				{
//					cout<<"CTacReference "<<tdst<<endl;
//					cout << " "<<ctref->GetSymbol()->GetName()<<endl;
//					cout << " "<<ctref->GetDerefSymbol()->GetName()<<endl;
					const CSymbol *tsym = ctref->GetSymbol();
					const CSymbol *tderef = ctref->GetDerefSymbol();
					if (inline_vars.find(tsym->GetName()) != inline_vars.end())
						tsym = inline_vars[tsym->GetName()];
					if (inline_vars.find(tderef->GetName()) != inline_vars.end())
						tderef = inline_vars[tderef->GetName()];
					tsrc1 = new CTacReference(tsym, tderef);
				}
			else if (CTacName* ctname = dynamic_cast<CTacName*>(tsrc1))
			{
//				cout<<"CTacName "<<tsrc1<<endl;
				if (inline_vars.find(ctname->GetSymbol()->GetName()) != inline_vars.end())
				{
//					cout<<"bef: "<<tsrc1<<endl;
					tsrc1 = new CTacName(inline_vars[ctname->GetSymbol()->GetName()]);
//					cout<<"aft: "<<tsrc1<<endl;
				}
			}


			if (tCallDst != NULL)
			{
				CTacInstr* newInstr = new CTacInstr(opAssign, tCallDst, tsrc1);

				new_instr_vector.push_back(newInstr);
//				cout<<newInstr<<endl;
			}

		}
		// Not Return
		else 
		{
			// Add prefix i_* to dest, src1, src2
			CTac* tdst = tinstr->GetDest();
			if (tdst)
			{
//				cout<<"1 "<<tdst<<endl;
				if (CTacConst* ctconst = dynamic_cast <CTacConst*>(tdst))
				{
//					cout<<"CTacConst "<<tdst<<endl;
				}
				else if (CTacTemp* cttemp = dynamic_cast <CTacTemp*>(tdst))
				{
//					cout<<"CTacTemp "<<tdst<<endl;
					if (inline_vars.find(cttemp->GetSymbol()->GetName()) != inline_vars.end())
					{
						tdst = new CTacTemp(inline_vars[cttemp->GetSymbol()->GetName()]);
//						cout<<inline_vars[cttemp->GetSymbol()->GetName()]<<endl;
					}
				}
				else if (CTacReference* ctref = dynamic_cast <CTacReference*>(tdst))
				{
//					cout<<"CTacReference "<<tdst<<endl;
//					cout << " "<<ctref->GetSymbol()->GetName()<<endl;
//					cout << " "<<ctref->GetDerefSymbol()->GetName()<<endl;
					const CSymbol *tsym = ctref->GetSymbol();
					const CSymbol *tderef = ctref->GetDerefSymbol();
					if (inline_vars.find(tsym->GetName()) != inline_vars.end())
						tsym = inline_vars[tsym->GetName()];
					if (inline_vars.find(tderef->GetName()) != inline_vars.end())
						tderef = inline_vars[tderef->GetName()];
					tdst = new CTacReference(tsym, tderef);
				}
				else if (CTacLabel* ctlabel = dynamic_cast<CTacLabel*>(tdst))
				{
//					cout<<"CtacLabel "<<tdst <<endl;
					ctlabel = GetLabel(tcb, ctlabel);
					tdst = ctlabel;
					// GOTO label
				}
				else if (CTacName* ctname = dynamic_cast<CTacName*>(tdst))
				{
//					cout<<"CTacName "<<tdst<<endl;
					if (inline_vars.find(ctname->GetSymbol()->GetName()) != inline_vars.end())
					{
						tdst = new CTacName(inline_vars[ctname->GetSymbol()->GetName()]);
					}
				}
			}

			CTacAddr* tsrc1 = tinstr->GetSrc(1);
			if (tsrc1)
			{
//				cout<<"2 "<<tsrc1<<endl;
				if (CTacConst* ctconst = dynamic_cast <CTacConst*>(tsrc1))
				{
//					cout<<"CTacConst "<<tsrc1<<endl;
				}
				else if (CTacTemp* cttemp = dynamic_cast <CTacTemp*>(tsrc1))
				{
//					cout<<"CTacTemp "<<tsrc1<<endl;
					if (inline_vars.find(cttemp->GetSymbol()->GetName()) != inline_vars.end())
					{
						tsrc1 = new CTacTemp(inline_vars[cttemp->GetSymbol()->GetName()]);
					}
				}
				else if (CTacReference* ctref = dynamic_cast <CTacReference*>(tsrc1))
				{
//					cout<<"CTacReference "<<tsrc1<<endl;
//					cout << " "<<ctref->GetSymbol()->GetName()<<endl;
//					cout << " "<<ctref->GetDerefSymbol()->GetName()<<endl;
					const CSymbol *tsym = ctref->GetSymbol();
					const CSymbol *tderef = ctref->GetDerefSymbol();
					if (inline_vars.find(tsym->GetName()) != inline_vars.end())
						tsym = inline_vars[tsym->GetName()];
					if (inline_vars.find(tderef->GetName()) != inline_vars.end())
						tderef = inline_vars[tderef->GetName()];
					tsrc1 = new CTacReference(tsym, tderef);
				}
				else if (CTacName* ctname = dynamic_cast<CTacName*>(tsrc1))
				{
//					cout<<"CTacName "<<tsrc1<<endl;
					if (inline_vars.find(ctname->GetSymbol()->GetName()) != inline_vars.end())
					{
						tsrc1 = new CTacName(inline_vars[ctname->GetSymbol()->GetName()]);
					}
				}
			}

			CTacAddr* tsrc2 = tinstr->GetSrc(2);
			if (tsrc2)
			{
//				cout<<"3 "<<tsrc2<<endl;
				if (CTacConst* ctconst = dynamic_cast <CTacConst*>(tsrc2))
				{
//					cout<<"CTacConst "<<tsrc2<<endl;
				}
				else if (CTacTemp* cttemp = dynamic_cast <CTacTemp*>(tsrc2))
				{
//					cout<<"CTacTemp "<<tsrc2<<endl;
					if (inline_vars.find(cttemp->GetSymbol()->GetName()) != inline_vars.end())
					{
						tsrc2 = new CTacTemp(inline_vars[cttemp->GetSymbol()->GetName()]);
					}
				}
				else if (CTacReference* ctref = dynamic_cast <CTacReference*>(tsrc2))
				{
//					cout<<"CTacReference "<<tsrc2<<endl;
//					cout << " "<<ctref->GetSymbol()->GetName()<<endl;
//					cout << " "<<ctref->GetDerefSymbol()->GetName()<<endl;
					const CSymbol *tsym = ctref->GetSymbol();
					const CSymbol *tderef = ctref->GetDerefSymbol();
					if (inline_vars.find(tsym->GetName()) != inline_vars.end())
						tsym = inline_vars[tsym->GetName()];
					if (inline_vars.find(tderef->GetName()) != inline_vars.end())
						tderef = inline_vars[tderef->GetName()];
					tsrc2 = new CTacReference(tsym, tderef);
				}
				else if (CTacName* ctname = dynamic_cast<CTacName*>(tsrc2))
				{
//					cout<<"CTacName "<<tsrc2<<endl;
					if (inline_vars.find(ctname->GetSymbol()->GetName()) != inline_vars.end())
					{
						tsrc2 = new CTacTemp(inline_vars[ctname->GetSymbol()->GetName()]);
					}
				}
			}
			CTacInstr* newInstr = new CTacInstr(top, tdst, tsrc1, tsrc2);
			new_instr_vector.push_back(newInstr);
			//cout<<newInstr<<endl;
			//cout<<"-----------------"<<endl;
		}
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

	for (int i=0; i<new_instr_vector.size(); i++)
	{
		tcb->InsInstr((*it)->GetId(), new_instr_vector[i]);
	}

	// Remove Call operation
	tcb->DelInstr((*it)->GetId());
	tcb->CleanupControlFlow();

	// clear all and parse TAC again
	int temp_original = _original_length;
	for (int i=0; i<_nodes.size(); i++)
	{
		delete _nodes[i];
	}
	_nodes.clear();

	parseTAC(_module);

	_original_length = temp_original;


	// recalculate parameters after inlining
	// LEN update
	for (int i=0; i<_nodes.size(); i++)
	{
		_nodes[i]->SetCodeLen(_nodes[i]->GetModule()->GetCodeBlock()->GetInstr().size());
	}

	calculateScore();


	_param_iter++;

	return 0;
}

int Inlining::GetCBSize() // Get current size
{
	int size = 0;
	for (int i=0; i<_nodes.size(); i++)
		size += _nodes[i]->GetCodeLen();

	return size;
}

int Inlining::PeekCBSize() // Estimate size of worst case
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

CTacLabel* Inlining::GetLabel(CCodeBlock* tcb, CTacLabel* label)
{
	string labName = label->GetLabel();
	if (inline_labels[labName] != NULL)
		return inline_labels[labName];

	CTacLabel* tinstr;

	size_t found = labName.find('_');
	if (found != string::npos)
	{
		int num = stoi(labName.substr(0, found));
		string subLabName = labName.substr(found+1);


		if (subLabName.compare("while_cond") == 0)
		{
			CTacLabel* tempLabel = tcb->CreateLabel();
			inline_labels[to_string(num-1)] = tempLabel;

			tempLabel = tcb->CreateLabel(subLabName.c_str());
			inline_labels[labName] = tempLabel;
			tinstr = tempLabel;

			tempLabel = tcb->CreateLabel("while_body");
			inline_labels[to_string(num+1)+"_while_body"] = tempLabel;
		}
		else if (subLabName.compare("while_body")==0)
		{
			tinstr = inline_labels[labName];
		}
		else if (subLabName.compare("if_true")==0)
		{
			CTacLabel* tempLabel = tcb->CreateLabel();
			inline_labels[to_string(num-1)] = tempLabel;

			tempLabel = tcb->CreateLabel(subLabName.c_str());
			inline_labels[labName] = tempLabel;
			tinstr = tempLabel;

			tempLabel = tcb->CreateLabel("if_false");
			inline_labels[to_string(num+1)+"_if_false"] = tempLabel;
		}
		else if (subLabName.compare("if_false")==0)
		{
			tinstr = inline_labels[labName];
		}
	}
	else
	{
		if (inline_labels[labName] != NULL)
			return inline_labels[labName];

		CTacLabel* tempLabel = tcb->CreateLabel();
		inline_labels[labName] = tempLabel;
		return tempLabel;
	}
	return tinstr;

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


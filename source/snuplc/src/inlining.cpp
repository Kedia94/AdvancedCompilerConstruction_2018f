#include <cassert>
#include <cmath>

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

		float score = INLINING_SCORE_LEN(tNode->GetNode()->GetCodeLen());
		score *= base_score;
		score *= pow((float)INLINING_CALL_IN_LOOP, (float)tNode->GetLoopLevel());
		tNode->SetScore(score);
	}
}

void Inlining::doOneStep()
{
	int Peek = PeekCBSize();
	cout<<"original: "<<_original_length<<" / predicted: "<<Peek<<endl;
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

	// TODO: Inline (max_index, max_child_index)
	CScope* parent = _nodes[max_index]->GetModule();
	CScope* target = _nodes[max_index]->GetChild(max_child_index)->GetNode()->GetModule();

	CCodeBlock* tcb = parent->GetCodeBlock();
	list<CTacInstr*> ops = tcb->GetInstr();

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
	cout<<"target: "<<proc<<endl;
	assert(proc);
	const CSymParam* parm;
	vector<string> param_vector;
	int paramnum = proc->GetNParams();
	for (int i=0; i<paramnum; i++){
		parm = proc->GetParam(i);
		cout<<i<<": "<< parm->GetName() << ", Type: "<<parm->GetDataType()<<endl;
		param_vector.push_back(parm->GetName());
	}





	param_vector.clear();

	// Remove child
	_nodes[max_index]->RemoveChild(max_child_index);


	// recalculate parameters after inlining
	// LEN update
	for (int i=0; i<_nodes.size(); i++)
	{
		_nodes[i]->SetCodeLen(_nodes[i]->GetModule()->GetCodeBlock()->GetInstr().size());
	}

	// TODO: Child update with loop level
	calculateScore();


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


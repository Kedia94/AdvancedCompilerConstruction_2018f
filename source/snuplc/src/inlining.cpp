

#include "inlining.h"

Inlining::Inlining(void)
{
	_module = NULL;
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
	
	//TODO
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
						fNodeWithLoop* tLoop = new fNodeWithLoop(tempNode, 1);
						_nodes[i]->AddChild(tLoop);
					}
				}
			}
		}
	}
}



void Inlining::doOneStep()
{
}

fNode* Inlining::FindSymbol(const CSymbol* symbol)
{
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


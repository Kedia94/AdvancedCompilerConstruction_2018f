#include "fnode.h"
#include "ir.h"

//------------------------------------------------------------------------------
// fNode
//
fNode::fNode(int code_length, string name, CScope *module)
{
	_code_length = code_length;
	_name = name;
	_module = module;
}

fNode::~fNode(void)
{
	for (int i=0; i<_children.size(); i++)
	{
		delete _children[i];
	}
	_children.clear();
}

void fNode::SetCodeLen(int code_length)
{
	_code_length = code_length;
}

int fNode::GetCodeLen() const
{
	return _code_length;
}

int fNode::GetChildScore(int index) const
{
	// TODO
	return index * index;
}

fNodeWithLoop* fNode::GetChild(int index)
{
	if (index < _children.size())
		return _children[index];

	return NULL;
}

int fNode::AddChild(fNodeWithLoop* child)
{
	_children.push_back(child);
}

void fNode::RemoveChild(int index)
{
	if (index < _children.size())
	{
		fNodeWithLoop* tempLoop = _children[index];
		_children.erase(_children.begin() + index - 1);
		delete tempLoop;
	}
}

void fNode::SetName(string name)
{
	_name = name;
}

string fNode::GetName() const
{
	return _name;
}

CScope* fNode::GetModule() const
{
	return _module;
}

ostream& fNode::print(ostream &out, int indent) const
{
	string ind(indent, ' ');

	out << ind << "[[ " << GetName() << endl;
	out << ind << "   len: " << GetCodeLen() << endl;

	for (int i=0; i<_children.size(); i++)
	{
		_children[i]->print(out, indent+2);
	}
	out << ind << "]]" << endl;

	return out;
}

//------------------------------------------------------------------------------
// fNodeWithLoop
//
fNodeWithLoop::fNodeWithLoop(fNode* original, int loop_level)
{
	_node = original;
	_loop_level = loop_level;
	_score = -1;
	_name = original->GetName();
}

fNodeWithLoop::~fNodeWithLoop(void)
{
}

void fNodeWithLoop::SetLoopLevel(int loop_level)
{
	_loop_level = loop_level;
}

int fNodeWithLoop::GetLoopLevel() const
{
	return _loop_level;
}

void fNodeWithLoop::SetScore(int score)
{
	_score = score;
}

int fNodeWithLoop::GetScore() const
{
	return _score;
}

fNode* fNodeWithLoop::GetNode()
{
	return _node;
}

void fNodeWithLoop::SetName(string name)
{
	_name = name;
}

string fNodeWithLoop::GetName() const
{
	return _name;
}

ostream& fNodeWithLoop::print(ostream &out, int indent) const
{
	string ind(indent, ' ');

	out << ind << "[[ " << GetName() << endl;
	out << ind << "   score: " << GetScore() << endl;
	out << ind << "   loop: " << GetLoopLevel() << endl;


	out << ind << "]]" << endl;

	return out;
}

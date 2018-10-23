#ifndef __SnuPL_FNODE_H__
#define __SnuPL_FNODE_H__

#include <iostream>
#include <vector>

#include "ir.h"

using namespace std;

class fNodeWithLoop;

class fNode {
	public:
		fNode(int code_length, string name, CScope* module);
		~fNode(void);
		void SetCodeLen(int code_length);
		int GetCodeLen() const;
		int GetChildLen() const;
		int GetChildScore(int index) const;
		fNodeWithLoop* GetChild(int index);
		int AddChild(fNodeWithLoop* child);
		void RemoveChild(int index);
		void SetName(string name);
		string GetName() const;
		CScope* GetModule() const;
		virtual ostream&  print(ostream &out, int indent=0) const;

	protected:
		int _code_length;
		vector<fNodeWithLoop*> _children;
		string _name;
		CScope* _module;
};

class fNodeWithLoop {
	public:
		fNodeWithLoop(fNode* original, int loop_level);
		~fNodeWithLoop(void);
		void SetLoopLevel(int loop_level);
		int GetLoopLevel() const;
		void SetScore(int score);
		int GetScore() const;
		fNode* GetNode();
		void SetName(string name);
		string GetName() const;
		virtual ostream&  print(ostream &out, int indent=0) const;

	protected:
		int _score;
		int _loop_level;
		fNode* _node;
		string _name;
};
#endif

/*
 * DirTree.h
 *
 *  Created on: 31.05.2012
 *      Author: ruslan
 */

#ifndef DIRTREE_H_
#define DIRTREE_H_
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "../err/err_code.h"
#include "../exceptions/exceptions.h"

namespace dinosaur {
namespace dir_tree {

class Dir
{
private:
	typedef std::map<std::string, Dir*> children_map;
	typedef children_map::const_iterator children_map_iter;
private:
	std::string m_name;
	children_map m_children;
public:
	Dir();
	Dir(const Dir & node);
	Dir(const std::string & m_name);
	Dir(const char * name);
	std::string get_name();
	Dir * add_child(const std::string  & name);
	Dir * add_child(const char * name);
	void make(std::string current_dir) throw (SyscallException);
	~Dir();
};

class DirTree {
private:
	Dir * m_root;
	Dir * m_iter;
public:
	DirTree();
	DirTree(const DirTree & dirtree);
	DirTree(const std::string & root);
	DirTree & operator = (const DirTree & dirtree);
	void reset();
	void put(const std::string & dir) throw (Exception);
	void put(const char * dir) throw (Exception);
	void make_dir_tree(std::string current_dir) throw (Exception, SyscallException);
	~DirTree();
};

class DirTreeTest
{
public:
	DirTreeTest(){}
	~DirTreeTest(){}
	void test_Dir()
	{
		std::string n = "test_dir";
		Dir dir(n);
		std::string c = "child1";
		dir.add_child(c);
		c = "child2";
		dir.add_child(c);
		c = "child1";
		dir.add_child(c);
		c = "";
		dir.add_child(c);
		c = "имя папки на русском русском";
		dir.add_child(c);
		std::string d = "";
		//dir.make(d);
		Dir dir_copy = dir;
		dir_copy.make(d);
	}
	void test_DirTree()
	{
		std::string n = "tree_test";
		DirTree tree(n);
		std::string v = "1";
		tree.put(v);
		v = "2";
		tree.put(v);
		tree.reset();
		v = "11";
		tree.put(v);
		tree.reset();
		v = "12";
		tree.put(v);

		tree.reset();
		v = "1";
		tree.put(v);
		v = "3";
		tree.put(v);

		tree.reset();
		v = "12";
		tree.put(v);
		v = "123";
		tree.put(v);

		v = "test_dir";
		DirTree tree_copy;
		tree_copy = tree;
		tree_copy.make_dir_tree(v);
	}
};

} /* namespace Bittorrent */
}
#endif /* DIRTREE_H_ */

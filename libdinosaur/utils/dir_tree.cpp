/*
 * DirTree.cpp
 *
 *  Created on: 31.05.2012
 *      Author: ruslan
 */

#include "dir_tree.h"

namespace dinosaur {
namespace dir_tree {

Dir::Dir()
{
	m_name = "";
}

Dir::Dir(const std::string & name)
{
	m_name = name;
}

Dir::Dir(const char * name)
{
	m_name = name;
}

Dir::Dir(const Dir & node)
{
	for(children_map_iter iter = node.m_children.begin(); iter != node.m_children.end(); ++iter)
	{
		m_children[iter->first] = new Dir(*iter->second);
	}
	this->m_name = node.m_name;
}

std::string Dir::get_name()
{
	return m_name;
}

Dir * Dir::add_child(const std::string & name)
{
	if (name == "")
		return NULL;
	children_map_iter iter = m_children.find(name);
	if (iter == m_children.end())
	{
		Dir * dir = new Dir(name);//bad alloc пробросить выше, а там поймаем)
		m_children[name] = dir;
		return dir;
	}
	else
		return (*iter).second;
}

Dir * Dir::add_child(const char * name)
{
	if (name == NULL)
		return NULL;
	std::string str_name = name;
	return add_child(str_name);
}

void Dir::make(std::string current_dir) throw (SyscallException)
{
	current_dir += m_name + "/";
	//std::cout<<"making "<<current_dir<<std::endl;
	if (mkdir(current_dir.c_str(), S_IRWXU) != 0 && errno != EEXIST)
		throw SyscallException();
	for(children_map_iter iter = m_children.begin(); iter != m_children.end(); ++iter)
	{
		Dir * dir = (*iter).second;
		dir->make(current_dir);
	}
}

Dir::~Dir()
{
	for(children_map_iter iter = m_children.begin(); iter != m_children.end(); ++iter)
		delete (*iter).second;
}

DirTree::DirTree() {
	// TODO Auto-generated constructor stub
	m_root = NULL;
	m_iter = NULL;
}

DirTree::DirTree(const DirTree & dirtree)
{
	this->m_root = dirtree.m_root == NULL ? NULL : new Dir(*dirtree.m_root);
	this->m_iter = m_root;
}

DirTree::DirTree(const std::string & root)
{
	m_root = new Dir(root);
	m_iter = m_root;
}

DirTree & DirTree::operator = (const DirTree & dirtree)
{
	if (this != &dirtree)
	{
		if (m_root != NULL)
			delete m_root;

		m_root = dirtree.m_root == NULL ? NULL : new Dir(*dirtree.m_root);
		m_iter = m_root;
	}
	return *this;
}

DirTree::~DirTree() {
	// TODO Auto-generated destructor stub
	if (m_root != NULL)
		delete m_root;
}

void DirTree::reset()
{
	if (m_root == NULL)
		return;
	if (m_root != NULL)
		m_iter = m_root;
}

void DirTree::put(const std::string & dir2put) throw (Exception)
{
	return put(dir2put.c_str());
}

void DirTree::put(const char * dir2put) throw (Exception)
{
	if (m_iter == NULL || dir2put == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	Dir * dir;
	dir = m_iter->add_child(dir2put);
	if (dir == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	m_iter = dir;
}

void DirTree::make_dir_tree(std::string  current_dir) throw (Exception, SyscallException)
{
	if (m_root == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	if (current_dir.length() != 0 && current_dir[current_dir.length() - 1] != '/')
		current_dir += "/";
	m_root->make(current_dir);
}

} /* namespace Bittorrent */
}

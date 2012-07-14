/*
 * DirTree.cpp
 *
 *  Created on: 31.05.2012
 *      Author: ruslan
 */

#include "dir_tree.h"

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

int Dir::make(std::string current_dir)
{
	current_dir += m_name + "/";
	//std::cout<<"making "<<current_dir<<std::endl;
	if (mkdir(current_dir.c_str(), S_IRWXU) != 0 && errno != EEXIST)
		return ERR_SYSCALL_ERROR;
	for(children_map_iter iter = m_children.begin(); iter != m_children.end(); ++iter)
	{
		Dir * dir = (*iter).second;
		int ret = dir->make(current_dir);
		if (ret != ERR_NO_ERROR)
			return ret;
	}
	return ERR_NO_ERROR;
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
	this->m_root = new Dir(*dirtree.m_root);
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

		m_root = new Dir(*dirtree.m_root);
		m_iter = m_root;
	}
	return *this;
}

DirTree::~DirTree() {
	// TODO Auto-generated destructor stub
	if (m_root != NULL)
		delete m_root;
}

int DirTree::reset()
{
	if (m_root == NULL)
		return ERR_INTERNAL;
	if (m_root != NULL)
		m_iter = m_root;
	return ERR_NO_ERROR;
}

int DirTree::put(const std::string & dir2put)
{
	return put(dir2put.c_str());
}

int DirTree::put(const char * dir2put)
{
	if (m_iter == NULL)
		return ERR_INTERNAL;
	if (dir2put == NULL)
		return ERR_BAD_ARG;
	Dir * dir;
	try
	{
		dir = m_iter->add_child(dir2put);
		if (dir == NULL)
			return ERR_INTERNAL;
		m_iter = dir;
		return ERR_NO_ERROR;
	}
	catch(...)
	{
		return ERR_INTERNAL;
	}
	return ERR_NO_ERROR;
}

int DirTree::make_dir_tree(std::string current_dir)
{
	if (m_root == NULL)
		return ERR_INTERNAL;
	if (current_dir.length() != 0 && current_dir[current_dir.length() - 1] != '/')
		current_dir += "/";
	return m_root->make(current_dir);
}

} /* namespace Bittorrent */

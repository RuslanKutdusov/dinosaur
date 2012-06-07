/*
 * fs_tests.h
 *
 *  Created on: 06.06.2012
 *      Author: ruslan
 */

#ifndef FS_TESTS_H_
#define FS_TESTS_H_

#include <iostream>
#include "fs.h"

void test_write_to_file()
{
	fs::FileManager fm;
	fm.Init_for_tests(512, 1);
	fm.test_dump_fd_cache();
	int id = fm.File_add("fm_test", 100, false, NULL);
	fm.test_dump_fd_cache();
	char buf[100];
	fm.File_write(id, buf, 100, 0, 123);
	sleep(2);
	fm.test_dump_fd_cache();
}

void test_fd_cache_test()
{
	fs::FileManager fm;
	fm.Init_for_tests(512, 3);
	//fm.test_dump_fd_cache();
	int id1 = fm.File_add("fm_test1", 100, false, NULL);
	int id2 = fm.File_add("fm_test2", 100, false, NULL);
	int id3 = fm.File_add("fm_test3", 100, false, NULL);
	int id4 = fm.File_add("fm_test4", 100, false, NULL);
	fm.test_dump_fd_cache();
	char buf[100];

	fm.File_write(id1, buf, 100, 0, 123);
	fm.File_write(id2, buf, 100, 0, 123);
	fm.File_write(id3, buf, 100, 0, 123);
	sleep(2);
	fm.test_dump_fd_cache();

	fm.File_write(id1, buf, 100, 0, 123);
	sleep(2);
	fm.test_dump_fd_cache();

	fm.File_write(id4, buf, 100, 0, 123);
	sleep(2);
	fm.test_dump_fd_cache();

	fm.File_write(id2, buf, 100, 0, 123);
	fm.File_write(id3, buf, 100, 0, 123);
	sleep(2);
	fm.test_dump_fd_cache();
	std::cin>>id1;
}

#endif /* FS_TESTS_H_ */

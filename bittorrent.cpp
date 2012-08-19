//============================================================================
// Name        : bittorrent.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "gui/gui.h"
#include <execinfo.h>

void catchCrash(int signum)
{
	void *callstack[128];
    int frames = backtrace(callstack, 128);
    char **strs=backtrace_symbols(callstack, frames);
    // тут выводим бэктрейс в файлик crash_report.txt
    // можно так же вывести и иную полезную инфу - версию ОС, программы, etc
    FILE *f = fopen("crash_report.txt", "w");
    if (f)
    {
        for(int i = 0; i < frames; ++i)
        {
            fprintf(f, "%s\n", strs[i]);
        }
        fclose(f);
    }
    free(strs);
    signal(signum, SIG_DFL);
	exit(3);
}

int main(int argc,char* argv[])
{
	signal(SIGSEGV, catchCrash);
	signal(SIGABRT, catchCrash);
	dinosaur::ERR_CODES_STR::save_defaults();
	try
	{
		init_gui();
	}
	catch(dinosaur::Exception  &e)
	{
		printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
	}
	return 0;
}

/*
 * gui.h
 *
 *  Created on: 20.05.2012
 *      Author: ruslan
 */

#ifndef GUI_H_
#define GUI_H_
#include <gtk/gtk.h>
#include <list>
#include <string>
#include <errno.h>
#include "../libdinosaur/dinosaur.h"
#include "../libdinosaur/torrent/torrent.h"
#include "../libdinosaur/torrent/torrent_types.h"
#include "../libdinosaur/exceptions/exceptions.h"


//extern "C" void on_window1_destroy (GtkObject *object, gpointer user_data);
//extern "C" void on_button_open_clicked (GtkObject *object, gpointer user_data);
void init_gui();
void show_open_dialog();
void show_cfg_dialog();
bool cfg_opened();
void update_statusbar();
void messagebox(const char * message);
void messagebox(const std::string & message);
void set_file_tree(const std::string  & hash, uint64_t files_count);
extern "C" void on_window1_show (GtkWidget *object, gpointer user_data);

extern dinosaur::DinosaurPtr bt;

#endif /* GUI_H_ */

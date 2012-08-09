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
#include "../dinosaur.h"
#include "../torrent/torrent.h"
#include "../torrent/torrent_types.h"


//extern "C" void on_window1_destroy (GtkObject *object, gpointer user_data);
//extern "C" void on_button_open_clicked (GtkObject *object, gpointer user_data);
void init_gui();
void show_open_dialog();
extern "C" void on_window1_show (GtkWidget *object, gpointer user_data);

#endif /* GUI_H_ */

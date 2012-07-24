/*
 * gui.cpp
 *
 *  Created on: 20.05.2012
 *      Author: ruslan
 */

#include "gui.h"

GtkWidget *window;
GtkDialog* open_dialog;
GtkFileChooserWidget * open_dialog_dir_chooser;
GtkTreeView* torrent_view;
GtkListStore* torrent_list;
GtkListStore* tracker_list;
GtkListStore* peer_list;
GtkLabel* label_hash;
GtkLabel* label_length;
GtkLabel* label_dir;
GtkLabel* label_piece_number;
GtkLabel* label_piece_length;
bittorrent::Bittorrent * bt;
torrent::MetafilePtr metafile;

enum
{
	TORRENT_LIST_COL_NAME,
	TORRENT_LIST_COL_PROGRESS,
	TORRENT_LIST_COL_DOWNLOADED,
	TORRENT_LIST_COL_UPLOADED,
	TORRENT_LIST_COL_DOWN_SPEED,
	TORRENT_LIST_COL_UP_SPEED,
	TORRENT_LIST_COL_HASH,
	TORRENT_LIST_COLS
};

enum
{
	TRACKER_LIST_COL_ANNOUNCE,
	TRACKER_LIST_COL_STATUS,
	TRACKER_LIST_COL_UPDATE_IN,
	TRACKER_LIST_COL_LEECHERS,
	TRACKER_LIST_COL_SEEDERS,
	TRACKER_LIST_COLS
};

enum
{
	PEER_LIST_COL_IP,
	PEER_LIST_COL_DOWNLOADED,
	PEER_LIST_COL_UPLOADED,
	PEER_LIST_COL_DOWN_SPEED,
	PEER_LIST_COL_UP_SPEED,
	PEER_LIST_COL_AVAILABLE,
	PEER_LIST_COLS
};

void messagebox(const char * message)
{
	GtkWidget * dialog = gtk_message_dialog_new ((GtkWindow*)window,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s", message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

extern "C" void on_button_open_clicked (GtkWidget *object, gpointer user_data)
{
	if (open_dialog != NULL)
		return;
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new ("Open File",
					      (GtkWindow*)window,
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		try
		{
			metafile.reset(new torrent::Metafile(filename));
			gtk_widget_destroy (dialog);
			show_open_dialog();
		}
		catch(Exception & e)
		{
			gtk_widget_destroy (dialog);
			messagebox(e.get_error().c_str());
		}
		g_free (filename);
	}
	else
		gtk_widget_destroy (dialog);
}

extern "C" void on_button_start_clicked (GtkWidget *object, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       selected_iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
	if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
	{
		gchar *g_hash;
		gtk_tree_model_get (model, &selected_iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
		std::string hash = g_hash;
		g_free(g_hash);
		bt->ContinueTorrent(hash);
	}
}

extern "C" void on_button_stop_clicked (GtkWidget *object, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       selected_iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
	if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
	{
		gchar *g_hash;
		gtk_tree_model_get (model, &selected_iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
		std::string hash = g_hash;
		g_free(g_hash);
		bt->PauseTorrent(hash);
	}
}

extern "C" void on_button_delete_clicked (GtkWidget *object, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       selected_iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
	if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
	{
		gchar *g_hash;
		gtk_tree_model_get (model, &selected_iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
		std::string hash = g_hash;
		g_free(g_hash);
		bt->DeleteTorrent(hash);
		on_window1_show(NULL,NULL);
	}
}

extern "C" void on_button_check_clicked(GtkWidget *object, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       selected_iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
	if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
	{
		gchar *g_hash;
		gtk_tree_model_get (model, &selected_iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
		std::string hash = g_hash;
		g_free(g_hash);
		bt->CheckTorrent(hash);
	}

}

extern "C" void on_window1_destroy (GtkWidget *object, gpointer user_data)
{
	delete bt;
	gtk_main_quit();
}


extern "C" void on_open_dialog_close (GtkWidget *object, gpointer user_data)
{

}

extern "C" void on_open_dialog_button_ok_clicked (GtkWidget *object, gpointer user_data)
{
	char *dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(open_dialog_dir_chooser));
	if (dir == NULL)
	{
		messagebox("Choose directory for saving");
		return;
	}
	std::string dir_str = dir;
	std::string hash;
	if (bt->AddTorrent(*metafile, dir_str, hash) != ERR_NO_ERROR)
	{
		gtk_object_destroy(GTK_OBJECT(open_dialog));
		messagebox(bt->get_error().c_str());
		open_dialog = NULL;
		metafile.reset();
		return;
	}
	if (bt->StartTorrent(hash) != ERR_NO_ERROR)
	{
		gtk_object_destroy(GTK_OBJECT(open_dialog));
		messagebox(bt->get_error().c_str());
		open_dialog = NULL;
		metafile.reset();
		return;
	}
	g_free(dir);
	//commit
	gtk_object_destroy(GTK_OBJECT(open_dialog));
	open_dialog = NULL;
	metafile.reset();
	on_window1_show(NULL,NULL);
}

extern "C" void on_open_dialog_button_cancel_clicked (GtkWidget *object, gpointer user_data)
{
	gtk_object_destroy(GTK_OBJECT(open_dialog));
	open_dialog = NULL;
	metafile.reset();
}


extern "C" void on_window1_show (GtkWidget *object, gpointer user_data)
{
	GtkTreeIter   iter;
	gtk_list_store_clear(torrent_list);
	std::list<std::string> torrents;
	bt->get_TorrentList(&torrents);
	if (torrents.empty())
		return;
	for(std::list<std::string>::iterator i = torrents.begin(); i != torrents.end(); ++i)
	{
		gtk_list_store_append(torrent_list, &iter);
		torrent::torrent_info info;
		bt->Torrent_info(*i, &info);
		float downloaded = info.downloaded / 1048576;
		float uploaded = info.uploaded / 1048576;
		float up_speed = info.tx_speed;
		float down_speed = info.rx_speed;
		gtk_list_store_set (torrent_list, &iter,
										TORRENT_LIST_COL_NAME, info.name.c_str(),
										TORRENT_LIST_COL_PROGRESS, (gint)info.progress,
										TORRENT_LIST_COL_DOWNLOADED, (gfloat)downloaded,
										TORRENT_LIST_COL_UPLOADED, (gfloat)uploaded,
										TORRENT_LIST_COL_DOWN_SPEED, (gfloat)down_speed,
										TORRENT_LIST_COL_UP_SPEED, (gfloat)up_speed,
										TORRENT_LIST_COL_HASH, (*i).c_str(),
						  -1);
	}
}

extern "C" void  on_torrent_view_cursor_changed(GtkWidget *object, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *g_hash;
		gtk_tree_model_get (model, &iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
		std::string hash = g_hash;
		//g_print ("selected row is: %s\n", name);
		g_free(g_hash);

		//GtkTreeIter   iter;
		gtk_list_store_clear(tracker_list);
		gtk_list_store_clear(peer_list);

		torrent::torrent_info info;
		bt->Torrent_info(hash, &info);

		gtk_label_set_text(label_hash, hash.c_str());
		gtk_label_set_text(label_dir, info.download_directory.c_str());
		char chars[100];
		sprintf(chars, "%llu", info.length);
		gtk_label_set_text(label_length, chars);

		sprintf(chars, "%llu", info.piece_length);
		gtk_label_set_text(label_piece_length, chars);

		sprintf(chars, "%u", info.piece_count);
		gtk_label_set_text(label_piece_number, chars);

		for(torrent::torrent_info::tracker_info_iter i = info.trackers.begin(); i != info.trackers.end(); ++i)
		{
			gtk_list_store_append(tracker_list, &iter);
			gtk_list_store_set (tracker_list, &iter,
					TRACKER_LIST_COL_ANNOUNCE, (*i).announce.c_str(),
					TRACKER_LIST_COL_STATUS, (*i).status.c_str(),
					TRACKER_LIST_COL_UPDATE_IN, (gint)(*i).update_in,
					TRACKER_LIST_COL_LEECHERS,(gint)(*i).leechers,
					TRACKER_LIST_COL_SEEDERS,(gint)(*i).seeders,
						  -1);
		}
		for(torrent::torrent_info::peer_info_iter i = info.seeders.begin(); i != info.seeders.end(); ++i)
		{
			gtk_list_store_append(peer_list, &iter);
			gtk_list_store_set (peer_list, &iter,
					PEER_LIST_COL_IP, (*i).ip,
					PEER_LIST_COL_DOWNLOADED, (gfloat)(*i).downloaded,
					PEER_LIST_COL_UPLOADED, (gfloat)(*i).uploaded,
					PEER_LIST_COL_DOWN_SPEED, (gfloat)(*i).downSpeed,
					PEER_LIST_COL_UP_SPEED, (gfloat)(*i).upSpeed,
					PEER_LIST_COL_AVAILABLE, (gfloat)(*i).available,
					-1);
		}
		for(torrent::torrent_info::peer_info_iter i = info.leechers.begin(); i != info.leechers.end(); ++i)
		{
			gtk_list_store_append(peer_list, &iter);
			gtk_list_store_set (peer_list, &iter,
					PEER_LIST_COL_IP, (*i).ip,
					PEER_LIST_COL_DOWNLOADED, (gfloat)(*i).downloaded,
					PEER_LIST_COL_UPLOADED, (gfloat)(*i).uploaded,
					PEER_LIST_COL_DOWN_SPEED, (gfloat)(*i).downSpeed,
					PEER_LIST_COL_UP_SPEED, (gfloat)(*i).upSpeed,
					PEER_LIST_COL_AVAILABLE, (gfloat)(*i).available,
					-1);
		}
	}
	else
	{
		gtk_list_store_clear(tracker_list);
	}
}

gboolean foreach_torrent_list (GtkTreeModel *model,
                GtkTreePath  *path,
                GtkTreeIter  *iter,
                gpointer      user_data)
{
	gchar * g_hash;
	gtk_tree_model_get (model, iter,
			TORRENT_LIST_COL_HASH, &g_hash,
			-1);

	torrent::torrent_info info;
	std::string hash = g_hash;
	bt->Torrent_info(hash, &info);
	float downloaded = info.downloaded / 1048576;
	float uploaded = info.uploaded / 1048576;
	float up_speed = info.tx_speed;
	float down_speed = info.rx_speed;
	gtk_list_store_set (torrent_list, iter,
						TORRENT_LIST_COL_NAME, info.name.c_str(),
						TORRENT_LIST_COL_PROGRESS, (gint)info.progress,
						TORRENT_LIST_COL_DOWNLOADED, (gfloat)downloaded,
						TORRENT_LIST_COL_UPLOADED, (gfloat)uploaded,
						TORRENT_LIST_COL_DOWN_SPEED, (gfloat)down_speed,
						TORRENT_LIST_COL_UP_SPEED, (gfloat)up_speed,
					  -1);

	//g_free(tree_path_str);
	g_free(g_hash);
	return FALSE;
}

gboolean foreach_tracker_list (GtkTreeModel *model,
                GtkTreePath  *path,
                GtkTreeIter  *iter,
                gpointer      user_data)
{
	torrent::torrent_info * info = (torrent::torrent_info*)user_data;
	gchar * g_announce;
	gtk_tree_model_get (model, iter,TRACKER_LIST_COL_ANNOUNCE, &g_announce,-1);
	std::string announce = g_announce;
	g_free(g_announce);
	for(torrent::torrent_info::tracker_info_iter i = info->trackers.begin(); i != info->trackers.end(); ++i)
	{
		if (announce == (*i).announce)
		{
			gtk_list_store_set (tracker_list, iter,
					TRACKER_LIST_COL_STATUS, (*i).status.c_str(),
					TRACKER_LIST_COL_UPDATE_IN, (gint)(*i).update_in,
					TRACKER_LIST_COL_LEECHERS,(gint)(*i).leechers,
					TRACKER_LIST_COL_SEEDERS,(gint)(*i).seeders,
						  -1);
			break;
		}
	}
	return FALSE;
}

gboolean on_timer(gpointer data)
{
	//return FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(torrent_list), foreach_torrent_list, NULL);

	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       selected_iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
	if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
	{
		gchar *g_hash;
		gtk_tree_model_get (model, &selected_iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
		std::string hash = g_hash;
		g_free(g_hash);
		torrent::torrent_info info;
		bt->Torrent_info(hash, &info);

		gtk_tree_model_foreach(GTK_TREE_MODEL(tracker_list), foreach_tracker_list, (gpointer)&info);

		gtk_list_store_clear(peer_list);
		for(torrent::torrent_info::peer_info_iter i = info.seeders.begin(); i != info.seeders.end(); ++i)
		{
			GtkTreeIter iter;
			gtk_list_store_append(peer_list, &iter);
			gtk_list_store_set (peer_list, &iter,
					PEER_LIST_COL_IP, (*i).ip,
					PEER_LIST_COL_DOWNLOADED, (gfloat)(*i).downloaded,
					PEER_LIST_COL_UPLOADED, (gfloat)(*i).uploaded,
					PEER_LIST_COL_DOWN_SPEED, (gfloat)(*i).downSpeed,
					PEER_LIST_COL_UP_SPEED, (gfloat)(*i).upSpeed,
					PEER_LIST_COL_AVAILABLE, (gfloat)(*i).available,
					-1);
		}
		for(torrent::torrent_info::peer_info_iter i = info.leechers.begin(); i != info.leechers.end(); ++i)
		{
			GtkTreeIter iter;
			gtk_list_store_append(peer_list, &iter);
			gtk_list_store_set (peer_list, &iter,
					PEER_LIST_COL_IP, (*i).ip,
					PEER_LIST_COL_DOWNLOADED, (gfloat)(*i).downloaded,
					PEER_LIST_COL_UPLOADED, (gfloat)(*i).uploaded,
					PEER_LIST_COL_DOWN_SPEED, (gfloat)(*i).downSpeed,
					PEER_LIST_COL_UP_SPEED, (gfloat)(*i).upSpeed,
					PEER_LIST_COL_AVAILABLE, (gfloat)(*i).available,
					-1);
		}
	}
	else
	{
		gtk_list_store_clear(tracker_list);
		gtk_list_store_clear(peer_list);
	}
	return TRUE;
}



/* создание окна в этот раз мы вынесли в отдельную функцию */
void init_gui()
{
	try
	{
		bt = new bittorrent::Bittorrent();
	}
	catch(Exception e)
	{
		e.print();
		return;
	}
	catch(...)
	{
		std::cout<<"Undefined behavior\n";
		return;
	}
	gtk_init (NULL, NULL);
	GtkBuilder *builder;
	GError* error = NULL;

	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, "gui.glade", &error))
	{
			/* загрузить файл не удалось */
			g_critical ("Не могу загрузить файл: %s", error->message);
			g_error_free (error);
	}

	gtk_builder_connect_signals(builder, NULL);

	window = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
	if (!window)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}

	torrent_list = GTK_LIST_STORE (gtk_builder_get_object (builder, "torrent_list"));
	if (!torrent_list)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	torrent_view = GTK_TREE_VIEW (gtk_builder_get_object (builder, "torrent_view"));
	if (!torrent_view)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	tracker_list = GTK_LIST_STORE (gtk_builder_get_object (builder, "tracker_list"));
	if (!tracker_list)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	peer_list = GTK_LIST_STORE (gtk_builder_get_object (builder, "peer_list"));
	if (!peer_list)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	label_hash = GTK_LABEL(gtk_builder_get_object (builder, "label_hash"));
	if (!label_hash)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	label_length = GTK_LABEL(gtk_builder_get_object (builder, "label_length"));
	if (!label_length)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	label_dir = GTK_LABEL(gtk_builder_get_object (builder, "label_dir"));
	if (!label_dir)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	label_piece_number = GTK_LABEL(gtk_builder_get_object (builder, "label_piece_number"));
	if (!label_piece_number)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	label_piece_length = GTK_LABEL(gtk_builder_get_object (builder, "label_piece_length"));
	if (!label_piece_length)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	g_object_unref (builder);

	g_timeout_add_seconds(1,on_timer,NULL);

	gtk_widget_show_all (window);
	open_dialog = NULL;
			/* передаём управление GTK+ */

	gtk_main ();
}

void show_open_dialog()
{
	GtkBuilder *builder;
	GError* error = NULL;
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, "open_dialog.glade", &error))
	{
			/* загрузить файл не удалось */
			g_critical ("Не могу загрузить файл: %s", error->message);
			g_error_free (error);
	}

	gtk_builder_connect_signals(builder, NULL);

	open_dialog = GTK_DIALOG (gtk_builder_get_object (builder, "open_dialog"));
	if (!open_dialog)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}

	GtkListStore* file_list =GTK_LIST_STORE (gtk_builder_get_object (builder, "file_list"));
	if (!file_list)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	GtkLabel* label_hash = GTK_LABEL(gtk_builder_get_object (builder, "label_hash"));
	if (!label_hash)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	GtkLabel* label_length = GTK_LABEL(gtk_builder_get_object (builder, "label_length"));
	if (!label_length)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}


	GtkLabel* label_piece_number = GTK_LABEL(gtk_builder_get_object (builder, "label_piece_number"));
	if (!label_piece_number)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	GtkLabel* label_piece_length = GTK_LABEL(gtk_builder_get_object (builder, "label_piece_length"));
	if (!label_piece_length)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	open_dialog_dir_chooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object (builder, "filechooserwidget1"));
	if (!open_dialog_dir_chooser)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}





	g_object_unref (builder);

	gtk_label_set_text(label_hash, metafile->info_hash_hex);
	char chars[100];
	sprintf(chars, "%llu", metafile->length);
	gtk_label_set_text(label_length, chars);

	sprintf(chars, "%llu", metafile->piece_length);
	gtk_label_set_text(label_piece_length, chars);

	sprintf(chars, "%u", metafile->piece_count);
	gtk_label_set_text(label_piece_number, chars);


	GtkTreeIter   iter;
	//for(torrent::torrent_info::file_list_iter i = info.file_list_.begin(); i != info.file_list_.end(); ++i)
	for(size_t i = 0; i < metafile->files.size(); i++)
	{
		gtk_list_store_append(file_list, &iter);
		gtk_list_store_set (file_list, &iter,
							0, metafile->files[i].name.c_str(),
							1, (guint64)metafile->files[i].length,
							-1);
	}
	gtk_widget_show_all (GTK_WIDGET(open_dialog));

}

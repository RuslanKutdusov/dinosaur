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
GtkTreeView* file_view;
GtkRadioButton* high_button;
GtkRadioButton* normal_button;
GtkRadioButton* low_button;
GtkListStore* torrent_list;
GtkListStore* tracker_list;
GtkListStore* peer_list;
GtkListStore* dp_list;
GtkTreeStore* file_tree;
GtkLabel* label_hash;
GtkLabel* label_length;
GtkLabel* label_dir;
GtkLabel* label_piece_number;
GtkLabel* label_piece_length;
GtkStatusbar * statusbar;
dinosaur::DinosaurPtr bt;
dinosaur::torrent::MetafilePtr metafile;

enum
{
	TORRENT_LIST_COL_NAME,
	TORRENT_LIST_COL_PROGRESS,
	TORRENT_LIST_COL_DOWNLOADED,
	TORRENT_LIST_COL_UPLOADED,
	TORRENT_LIST_COL_DOWN_SPEED,
	TORRENT_LIST_COL_UP_SPEED,
	TORRENT_LIST_COL_WORK,
	TORRENT_LIST_COL_SEEDS,
	TORRENT_LIST_COL_LEECHS,
	TORRENT_LIST_COL_START_TIME,
	TORRENT_LIST_COL_REMAIN_TIME,
	TORRENT_LIST_COL_RATIO,
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
	PEER_LIST_COL_BLOCK2REQUEST,
	PEER_LIST_COL_REQUESTED_BLOCKS,
	PEER_LIST_COLS
};

enum
{
	DP_LIST_COL_INDEX,
	DP_LIST_COL_BLOCK2DOWNLOAD,
	DP_LIST_COL_DOWNLOADED_BLOCKS,
	DP_LIST_COL_PRIO,
	DP_LIST_COLS
};

enum
{
	FILE_TREE_PATH,
	FILE_TREE_LENGTH,
	FILE_TREE_PROGRESS,
	FILE_TREE_PRIORITY,
	FILE_TREE_INDEX,
	FILE_LIST_COLS
};

void int_bytes2str(uint64_t i, char * str, bool speed = false)
{
	char c[3];
	strncpy(c, speed ? "/s\0" : "\0", 3);
	if (i < 1024)
	{
		sprintf(str, "%llu B%s", i,c);
		return;
	}
	if (i < 1048576)
	{
		float kb = i /  1024.0f;
		sprintf(str, "%.2f KB%s", kb, c);
		return;
	}
	if (i < 1073741824)
	{
		float mb = i / 1048576.0f;
		sprintf(str, "%.2f MB%s", mb, c);
		return;
	}
	float gb = i / 1073741824.0f;
	sprintf(str, "%.2f GB%s", gb, c);
}

void priority(dinosaur::DOWNLOAD_PRIORITY prio, std::string & str_prio)
{
	switch(prio)
	{
	case(dinosaur::DOWNLOAD_PRIORITY_LOW):
	str_prio = "Low";
			break;
	case(dinosaur::DOWNLOAD_PRIORITY_NORMAL):
	str_prio = "Normal";
			break;
	case(dinosaur::DOWNLOAD_PRIORITY_HIGH):
	str_prio = "High";
			break;
	}
}

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

void messagebox(const std::string & message)
{
	messagebox(message.c_str());
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
			metafile.reset(new dinosaur::torrent::Metafile(filename));
			gtk_widget_destroy (dialog);
			show_open_dialog();
		}
		catch(dinosaur::Exception & e)
		{
			gtk_widget_destroy (dialog);
			messagebox(exception_errcode2str(e));
		}
		catch(dinosaur::SyscallException & e)
		{
			gtk_widget_destroy (dialog);
			messagebox(e.get_errno_str());
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
		try
		{
			bt->ContinueTorrent(hash);
		}
		catch (dinosaur::Exception & e) {
			messagebox(dinosaur::exception_errcode2str(e));
		}
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
		try
		{
			bt->PauseTorrent(hash);
		}
		catch (dinosaur::Exception & e) {
			messagebox(dinosaur::exception_errcode2str(e));
		}
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
		try
		{
			bt->DeleteTorrent(hash, false);
			gtk_list_store_remove(torrent_list, &selected_iter);
		}
		catch (dinosaur::Exception & e) {
			printf("Exception\n");
			messagebox(dinosaur::exception_errcode2str(e));
		}
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
		try
		{
			bt->CheckTorrent(hash);
		}
		catch (dinosaur::Exception & e) {
			messagebox(dinosaur::exception_errcode2str(e));
		}
	}
}

extern "C" void on_button_cfg_clicked(GtkWidget *object, gpointer user_data)
{
	if (!cfg_opened())
		show_cfg_dialog();
}

extern "C" void on_window1_destroy (GtkWidget *object, gpointer user_data)
{
	dinosaur::Dinosaur::DeleteDinosaur(bt);
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
	try
	{
		bt->AddTorrent(*metafile, dir_str, hash);
		g_free(dir);
		gtk_object_destroy(GTK_OBJECT(open_dialog));
		open_dialog = NULL;
		metafile.reset();
		on_window1_show(NULL,NULL);
		return;
	}
	catch (dinosaur::Exception & e)
	{
		messagebox(dinosaur::exception_errcode2str(e));
	}
	catch (dinosaur::SyscallException & e)
	{
		messagebox(e.get_errno_str());
	}
	gtk_object_destroy(GTK_OBJECT(open_dialog));
	open_dialog = NULL;
	metafile.reset();
	return;
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
	bt->get_TorrentList(torrents);
	if (torrents.empty())
		return;
	try
	{
		for(std::list<std::string>::iterator i = torrents.begin(); i != torrents.end(); ++i)
		{
			std::string hash = *i;
			gtk_list_store_append(torrent_list, &iter);
			dinosaur::info::torrent_dyn dyn;
			dinosaur::info::torrent_stat stat;
			bt->get_torrent_info_dyn(hash, dyn);
			bt->get_torrent_info_stat(hash, stat);


			char downloaded[256];
			int_bytes2str(dyn.downloaded, downloaded);

			char uploaded[256];
			int_bytes2str(dyn.uploaded, uploaded);

			char rx_speed[256];
			int_bytes2str(dyn.rx_speed, rx_speed, true);

			char tx_speed[256];
			int_bytes2str(dyn.tx_speed, tx_speed, true);

			std::string work;
			switch(dyn.work)
			{
			case(dinosaur::TORRENT_WORK_DOWNLOADING):
					work = "Downloading ";
					char per[10];
					sprintf(per, "%d%%", dyn.progress);
					work += per;
					break;
			case(dinosaur::TORRENT_WORK_UPLOADING):
					work = "Uploading";
					break;
			case(dinosaur::TORRENT_WORK_CHECKING):
					work = "Checking ";
					sprintf(per, "%d%%", dyn.progress);
					work += per;
					break;
			case(dinosaur::TORRENT_WORK_PAUSED):
					work = "Paused";
					break;
			case(dinosaur::TORRENT_WORK_FAILURE):
					work = "Failure";
					break;
			case(dinosaur::TORRENT_WORK_DONE):
					work = "Done";
					break;
			case(dinosaur::TORRENT_WORK_RELEASING):
					work = "Releasing";
					break;

			}

			char seeds[256];
			sprintf(seeds, "%u(%u)", dyn.seeders, dyn.total_seeders);

			char leechs[256];
			sprintf(leechs, "%u", dyn.leechers);

			char start[256];
			sprintf(start, "%d", (int)stat.start_time);

			char remain[256];
			sprintf(remain, "%d",  (int)dyn.remain_time);

			char ratio[256];
			sprintf(ratio, "%.3f", dyn.ratio);


			gtk_list_store_set (torrent_list, &iter,
								TORRENT_LIST_COL_NAME, stat.name.c_str(),
								TORRENT_LIST_COL_PROGRESS, (gint)dyn.progress,
								TORRENT_LIST_COL_DOWNLOADED, downloaded,
								TORRENT_LIST_COL_UPLOADED, uploaded,
								TORRENT_LIST_COL_DOWN_SPEED, rx_speed,
								TORRENT_LIST_COL_UP_SPEED, tx_speed,
								TORRENT_LIST_COL_WORK, work.c_str(),
								TORRENT_LIST_COL_SEEDS, seeds,
								TORRENT_LIST_COL_LEECHS, leechs,
								TORRENT_LIST_COL_START_TIME, start,
								TORRENT_LIST_COL_REMAIN_TIME, remain,
								TORRENT_LIST_COL_RATIO, ratio,
								TORRENT_LIST_COL_HASH, hash.c_str(),
							  -1);
		}
	}
	catch (dinosaur::Exception & e) {
		printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
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
		try
		{
			gchar *g_hash;
			gtk_tree_model_get (model, &iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
			std::string hash = g_hash;
			g_free(g_hash);

			gtk_list_store_clear(tracker_list);
			gtk_list_store_clear(peer_list);

			dinosaur::info::torrent_dyn dyn;
			dinosaur::info::torrent_stat stat;
			bt->get_torrent_info_dyn(hash, dyn);
			bt->get_torrent_info_stat(hash, stat);

			gtk_label_set_text(label_hash, hash.c_str());
			gtk_label_set_text(label_dir, stat.download_directory.c_str());
			char chars[100];
			sprintf(chars, "%llu", stat.length);
			gtk_label_set_text(label_length, chars);

			sprintf(chars, "%llu", stat.piece_length);
			gtk_label_set_text(label_piece_length, chars);

			sprintf(chars, "%u", stat.piece_count);
			gtk_label_set_text(label_piece_number, chars);

			dinosaur::info::trackers trackers;
			bt->get_torrent_info_trackers(hash, trackers);
			for(dinosaur::info::trackers::iterator i = trackers.begin(); i != trackers.end(); ++i)
			{
				gtk_list_store_append(tracker_list, &iter);
				gtk_list_store_set (tracker_list, &iter,
						TRACKER_LIST_COL_ANNOUNCE, (*i).announce.c_str(),
						//TRACKER_LIST_COL_STATUS, (*i).status.c_str(),
						TRACKER_LIST_COL_UPDATE_IN, (gint)(*i).update_in,
						TRACKER_LIST_COL_LEECHERS,(gint)(*i).leechers,
						TRACKER_LIST_COL_SEEDERS,(gint)(*i).seeders,
							  -1);
			}

			set_file_tree(hash, stat.files_count);
		}
		catch (dinosaur::Exception & e) {
			printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
		}
	}
	else
	{
		gtk_list_store_clear(tracker_list);
	}
}

extern "C" void on_file_view_cursor_changed(GtkWidget *object, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       selected_iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));
	if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
	{
		uint64_t index;
		gtk_tree_model_get (model, &selected_iter, FILE_TREE_INDEX, &index, -1);

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
		if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
		{
			gchar *g_hash;
			gtk_tree_model_get (model, &selected_iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
			std::string hash = g_hash;
			g_free(g_hash);

			try
			{
				dinosaur::info::file_dyn dyn;
				bt->get_torrent_info_file_dyn(hash, index, dyn);
				switch (dyn.priority) {
					case dinosaur::DOWNLOAD_PRIORITY_HIGH:
						if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(high_button)))
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(high_button), true);
						break;
					case dinosaur::DOWNLOAD_PRIORITY_NORMAL:
						if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(normal_button)))
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(normal_button), true);
						break;
					case dinosaur::DOWNLOAD_PRIORITY_LOW:
						if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(low_button)))
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(low_button), true);
						break;
					default:
						break;
				}
			}
			catch (dinosaur::Exception & e) {
				messagebox(dinosaur::exception_errcode2str(e));
			}
		}

	}
}

void change_file_priority(dinosaur::DOWNLOAD_PRIORITY prio)
{
	printf("change\n");
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       selected_iter;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));
	if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
	{
		uint64_t index;
		gtk_tree_model_get (model, &selected_iter, FILE_TREE_INDEX, &index, -1);

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrent_view));
		if (gtk_tree_selection_get_selected(selection, &model, &selected_iter))
		{
			gchar *g_hash;
			gtk_tree_model_get (model, &selected_iter, TORRENT_LIST_COL_HASH, &g_hash, -1);
			std::string hash = g_hash;
			g_free(g_hash);

			try
			{
				bt->set_file_priority(hash, index, prio);
			}
			catch (dinosaur::Exception & e) {
				messagebox(dinosaur::exception_errcode2str(e));
			}
		}

	}
}

extern "C" void on_radiobutton1_toggled (GtkRadioButton *but,  gpointer        user_data)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(but)))
		return;
	change_file_priority(dinosaur::DOWNLOAD_PRIORITY_HIGH);
}

extern "C" void on_radiobutton2_toggled (GtkRadioButton *but,  gpointer        user_data)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(but)))
		return;
	change_file_priority(dinosaur::DOWNLOAD_PRIORITY_NORMAL);
}

extern "C" void on_radiobutton3_toggled (GtkRadioButton *but,  gpointer        user_data)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(but)))
		return;
	change_file_priority(dinosaur::DOWNLOAD_PRIORITY_LOW);
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

	std::string hash = g_hash;
	try
	{

		dinosaur::info::torrent_dyn  dyn;
		bt->get_torrent_info_dyn(hash, dyn);

		char downloaded[256];
		int_bytes2str(dyn.downloaded, downloaded);

		char uploaded[256];
		int_bytes2str(dyn.uploaded, uploaded);

		char rx_speed[256];
		int_bytes2str(dyn.rx_speed, rx_speed, true);

		char tx_speed[256];
		int_bytes2str(dyn.tx_speed, tx_speed, true);

		std::string work;
		switch(dyn.work)
		{
		case(dinosaur::TORRENT_WORK_DOWNLOADING):
				work = "Downloading ";
				char per[10];
				sprintf(per, "%d%%", dyn.progress);
				work += per;
				break;
		case(dinosaur::TORRENT_WORK_UPLOADING):
				work = "Uploading";
				break;
		case(dinosaur::TORRENT_WORK_CHECKING):
				work = "Checking ";
				sprintf(per, "%d%%", dyn.progress);
				work += per;
				break;
		case(dinosaur::TORRENT_WORK_PAUSED):
				work = "Paused";
				break;
		case(dinosaur::TORRENT_WORK_FAILURE):
				work = "Failure";
				break;
		case(dinosaur::TORRENT_WORK_DONE):
				work = "Done";
				break;
		case(dinosaur::TORRENT_WORK_RELEASING):
				work = "Releasing";
				break;
		}



		char seeds[256];
		memset(seeds, 0, 256);
		sprintf(seeds, "%u(%u)", dyn.seeders, dyn.total_seeders);

		char leechs[256];
		sprintf(leechs, "%u", dyn.leechers);

		char remain[256];
		sprintf(remain, "%d",  (int)dyn.remain_time);

		char ratio[256];
		sprintf(ratio, "%.3f", dyn.ratio);

		gtk_list_store_set (torrent_list, iter,
							//TORRENT_LIST_COL_NAME, stat.name.c_str(),
							TORRENT_LIST_COL_PROGRESS, (gint)dyn.progress,
							TORRENT_LIST_COL_DOWNLOADED, downloaded,
							TORRENT_LIST_COL_UPLOADED, uploaded,
							TORRENT_LIST_COL_DOWN_SPEED, rx_speed,
							TORRENT_LIST_COL_UP_SPEED, tx_speed,
							TORRENT_LIST_COL_WORK, work.c_str(),
							TORRENT_LIST_COL_SEEDS, seeds,
							TORRENT_LIST_COL_LEECHS, leechs,
							TORRENT_LIST_COL_REMAIN_TIME, remain,
							TORRENT_LIST_COL_RATIO, ratio,
						  -1);
	}
	catch (dinosaur::Exception & e) {
		printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
	}

	g_free(g_hash);
	return FALSE;
}

gboolean foreach_tracker_list (GtkTreeModel *model,
                GtkTreePath  *path,
                GtkTreeIter  *iter,
                gpointer      user_data)
{
	dinosaur::info::trackers trackers;
	std::string hash = (char*)user_data;
	gchar * g_announce;
	gtk_tree_model_get (model, iter,TRACKER_LIST_COL_ANNOUNCE, &g_announce,-1);
	std::string announce = g_announce;
	g_free(g_announce);

	try
	{
		bt->get_torrent_info_trackers(hash, trackers);
		for(dinosaur::info::trackers::iterator i = trackers.begin(); i != trackers.end(); ++i)
		{
			if (announce == (*i).announce)
			{
				std::string status;
				switch ((*i).status) {
					case dinosaur::TRACKER_STATUS_OK:
						status = "OK";
						break;
					case dinosaur::TRACKER_STATUS_CONNECTING:
						status = "Connecting...";
						break;
					case dinosaur::TRACKER_STATUS_ERROR:
						status = "Error";
						break;
					case dinosaur::TRACKER_STATUS_INVALID_ANNOUNCE:
						status = "Invalid(unsupported) announce";
						break;
					case dinosaur::TRACKER_STATUS_SEE_FAILURE_MESSAGE:
						status = (*i).failure_mes;
						break;
					case dinosaur::TRACKER_STATUS_TIMEOUT:
						status = "Timeout";
						break;
					case dinosaur::TRACKER_STATUS_UNRESOLVED:
						status = "Can not resolve domain name";
						break;
					case dinosaur::TRACKER_STATUS_UPDATING:
						status = "Updating...";
						break;
					default:
						break;
				}
				gtk_list_store_set (tracker_list, iter,
						TRACKER_LIST_COL_STATUS, status.c_str(),
						TRACKER_LIST_COL_UPDATE_IN, (gint)(*i).update_in,
						TRACKER_LIST_COL_LEECHERS,(gint)(*i).leechers,
						TRACKER_LIST_COL_SEEDERS,(gint)(*i).seeders,
							  -1);
				break;
			}
		}
	}
	catch (dinosaur::Exception & e) {
		printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
	}
	return FALSE;
}

void set_file_tree(const std::string  & hash, uint64_t files_count)
{
	gtk_tree_store_clear(file_tree);

	try
	{
		for(dinosaur::FILE_INDEX i = 0; i < files_count; i++)
		{
			dinosaur::info::file_stat stat;
			dinosaur::info::file_dyn  dyn;

			bt->get_torrent_info_file_stat(hash, i, stat);
			bt->get_torrent_info_file_dyn(hash, i, dyn);

			char length[256];
			int_bytes2str(stat.length, length);

			int progress = stat.length == 0 ? 0 : (dyn.downloaded * 100) / stat.length;
			std::string prio;
			priority(dyn.priority, prio);

			GtkTreeIter  iter;
			gtk_tree_store_append(file_tree, &iter, NULL);


			gtk_tree_store_set(file_tree, &iter,
								FILE_TREE_PATH, stat.path.c_str(),
								FILE_TREE_LENGTH, length,
								FILE_TREE_PROGRESS, progress,
								FILE_TREE_PRIORITY, prio.c_str(),
								FILE_TREE_INDEX, i,
								-1);

		}
	}
	catch (dinosaur::Exception & e) {
		gtk_tree_store_clear(file_tree);
	}
}

gboolean foreach_file_tree (GtkTreeModel *model,
                GtkTreePath  *path,
                GtkTreeIter  *iter,
                gpointer      user_data)
{
	std::string hash = (char*)user_data;
	dinosaur::FILE_INDEX index;
	gtk_tree_model_get (model, iter, FILE_TREE_INDEX, &index,-1);
	try
	{
		dinosaur::info::file_stat stat;
		dinosaur::info::file_dyn  dyn;

		bt->get_torrent_info_file_stat(hash, index, stat);
		bt->get_torrent_info_file_dyn(hash, index, dyn);

		int progress = stat.length == 0 ? 0 : (dyn.downloaded * 100) / stat.length;
		std::string prio;
		priority(dyn.priority, prio);

		gtk_tree_store_set(file_tree, iter,
										FILE_TREE_PROGRESS, progress,
										FILE_TREE_PRIORITY, prio.c_str(),
										-1);

	}
	catch (dinosaur::Exception & e) {
		gtk_tree_store_clear(file_tree);
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

		gtk_tree_model_foreach(GTK_TREE_MODEL(tracker_list), foreach_tracker_list, g_hash);
		gtk_tree_model_foreach(GTK_TREE_MODEL(file_tree), foreach_file_tree, g_hash);

		g_free(g_hash);

		try
		{
			dinosaur::info::peers peers;
			bt->get_torrent_info_seeders(hash, peers);
			gtk_list_store_clear(peer_list);
			for(dinosaur::info::peers::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				GtkTreeIter iter;
				gtk_list_store_append(peer_list, &iter);

				char downloaded[256];
				int_bytes2str((*i).downloaded, downloaded);

				char uploaded[256];
				int_bytes2str((*i).uploaded, uploaded);

				char downSpeed[256];
				int_bytes2str((*i).downSpeed, downSpeed, true);

				char upSpeed[256];
				int_bytes2str((*i).upSpeed, upSpeed, true);

				char available[256];
				sprintf(available, "%.3f", (*i).available);

				char block2request[256];
				sprintf(block2request, "%u", (*i).blocks2request);

				char requested_blocks[256];
				sprintf(requested_blocks, "%u", (*i).requested_blocks);

				gtk_list_store_set (peer_list, &iter,
						PEER_LIST_COL_IP, (*i).ip,
						PEER_LIST_COL_DOWNLOADED, downloaded,
						PEER_LIST_COL_UPLOADED, uploaded,
						PEER_LIST_COL_DOWN_SPEED, downSpeed,
						PEER_LIST_COL_UP_SPEED, upSpeed,
						PEER_LIST_COL_AVAILABLE, available,
						PEER_LIST_COL_BLOCK2REQUEST, block2request,
						PEER_LIST_COL_REQUESTED_BLOCKS, requested_blocks,
						-1);
			}
			bt->get_torrent_info_leechers(hash, peers);
			for(dinosaur::info::peers::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				GtkTreeIter iter;
				gtk_list_store_append(peer_list, &iter);

				char downloaded[256];
				int_bytes2str((*i).downloaded, downloaded);

				char uploaded[256];
				int_bytes2str((*i).uploaded, uploaded);

				char downSpeed[256];
				int_bytes2str((*i).downSpeed, downSpeed, true);

				char upSpeed[256];
				int_bytes2str((*i).upSpeed, upSpeed, true);

				char available[256];
				sprintf(available, "%.3f", (*i).available);

				gtk_list_store_set (peer_list, &iter,
						PEER_LIST_COL_IP, (*i).ip,
						PEER_LIST_COL_DOWNLOADED, downloaded,
						PEER_LIST_COL_UPLOADED, uploaded,
						PEER_LIST_COL_DOWN_SPEED, downSpeed,
						PEER_LIST_COL_UP_SPEED, upSpeed,
						PEER_LIST_COL_AVAILABLE, available,
						-1);
			}

			dinosaur::info::downloadable_pieces dp;
			bt->get_torrent_info_downloadable_pieces(hash, dp);
			gtk_list_store_clear(dp_list);
			for(dinosaur::info::downloadable_pieces::iterator i = dp.begin(); i != dp.end(); ++i)
			{
				GtkTreeIter iter;
				gtk_list_store_append(dp_list, &iter);

				char index[256];
				sprintf(index, "%u", (*i).index);

				char block2download[256];
				sprintf(block2download, "%u", (*i).block2download);

				char downloaded_blocks[256];
				sprintf(downloaded_blocks, "%u", (*i).downloaded_blocks);

				std::string prio;
				priority((*i).priority, prio);

				gtk_list_store_set (dp_list, &iter,
									DP_LIST_COL_INDEX, index,
									DP_LIST_COL_BLOCK2DOWNLOAD, block2download,
									DP_LIST_COL_DOWNLOADED_BLOCKS, downloaded_blocks,
									DP_LIST_COL_PRIO, prio.c_str(),
									-1);
			}
		}
		catch (dinosaur::Exception & e) {
			printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
		}
	}
	else
	{
		gtk_list_store_clear(tracker_list);
		gtk_list_store_clear(peer_list);
		gtk_list_store_clear(dp_list);
		gtk_tree_store_clear(file_tree);
	}
	return TRUE;
}

void update_statusbar()
{
	//gtk_statusbar_remove_all(statusbar, 0);
	dinosaur::socket_status s= bt->get_socket_status();
	if (s.listen)
		 gtk_statusbar_push(statusbar, 0, "Socket status: ok");
	else
	{
		char chars[256];
		std::string error_mes = s.exception_errcode == dinosaur::Exception::NO_ERROR ? sys_errlist[s.errno_] : dinosaur::exception_errcode2str(s.exception_errcode);
		sprintf(chars, "Socket status: %s", error_mes.c_str());
		gtk_statusbar_push(statusbar, 0, chars);
	}
}


/* создание окна в этот раз мы вынесли в отдельную функцию */
void init_gui()
{
	try
	{
		dinosaur::torrent_failures tfs;
		dinosaur::Dinosaur::CreateDinosaur(bt, tfs);
	}
	catch(dinosaur::Exception & e)
	{
		std::cout<<dinosaur::exception_errcode2str(e);
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

	dp_list = GTK_LIST_STORE (gtk_builder_get_object (builder, "dp_list"));
	if (!dp_list)
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

	statusbar = GTK_STATUSBAR(gtk_builder_get_object (builder, "statusbar"));
	if (!statusbar)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	file_tree = GTK_TREE_STORE(gtk_builder_get_object (builder, "file_tree"));
	if (!file_tree)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	file_view = GTK_TREE_VIEW(gtk_builder_get_object (builder, "file_view"));
	if (!file_view)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	high_button = GTK_RADIO_BUTTON(gtk_builder_get_object (builder, "radiobutton1"));
	if (!high_button)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	normal_button = GTK_RADIO_BUTTON(gtk_builder_get_object (builder, "radiobutton2"));
	if (!normal_button)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	low_button = GTK_RADIO_BUTTON(gtk_builder_get_object (builder, "radiobutton3"));
	if (!low_button)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета диалога");
	}

	g_object_unref (builder);

	g_timeout_add_seconds(1,on_timer,NULL);

	update_statusbar();

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

	gtk_label_set_text(label_hash, metafile->info_hash_hex.c_str());
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
							0, metafile->files[i].path.c_str(),
							1, (guint64)metafile->files[i].length,
							-1);
	}
	gtk_widget_show_all (GTK_WIDGET(open_dialog));
}

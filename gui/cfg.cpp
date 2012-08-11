/*
 * cfg.cpp
 *
 *  Created on: 09.08.2012
 *      Author: ruslan
 */

#include "gui.h"

GtkWidget * cfg_window = NULL;
GtkFileChooserButton * download_dir;
GtkEntry * listen_port;
GtkEntry * w_cache_size;
GtkEntry * r_cache_size;
GtkEntry * def_interval;
GtkEntry * num_want;
GtkEntry * max_seeds;
GtkEntry * max_leechs;
GtkCheckButton * send_have;
GtkEntry * listen_on;
GtkEntry * max_torrents;

bool cfg_opened()
{
	return cfg_window != NULL;
}

void show_cfg_dialog()
{
	GtkBuilder *builder;
	GError* error = NULL;
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, "config.glade", &error))
	{
			/* загрузить файл не удалось */
			g_critical ("Не могу загрузить файл: %s", error->message);
			g_error_free (error);
	}

	gtk_builder_connect_signals(builder, NULL);

	cfg_window = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
	if (!cfg_window)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}

	download_dir = GTK_FILE_CHOOSER_BUTTON (gtk_builder_get_object (builder, "download_dir"));
	if (!download_dir)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	gtk_file_chooser_button_set_title(download_dir, bt->Config.get_download_directory().c_str());

	char chars[256];


	listen_port = GTK_ENTRY (gtk_builder_get_object (builder, "listen_port"));
	if (!listen_port)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%u", bt->Config.get_port());
	gtk_entry_set_text(listen_port, chars);

	w_cache_size = GTK_ENTRY (gtk_builder_get_object (builder, "w_cache_size"));
	if (!w_cache_size)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%u", bt->Config.get_write_cache_size());
	gtk_entry_set_text(w_cache_size, chars);

	r_cache_size = GTK_ENTRY (gtk_builder_get_object (builder, "r_cache_size"));
	if (!r_cache_size)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%u", bt->Config.get_read_cache_size());
	gtk_entry_set_text(r_cache_size, chars);

	def_interval = GTK_ENTRY (gtk_builder_get_object (builder, "def_interval"));
	if (!def_interval)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%llu", bt->Config.get_tracker_default_interval());
	gtk_entry_set_text(def_interval, chars);

	num_want = GTK_ENTRY (gtk_builder_get_object (builder, "num_want"));
	if (!num_want)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%u", bt->Config.get_tracker_numwant());
	gtk_entry_set_text(num_want, chars);

	max_seeds = GTK_ENTRY (gtk_builder_get_object (builder, "max_seeds"));
	if (!max_seeds)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%u", bt->Config.get_max_active_seeders());
	gtk_entry_set_text(max_seeds, chars);

	max_leechs = GTK_ENTRY (gtk_builder_get_object (builder, "max_leechs"));
	if (!max_leechs)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%u", bt->Config.get_max_active_leechers());
	gtk_entry_set_text(max_leechs, chars);

	send_have = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "send_have"));
	if (!send_have)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(send_have), bt->Config.get_send_have());

	listen_on = GTK_ENTRY (gtk_builder_get_object (builder, "listen_on"));
	if (!listen_on)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	dinosaur::IP_CHAR ip;
	bt->Config.get_listen_on(ip);
	gtk_entry_set_text(listen_on, ip);

	max_torrents = GTK_ENTRY (gtk_builder_get_object (builder, "max_torrents"));
	if (!max_torrents)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}
	sprintf(chars, "%u", bt->Config.get_max_active_torrents());
	gtk_entry_set_text(max_torrents, chars);

	GtkButton * cfg_ok = GTK_BUTTON (gtk_builder_get_object (builder, "cfg_ok"));
	if (!cfg_ok)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}

	GtkButton * cfg_cancel = GTK_BUTTON (gtk_builder_get_object (builder, "cfg_cancel"));
	if (!cfg_cancel)
	{
			/* что-то не так, наверное, ошиблись в имени */
			g_critical ("Ошибка при получении виджета окна");
	}

	g_object_unref (builder);
	gtk_widget_show_all (GTK_WIDGET(cfg_window));
}

extern "C" void on_cfg_cancel_clicked (GtkWidget *object, gpointer user_data)
{
	gtk_object_destroy(GTK_OBJECT(cfg_window));
	cfg_window = NULL;
}

extern "C" void on_cfg_ok_clicked (GtkWidget *object, gpointer user_data)
{
	try
	{
		bt->Config.set_download_directory((const char *)gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(download_dir)));
		uint16_t uint16;
		uint32_t uint32;
		uint64_t uint64;

		if (sscanf((const char *)gtk_entry_get_text(listen_port), "%hu", &uint16) == 0)
			messagebox("Invalid port");
		else
			bt->Config.set_port(uint16);

		if (sscanf((const char *)gtk_entry_get_text(w_cache_size), "%hu", &uint16) == 0)
			messagebox("Invalid write cache size");
		else
			bt->Config.set_write_cache_size(uint16);

		if (sscanf((const char *)gtk_entry_get_text(r_cache_size), "%hu", &uint16) == 0)
			messagebox("Invalid read cache size");
		else
			bt->Config.set_read_cache_size(uint16);

		if (sscanf((const char *)gtk_entry_get_text(def_interval), "%llu", &uint64) == 0)
			messagebox("Invalid tracker default interval");
		else
			bt->Config.set_tracker_default_interval(uint64);

		if (sscanf((const char *)gtk_entry_get_text(num_want), "%u", &uint32) == 0)
			messagebox("Invalid tracker numwant");
		else
			bt->Config.set_tracker_numwant(uint32);

		if (sscanf((const char *)gtk_entry_get_text(max_seeds), "%u", &uint32) == 0)
			messagebox("Invalid max active seeds");
		else
			bt->Config.set_max_active_seeders(uint32);

		if (sscanf((const char *)gtk_entry_get_text(max_leechs), "%u", &uint32) == 0)
			messagebox("Invalid max active leechs");
		else
			bt->Config.set_max_active_leechers(uint32);

		bt->Config.set_send_have((bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(send_have)));

		bt->Config.set_listen_on((char *)gtk_entry_get_text(listen_on));

		if (sscanf((const char *)gtk_entry_get_text(max_torrents), "%hu", &uint16) == 0)
			messagebox("Invalid max active torrents");
		else
			bt->Config.set_max_active_torrents(uint16);

			bt->UpdateConfigs();
	}
	catch(dinosaur::Exception & e)
	{
		messagebox(dinosaur::exception_errcode2str(e));
	}
	catch(dinosaur::SyscallException & e)
	{
		messagebox(e.get_errno_str());
	}

	gtk_object_destroy(GTK_OBJECT(cfg_window));
	cfg_window = NULL;
	update_statusbar();
}




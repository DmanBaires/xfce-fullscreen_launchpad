/*
 * Folder management functions for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 */

#include "xfce-launcher.h"
#include <string.h>

FolderInfo* create_folder(const gchar *name) {
    FolderInfo *folder = g_new0(FolderInfo, 1);
    folder->id = g_strdup_printf("folder_%" G_GINT64_FORMAT, g_get_monotonic_time());
    folder->name = g_strdup(name);
    folder->icon = g_strdup("folder");
    folder->apps = NULL;
    folder->is_open = FALSE;
    return folder;
}

void free_folder_info(FolderInfo *folder_info) {
    if (folder_info) {
        g_free(folder_info->id);
        g_free(folder_info->name);
        g_free(folder_info->icon);
        /* Note: The apps list contains pointers to AppInfo structs
         * that are owned by the main app_list, so we don't free them here */
        if (folder_info->apps)
            g_list_free(folder_info->apps);
        g_free(folder_info);
    }
}

FolderInfo* find_folder_by_id(LauncherPlugin *launcher, const gchar *folder_id) {
    GList *iter;
    for (iter = launcher->folder_list; iter != NULL; iter = g_list_next(iter)) {
        FolderInfo *folder = (FolderInfo *)iter->data;
        if (g_strcmp0(folder->id, folder_id) == 0) {
            return folder;
        }
    }
    return NULL;
}

static void rebuild_filtered_list_from_search_entry(LauncherPlugin *launcher) {
    if (!launcher) {
        return;
    }

    if (launcher->filtered_list) {
        g_list_free(launcher->filtered_list);
        launcher->filtered_list = NULL;
    }

    if (!launcher->search_entry) {
        launcher->filtered_list = launcher->app_list ? g_list_copy(launcher->app_list) : NULL;
        return;
    }

    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(launcher->search_entry));
    if (!search_text || strlen(search_text) == 0) {
        launcher->filtered_list = launcher->app_list ? g_list_copy(launcher->app_list) : NULL;
        return;
    }

    gchar *search_lower = g_utf8_strdown(search_text, -1);

    for (GList *iter = launcher->app_list; iter != NULL; iter = g_list_next(iter)) {
        AppInfo *app_info = (AppInfo *)iter->data;
        if (app_info && app_info->name && !app_info->is_hidden) {
            gchar *name_lower = g_utf8_strdown(app_info->name, -1);
            if (strstr(name_lower, search_lower) != NULL) {
                launcher->filtered_list = g_list_append(launcher->filtered_list, app_info);
            }
            g_free(name_lower);
        }
    }

    g_free(search_lower);
}

void launcher_close_folder_view(LauncherPlugin *launcher) {
    if (!launcher) {
        return;
    }

    launcher->open_folder = NULL;

    if (launcher->back_button) {
        gtk_widget_hide(launcher->back_button);
    }

    rebuild_filtered_list_from_search_entry(launcher);
    launcher->current_page = 0;
}

void launcher_delete_folder(LauncherPlugin *launcher, FolderInfo *folder) {
    if (!launcher || !folder) {
        return;
    }

    if (launcher->open_folder == folder) {
        launcher_close_folder_view(launcher);
    }

    /* Move apps out of the folder */
    while (folder->apps != NULL) {
        AppInfo *app = (AppInfo *)folder->apps->data;
        remove_app_from_folder(launcher, app);
    }

    launcher->folder_list = g_list_remove(launcher->folder_list, folder);
    free_folder_info(folder);
}

void launcher_normalize_folder(LauncherPlugin *launcher, FolderInfo *folder) {
    if (!launcher || !folder) {
        return;
    }

    gint count = g_list_length(folder->apps);

    /* 1B: auto-delete empty folders */
    if (count == 0) {
        launcher_delete_folder(launcher, folder);
        return;
    }

    /* 2B: auto-dissolve folders with 1 app */
    if (count == 1) {
        AppInfo *only = (AppInfo *)folder->apps->data;
        remove_app_from_folder(launcher, only);
        launcher_delete_folder(launcher, folder);
        return;
    }
}

void launcher_normalize_all_folders(LauncherPlugin *launcher) {
    if (!launcher) {
        return;
    }

    for (GList *iter = launcher->folder_list; iter != NULL;) {
        GList *next = iter->next;
        FolderInfo *folder = (FolderInfo *)iter->data;
        launcher_normalize_folder(launcher, folder);
        iter = next;
    }
}

void add_app_to_folder(LauncherPlugin *launcher, AppInfo *app, const gchar *folder_id) {
    FolderInfo *folder = find_folder_by_id(launcher, folder_id);
    if (folder && app) {
        /* No-op if already in that folder */
        if (app->folder_id && g_strcmp0(app->folder_id, folder_id) == 0) {
            return;
        }

        /* Remove from any existing folder */
        FolderInfo *old_folder = NULL;
        if (app->folder_id) {
            old_folder = find_folder_by_id(launcher, app->folder_id);
            if (old_folder) {
                old_folder->apps = g_list_remove(old_folder->apps, app);
            }
            g_free(app->folder_id);
            app->folder_id = NULL;
        }

        /* Add to new folder */
        app->folder_id = g_strdup(folder_id);
        folder->apps = g_list_append(folder->apps, app);

        /* Apply auto-delete/auto-dissolve to the old folder after removal */
        if (old_folder) {
            launcher_normalize_folder(launcher, old_folder);
        }
    }
}

void remove_app_from_folder(LauncherPlugin *launcher, AppInfo *app) {
    if (app && app->folder_id) {
        FolderInfo *folder = find_folder_by_id(launcher, app->folder_id);
        if (folder) {
            folder->apps = g_list_remove(folder->apps, app);
        }
        g_free(app->folder_id);
        app->folder_id = NULL;
    }
}

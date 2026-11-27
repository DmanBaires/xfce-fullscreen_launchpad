/*
 * Configuration management for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 */

#include "xfce-launcher.h"

#include <stdlib.h>
#include <string.h>

gchar* get_config_file_path(void) {
    return g_build_filename(g_get_user_config_dir(), "xfce4", "launcher", "config.xml", NULL);
}

static gchar *xml_escape_attr(const gchar *s) {
    if (!s) {
        return g_strdup("");
    }

    return g_markup_escape_text(s, -1);
}

void save_configuration(LauncherPlugin *launcher) {
    gchar *config_path = get_config_file_path();
    gchar *config_dir = g_path_get_dirname(config_path);
    GString *xml;
    GList *iter;
    
    /* Create config directory if it doesn't exist */
    g_mkdir_with_parents(config_dir, 0700);
    
    /* Build XML content */
    xml = g_string_new("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    g_string_append(xml, "<launcher-config>\n");
    
    /* Save folders */
    g_string_append(xml, "  <folders>\n");
    for (iter = launcher->folder_list; iter != NULL; iter = g_list_next(iter)) {
        FolderInfo *folder = (FolderInfo *)iter->data;

        gchar *id = xml_escape_attr(folder ? folder->id : NULL);
        gchar *name = xml_escape_attr(folder ? folder->name : NULL);
        gchar *icon = xml_escape_attr((folder && folder->icon) ? folder->icon : "folder");

        g_string_append_printf(xml,
                               "    <folder id=\"%s\" name=\"%s\" icon=\"%s\"/>\n",
                               id,
                               name,
                               icon);

        g_free(id);
        g_free(name);
        g_free(icon);
    }
    g_string_append(xml, "  </folders>\n");
    
    /* Save app states */
    g_string_append(xml, "  <apps>\n");
    for (iter = launcher->app_list; iter != NULL; iter = g_list_next(iter)) {
        AppInfo *app = (AppInfo *)iter->data;
        if (!app) {
            continue;
        }

        if (app->is_hidden || app->folder_id || app->position != -1) {
            gchar *name = xml_escape_attr(app->name);

            g_string_append_printf(xml,
                                   "    <app name=\"%s\" hidden=\"%s\" position=\"%d\"",
                                   name,
                                   app->is_hidden ? "true" : "false",
                                   app->position);

            g_free(name);

            if (app->folder_id) {
                gchar *folder = xml_escape_attr(app->folder_id);
                g_string_append_printf(xml, " folder=\"%s\"", folder);
                g_free(folder);
            }
            g_string_append(xml, "/>\n");
        }
    }
    g_string_append(xml, "  </apps>\n");
    g_string_append(xml, "</launcher-config>\n");
    
    /* Write to file */
    g_file_set_contents(config_path, xml->str, xml->len, NULL);
    
    g_string_free(xml, TRUE);
    g_free(config_dir);
    g_free(config_path);
}

/* User data for GMarkup parser */
typedef struct {
    LauncherPlugin *launcher;
    gboolean in_folders;
    gboolean in_apps;
} ParserData;

/* GMarkup parser callbacks */
static void start_element(GMarkupParseContext *context,
                        const gchar *element_name,
                        const gchar **attribute_names,
                        const gchar **attribute_values,
                        gpointer user_data,
                        GError **error) {
    (void)context;
    (void)error;

    ParserData *data = (ParserData *)user_data;

    if (strcmp(element_name, "folders") == 0) {
        data->in_folders = TRUE;
    } else if (strcmp(element_name, "apps") == 0) {
        data->in_apps = TRUE;
    } else if (strcmp(element_name, "folder") == 0 && data->in_folders) {
        const gchar *id = NULL, *name = NULL, *icon = NULL;
        for (int i = 0; attribute_names && attribute_names[i]; i++) {
            if (strcmp(attribute_names[i], "id") == 0) id = attribute_values[i];
            if (strcmp(attribute_names[i], "name") == 0) name = attribute_values[i];
            if (strcmp(attribute_names[i], "icon") == 0) icon = attribute_values[i];
        }
        if (id && name) {
            FolderInfo *folder = create_folder(name);
            g_free(folder->id);
            folder->id = g_strdup(id);
            if (icon) {
                g_free(folder->icon);
                folder->icon = g_strdup(icon);
            }
            data->launcher->folder_list = g_list_append(data->launcher->folder_list, folder);
        }
    } else if (strcmp(element_name, "app") == 0 && data->in_apps) {
        const gchar *name = NULL, *hidden = NULL, *folder = NULL, *position = NULL;
        for (int i = 0; attribute_names && attribute_names[i]; i++) {
            if (strcmp(attribute_names[i], "name") == 0) name = attribute_values[i];
            if (strcmp(attribute_names[i], "hidden") == 0) hidden = attribute_values[i];
            if (strcmp(attribute_names[i], "folder") == 0) folder = attribute_values[i];
            if (strcmp(attribute_names[i], "position") == 0) position = attribute_values[i];
        }

        if (name) {
            GList *iter;
            for (iter = data->launcher->app_list; iter != NULL; iter = g_list_next(iter)) {
                AppInfo *app = (AppInfo *)iter->data;
                if (strcmp(app->name, name) == 0) {
                    if (hidden && strcmp(hidden, "true") == 0) app->is_hidden = TRUE;
                    if (folder) {
                        g_free(app->folder_id);
                        app->folder_id = g_strdup(folder);
                    }
                    if (position) app->position = atoi(position);
                    break;
                }
            }
        }
    }
}

static void end_element(GMarkupParseContext *context,
                      const gchar *element_name,
                      gpointer user_data,
                      GError **error) {
    (void)context;
    (void)error;

    ParserData *data = (ParserData *)user_data;
    if (strcmp(element_name, "folders") == 0) {
        data->in_folders = FALSE;
    } else if (strcmp(element_name, "apps") == 0) {
        data->in_apps = FALSE;
    }
}

static gint sort_apps_by_position(gconstpointer a, gconstpointer b) {
    const AppInfo *app_a = *(const AppInfo **)a;
    const AppInfo *app_b = *(const AppInfo **)b;

    if (app_a->position == -1 && app_b->position == -1) {
        return g_utf8_collate(app_a->name, app_b->name);
    }
    if (app_a->position == -1) return 1;
    if (app_b->position == -1) return -1;
    return app_a->position - app_b->position;
}


static void rebuild_folder_app_links(LauncherPlugin *launcher) {
    if (!launcher) {
        return;
    }

    /* Clear existing folder app lists */
    for (GList *it = launcher->folder_list; it != NULL; it = it->next) {
        FolderInfo *folder = (FolderInfo *)it->data;
        if (folder && folder->apps) {
            g_list_free(folder->apps);
            folder->apps = NULL;
        }
    }

    /* Rebuild from app->folder_id; prune invalid folder references */
    for (GList *it = launcher->app_list; it != NULL; it = it->next) {
        AppInfo *app = (AppInfo *)it->data;
        if (!app || !app->folder_id) {
            continue;
        }

        FolderInfo *folder = find_folder_by_id(launcher, app->folder_id);
        if (!folder) {
            g_free(app->folder_id);
            app->folder_id = NULL;
            continue;
        }

        folder->apps = g_list_append(folder->apps, app);
    }

    /* Apply auto-delete / auto-dissolve rules (1B/2B) */
    launcher_normalize_all_folders(launcher);
}

void load_configuration(LauncherPlugin *launcher) {
    gchar *config_path = get_config_file_path();
    gchar *contents = NULL;
    gsize length;
    GError *error = NULL;

    if (!g_file_get_contents(config_path, &contents, &length, &error)) {
        if (error) {
            g_warning("Failed to read config file: %s", error->message);
            g_error_free(error);
        }
        g_free(config_path);
        return;
    }

    ParserData data = { .launcher = launcher, .in_folders = FALSE, .in_apps = FALSE };
    GMarkupParser parser = {
        .start_element = start_element,
        .end_element = end_element,
        .text = NULL,
        .passthrough = NULL,
        .error = NULL
    };

    GMarkupParseContext *context = g_markup_parse_context_new(&parser, 0, &data, NULL);
    if (!g_markup_parse_context_parse(context, contents, length, &error)) {
        if (error) {
            g_warning("Failed to parse config file: %s", error->message);
            g_error_free(error);
        }
    }

    g_markup_parse_context_free(context);
    g_free(contents);
    g_free(config_path);

    rebuild_folder_app_links(launcher);

    launcher->app_list = g_list_sort(launcher->app_list, (GCompareFunc)sort_apps_by_position);
}

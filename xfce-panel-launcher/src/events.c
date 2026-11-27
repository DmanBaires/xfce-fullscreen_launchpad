/*
 * Event handlers for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 */

#include "xfce-launcher.h"
#include <string.h>
#include <math.h>

static gboolean selection_data_get_app_ptr(const GtkSelectionData *data, AppInfo **out_app) {
    const guchar *payload;
    gint len;

    if (!out_app) {
        return FALSE;
    }
    *out_app = NULL;

    if (!data) {
        return FALSE;
    }

    payload = gtk_selection_data_get_data(data);
    len = gtk_selection_data_get_length(data);

    if (!payload || len < (gint)sizeof(gsize)) {
        return FALSE;
    }

    gsize ptr_value = 0;
    memcpy(&ptr_value, payload, sizeof(ptr_value));
    *out_app = (AppInfo *)ptr_value;

    return *out_app != NULL;
}

static GtkWidget *find_grid_child_at_pos(GtkWidget *grid, gint x, gint y) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(grid));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *child = GTK_WIDGET(iter->data);
        GtkAllocation a;

        if (!gtk_widget_get_visible(child)) {
            continue;
        }

        gtk_widget_get_allocation(child, &a);
        if (x >= a.x && x < a.x + a.width && y >= a.y && y < a.y + a.height) {
            g_list_free(children);
            return child;
        }
    }

    g_list_free(children);
    return NULL;
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, LauncherPlugin *launcher) {
    (void)widget;

    switch (event->keyval) {
        case GDK_KEY_Escape:
            hide_overlay(launcher);
            return TRUE;
        case GDK_KEY_Right:
            if (launcher->current_page < launcher->total_pages - 1) {
                launcher->current_page++;
                populate_current_page(launcher);
                update_page_dots(launcher);
                return TRUE;
            }
            break;
        case GDK_KEY_Left:
            if (launcher->current_page > 0) {
                launcher->current_page--;
                populate_current_page(launcher);
                update_page_dots(launcher);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

void on_search_changed(GtkSearchEntry *entry, LauncherPlugin *launcher) {
    (void)entry;
    update_filtered_applications(launcher);
}

static gboolean app_matches_category(const AppInfo *app_info, const gchar *category) {
    if (!category) {
        return TRUE;
    }

    if (!app_info || !app_info->desktop_info) {
        return FALSE;
    }

    const gchar *categories = g_desktop_app_info_get_categories(app_info->desktop_info);
    if (!categories) {
        return FALSE;
    }

    gchar **tokens = g_strsplit(categories, ";", -1);
    gboolean matches = FALSE;
    for (gchar **token = tokens; *token; token++) {
        if (g_strcmp0(*token, category) == 0 ||
            (g_strcmp0(category, "AudioVideo") == 0 &&
             (g_strcmp0(*token, "Audio") == 0 || g_strcmp0(*token, "Video") == 0))) {
            matches = TRUE;
            break;
        }
    }
    g_strfreev(tokens);
    return matches;
}

void update_filtered_applications(LauncherPlugin *launcher) {
    if (!launcher) {
        return;
    }

    const gchar *search_text = launcher->search_entry
        ? gtk_entry_get_text(GTK_ENTRY(launcher->search_entry)) : "";
    if (launcher->filtered_list) {
        g_list_free(launcher->filtered_list);
    }
    launcher->filtered_list = NULL;

    if ((!search_text || !*search_text) && !launcher->selected_category) {
        launcher->filtered_list = g_list_copy(launcher->app_list);
    } else {
        gchar *search_lower = search_text && *search_text
            ? g_utf8_strdown(search_text, -1) : NULL;

        for (GList *iter = launcher->app_list; iter != NULL; iter = g_list_next(iter)) {
            AppInfo *app_info = (AppInfo *)iter->data;
            if (app_info && app_info->name && !app_info->is_hidden &&
                app_matches_category(app_info, launcher->selected_category)) {
                if (!search_lower) {
                    launcher->filtered_list = g_list_append(launcher->filtered_list, app_info);
                    continue;
                }

                gchar *name_lower = g_utf8_strdown(app_info->name, -1);
                if (strstr(name_lower, search_lower) != NULL) {
                    launcher->filtered_list = g_list_append(launcher->filtered_list, app_info);
                }
                g_free(name_lower);
            }
        }
        g_free(search_lower);
    }

    launcher->current_page = 0;
    populate_current_page(launcher);
    update_page_dots(launcher);
}

void on_category_clicked(GtkButton *button, LauncherPlugin *launcher) {
    if (!launcher) {
        return;
    }

    const gchar *category = g_object_get_data(G_OBJECT(button), "category-id");
    g_free(launcher->selected_category);
    launcher->selected_category = g_strdup(category);
    launcher->open_folder = NULL;
    gtk_widget_hide(launcher->back_button);

    GList *buttons = gtk_container_get_children(GTK_CONTAINER(launcher->category_box));
    for (GList *iter = buttons; iter; iter = iter->next) {
        GtkWidget *category_button = GTK_WIDGET(iter->data);
        const gchar *button_category = g_object_get_data(G_OBJECT(category_button), "category-id");
        gboolean is_all = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(category_button), "all-categories"));
        gboolean active = category ? g_strcmp0(category, button_category) == 0 : is_all;
        GtkStyleContext *context = gtk_widget_get_style_context(category_button);
        if (active) {
            gtk_style_context_add_class(context, "selected");
        } else {
            gtk_style_context_remove_class(context, "selected");
        }
    }
    g_list_free(buttons);

    update_filtered_applications(launcher);
}

gboolean on_background_button_press(GtkWidget *widget, GdkEventButton *event, LauncherPlugin *launcher) {
    (void)widget;

    if (event->button != GDK_BUTTON_PRIMARY) {
        return FALSE;
    }

    GtkWidget *event_widget = gtk_get_event_widget((GdkEvent *)event);
    for (GtkWidget *ancestor = event_widget; ancestor; ancestor = gtk_widget_get_parent(ancestor)) {
        if (GTK_IS_BUTTON(ancestor) || GTK_IS_ENTRY(ancestor)) {
            return FALSE;
        }
    }

    hide_overlay(launcher);
    return TRUE;
}

void on_dot_clicked(GtkWidget *dot, gpointer data) {
    (void)data;

    gint page_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dot), "page-index"));
    LauncherPlugin *launcher = g_object_get_data(G_OBJECT(dot), "launcher");
    
    if (launcher && page_index != launcher->current_page) {
        launcher->current_page = page_index;
        populate_current_page(launcher);
        update_page_dots(launcher);
    }
}

gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, LauncherPlugin *launcher) {
    (void)widget;

    gboolean changed = FALSE;
    
    switch (event->direction) {
        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_UP:
            if (launcher->current_page > 0) {
                launcher->current_page--;
                changed = TRUE;
            }
            break;
            
        case GDK_SCROLL_RIGHT:
        case GDK_SCROLL_DOWN:
            if (launcher->current_page < launcher->total_pages - 1) {
                launcher->current_page++;
                changed = TRUE;
            }
            break;
            
        case GDK_SCROLL_SMOOTH:
            if (fabs(event->delta_x) > fabs(event->delta_y)) {
                if (event->delta_x < -0.3) {
                    if (launcher->current_page > 0) {
                        launcher->current_page--;
                        changed = TRUE;
                    }
                } else if (event->delta_x > 0.3) {
                    if (launcher->current_page < launcher->total_pages - 1) {
                        launcher->current_page++;
                        changed = TRUE;
                    }
                }
            } else {
                if (event->delta_y < -0.3) {
                    if (launcher->current_page > 0) {
                        launcher->current_page--;
                        changed = TRUE;
                    }
                } else if (event->delta_y > 0.3) {
                    if (launcher->current_page < launcher->total_pages - 1) {
                        launcher->current_page++;
                        changed = TRUE;
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    if (changed) {
        populate_current_page(launcher);
        update_page_dots(launcher);
    }
    
    return TRUE;
}

void on_swipe_gesture(GtkGestureSwipe *gesture, gdouble velocity_x, gdouble velocity_y, LauncherPlugin *launcher) {
    (void)gesture;
    (void)velocity_y;

    gboolean changed = FALSE;
    
    if (velocity_x > 0 && launcher->current_page > 0) {
        launcher->current_page--;
        changed = TRUE;
    } else if (velocity_x < 0 && launcher->current_page < launcher->total_pages - 1) {
        launcher->current_page++;
        changed = TRUE;
    }
    
    if (changed) {
        populate_current_page(launcher);
        update_page_dots(launcher);
    }
}

static void on_hide_menu_activate(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;

    HideCallbackData *data = (HideCallbackData *)user_data;
    if (data && data->app_info && data->launcher) {
        hide_application(data->app_info, data->launcher);
    }
}

void on_folder_clicked(GtkWidget *button, FolderInfo *folder_info) {
    LauncherPlugin *launcher = g_object_get_data(G_OBJECT(button), "launcher");
    if (launcher) {
        launcher->open_folder = folder_info;
        g_list_free(launcher->filtered_list);
        launcher->filtered_list = g_list_copy(folder_info->apps);
        launcher->current_page = 0;
        populate_current_page(launcher);
        update_page_dots(launcher);
        gtk_widget_show(launcher->back_button);
    }
}

void on_back_button_clicked(GtkWidget *button, LauncherPlugin *launcher) {
    (void)button;

    launcher->open_folder = NULL;
    g_list_free(launcher->filtered_list);
    launcher->filtered_list = g_list_copy(launcher->app_list);
    launcher->current_page = 0;
    populate_current_page(launcher);
    update_page_dots(launcher);
    gtk_widget_hide(launcher->back_button);
}

typedef struct {
    AppInfo *app_info;
    LauncherPlugin *launcher;
} AppMenuData;

typedef struct {
    FolderInfo *folder_info;
    LauncherPlugin *launcher;
} FolderMenuData;

static void on_remove_from_folder_activate(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;

    AppMenuData *data = (AppMenuData *)user_data;
    if (!data || !data->launcher || !data->app_info || !data->app_info->folder_id) {
        return;
    }

    LauncherPlugin *launcher = data->launcher;
    AppInfo *app = data->app_info;

    FolderInfo *folder = find_folder_by_id(launcher, app->folder_id);
    remove_app_from_folder(launcher, app);

    /* Apply 1B/2B rules: delete empty folder, dissolve 1-app folder */
    if (folder) {
        launcher_normalize_folder(launcher, folder);
    }

    populate_current_page(launcher);
    update_page_dots(launcher);
    save_configuration(launcher);
}

static void on_folder_delete_activate(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;

    FolderMenuData *data = (FolderMenuData *)user_data;
    if (!data || !data->launcher || !data->folder_info) {
        return;
    }

    launcher_delete_folder(data->launcher, data->folder_info);

    populate_current_page(data->launcher);
    update_page_dots(data->launcher);
    save_configuration(data->launcher);
}

static void on_folder_rename_activate(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;

    FolderMenuData *data = (FolderMenuData *)user_data;
    if (!data || !data->launcher || !data->folder_info) {
        return;
    }

    LauncherPlugin *launcher = data->launcher;
    FolderInfo *folder = data->folder_info;

    GtkWidget *parent = launcher->overlay_window ? launcher->overlay_window : gtk_widget_get_toplevel(launcher->button);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Rename Folder",
                                                    GTK_WINDOW(parent),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_OK", GTK_RESPONSE_OK,
                                                    NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_entry_set_text(GTK_ENTRY(entry), folder->name ? folder->name : "");
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 0);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text && *text) {
            g_free(folder->name);
            folder->name = g_strdup(text);

            populate_current_page(launcher);
            update_page_dots(launcher);
            save_configuration(launcher);
        }
    }

    gtk_widget_destroy(dialog);
}

gboolean on_folder_button_press_event(GtkWidget *widget, GdkEventButton *event, FolderInfo *folder_info) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        LauncherPlugin *launcher = g_object_get_data(G_OBJECT(widget), "launcher");
        if (!launcher || !folder_info) {
            return FALSE;
        }

        GtkWidget *menu = gtk_menu_new();

        GtkWidget *rename_item = gtk_menu_item_new_with_label("Rename…");
        FolderMenuData *rename_data = g_new0(FolderMenuData, 1);
        rename_data->launcher = launcher;
        rename_data->folder_info = folder_info;
        g_signal_connect_data(rename_item, "activate",
                              G_CALLBACK(on_folder_rename_activate),
                              rename_data, (GClosureNotify)g_free, 0);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), rename_item);

        GtkWidget *delete_item = gtk_menu_item_new_with_label("Delete Folder");
        FolderMenuData *delete_data = g_new0(FolderMenuData, 1);
        delete_data->launcher = launcher;
        delete_data->folder_info = folder_info;
        g_signal_connect_data(delete_item, "activate",
                              G_CALLBACK(on_folder_delete_activate),
                              delete_data, (GClosureNotify)g_free, 0);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

        return TRUE;
    }

    return FALSE;
}

gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, AppInfo *app_info) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkWidget *menu = gtk_menu_new();
        LauncherPlugin *launcher = g_object_get_data(G_OBJECT(widget), "launcher");

        if (!launcher) {
            g_warning("Launcher reference not found in button data");
            return FALSE;
        }

        if (app_info && app_info->folder_id) {
            GtkWidget *remove_item = gtk_menu_item_new_with_label("Remove from Folder");
            AppMenuData *remove_data = g_new0(AppMenuData, 1);
            remove_data->app_info = app_info;
            remove_data->launcher = launcher;
            g_signal_connect_data(remove_item, "activate",
                                  G_CALLBACK(on_remove_from_folder_activate),
                                  remove_data, (GClosureNotify)g_free, 0);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), remove_item);

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        }

        GtkWidget *hide_item = gtk_menu_item_new_with_label("Hide");
        HideCallbackData *hide_data = g_new0(HideCallbackData, 1);
        hide_data->app_info = app_info;
        hide_data->launcher = launcher;
        g_signal_connect_data(hide_item, "activate",
                              G_CALLBACK(on_hide_menu_activate),
                              hide_data, (GClosureNotify)g_free, 0);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), hide_item);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

        return TRUE;
    }
    return FALSE;
}

/* Drag and drop handlers */
void on_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data) {
    (void)context;
    (void)user_data;

    LauncherPlugin *launcher = g_object_get_data(G_OBJECT(widget), "launcher");
    AppInfo *app_info = g_object_get_data(G_OBJECT(widget), "app-info");
    if (launcher && app_info) {
        launcher->drag_source = app_info;
    }
}

void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                          GtkSelectionData *data, guint info, guint time, LauncherPlugin *launcher) {
    (void)x;
    (void)y;
    (void)info;

    AppInfo *source_app = NULL;
    if (!selection_data_get_app_ptr(data, &source_app) || !launcher) {
        gtk_drag_finish(context, FALSE, FALSE, time);
        return;
    }

    /* Target can be an app button or a folder button */
    AppInfo *target_app = g_object_get_data(G_OBJECT(widget), "app-info");
    FolderInfo *target_folder = g_object_get_data(G_OBJECT(widget), "folder-info");

    if (target_app && target_app != source_app) {
        /* Dropped on another app -> create a folder with both */
        FolderInfo *folder = create_folder("New Folder");
        launcher->folder_list = g_list_append(launcher->folder_list, folder);

        add_app_to_folder(launcher, source_app, folder->id);
        add_app_to_folder(launcher, target_app, folder->id);

        populate_current_page(launcher);
        update_page_dots(launcher);
        save_configuration(launcher);

        gtk_drag_finish(context, TRUE, FALSE, time);
        return;
    }

    if (target_folder) {
        /* Dropped on a folder -> add to folder */
        add_app_to_folder(launcher, source_app, target_folder->id);

        populate_current_page(launcher);
        update_page_dots(launcher);
        save_configuration(launcher);

        gtk_drag_finish(context, TRUE, FALSE, time);
        return;
    }

    gtk_drag_finish(context, FALSE, FALSE, time);
}

void on_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data,
                     guint info, guint time, AppInfo *app_info) {
    (void)widget;
    (void)context;
    (void)info;
    (void)time;

    if (!app_info || !data) {
        return;
    }

    /* Transfer an in-process pointer; restricted to same app via GTK_TARGET_SAME_APP */
    gsize ptr_value = (gsize)app_info;
    gtk_selection_data_set(data,
                           gdk_atom_intern_static_string(XFCE_LAUNCHER_DND_TARGET),
                           8,
                           (const guchar *)&ptr_value,
                           (gint)sizeof(ptr_value));
}

gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, LauncherPlugin *launcher) {
    (void)widget;

    if (!launcher || !launcher->drag_source) {
        gtk_drag_finish(context, FALSE, FALSE, time);
        return FALSE;
    }

    /* For now, only allow reordering on the main view (not inside an open folder) */
    if (launcher->open_folder) {
        gtk_drag_finish(context, FALSE, FALSE, time);
        return FALSE;
    }

    GtkWidget *target_widget = find_grid_child_at_pos(launcher->app_grid, x, y);
    AppInfo *target_app = target_widget ? g_object_get_data(G_OBJECT(target_widget), "app-info") : NULL;
    FolderInfo *target_folder = target_widget ? g_object_get_data(G_OBJECT(target_widget), "folder-info") : NULL;

    gboolean success = FALSE;

    if (target_app && launcher->drag_source != target_app) {
        /* Dropped on another app -> create a folder */
        FolderInfo *folder = create_folder("New Folder");
        launcher->folder_list = g_list_append(launcher->folder_list, folder);

        add_app_to_folder(launcher, launcher->drag_source, folder->id);
        add_app_to_folder(launcher, target_app, folder->id);

        success = TRUE;
    } else if (target_folder) {
        /* Dropped on a folder -> add to folder */
        add_app_to_folder(launcher, launcher->drag_source, target_folder->id);
        success = TRUE;
    } else {
        /* Dropped on empty space -> reorder (best-effort mapping) */
        gint apps_per_page = launcher->apps_per_page > 0 ? launcher->apps_per_page : APPS_PER_PAGE;
        gint page_offset = launcher->current_page * apps_per_page;
        gint spacing = launcher->grid_spacing > 0 ? launcher->grid_spacing : 20;
        gint tile = launcher->button_size > 0 ? launcher->button_size : BUTTON_SIZE;
        gint cols = launcher->grid_columns > 0 ? launcher->grid_columns : GRID_COLUMNS;

        gint col = x / (tile + spacing);
        gint row = y / (tile + spacing);
        gint new_index = row * cols + col + page_offset;

        gint max_index = g_list_length(launcher->app_list) - 1;
        if (new_index < 0) new_index = 0;
        if (new_index > max_index) new_index = max_index;

        launcher->app_list = g_list_remove(launcher->app_list, launcher->drag_source);
        launcher->app_list = g_list_insert(launcher->app_list, launcher->drag_source, new_index);

        recalculate_positions(launcher);
        success = TRUE;
    }

    launcher->drag_source = NULL;

    if (success) {
        populate_current_page(launcher);
        update_page_dots(launcher);
        save_configuration(launcher);
        gtk_drag_finish(context, TRUE, FALSE, time);
        return TRUE;
    }

    gtk_drag_finish(context, FALSE, FALSE, time);
    return FALSE;
}

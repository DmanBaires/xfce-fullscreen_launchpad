/*
 * UI components for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki novik@axisos.org
 */

#include "xfce-launcher.h"

static GtkTargetEntry app_dnd_targets[] = {
    { (gchar *)XFCE_LAUNCHER_DND_TARGET, GTK_TARGET_SAME_APP, 0 },
};

static gchar *build_highlight_markup(const gchar *text, const gchar *query) {
    if (!text || !query || !*query) {
        return NULL;
    }

    /* Case-insensitive match with correct byte offsets in the original string */
    gchar *escaped_query = g_regex_escape_string(query, -1);
    GRegex *re = g_regex_new(escaped_query, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0, NULL);
    g_free(escaped_query);

    if (!re) {
        return NULL;
    }

    GMatchInfo *match_info = NULL;
    gboolean matched = g_regex_match(re, text, 0, &match_info);
    if (!matched || !match_info) {
        if (match_info) {
            g_match_info_free(match_info);
        }
        g_regex_unref(re);
        return NULL;
    }

    gint start = 0;
    gint end = 0;
    if (!g_match_info_fetch_pos(match_info, 0, &start, &end) || start < 0 || end <= start) {
        g_match_info_free(match_info);
        g_regex_unref(re);
        return NULL;
    }

    gchar *prefix = g_strndup(text, (gsize)start);
    gchar *match = g_strndup(text + start, (gsize)(end - start));
    gchar *suffix = g_strdup(text + end);

    gchar *prefix_esc = g_markup_escape_text(prefix, -1);
    gchar *match_esc = g_markup_escape_text(match, -1);
    gchar *suffix_esc = g_markup_escape_text(suffix, -1);

    gchar *markup = g_strdup_printf(
        "%s<span background=\"#ffffff\" background-alpha=\"20000\" foreground=\"#ffffff\" weight=\"bold\">%s</span>%s",
        prefix_esc,
        match_esc,
        suffix_esc);

    g_free(prefix);
    g_free(match);
    g_free(suffix);
    g_free(prefix_esc);
    g_free(match_esc);
    g_free(suffix_esc);

    g_match_info_free(match_info);
    g_regex_unref(re);

    return markup;
}

static GtkWidget *create_label_with_optional_highlight(LauncherPlugin *launcher, const gchar *text) {
    GtkWidget *label = gtk_label_new(NULL);

    const gchar *query = NULL;
    if (launcher && launcher->search_entry) {
        query = gtk_entry_get_text(GTK_ENTRY(launcher->search_entry));
    }

    gchar *markup = build_highlight_markup(text, query);
    if (markup) {
        gtk_label_set_markup(GTK_LABEL(label), markup);
        g_free(markup);
    } else {
        gtk_label_set_text(GTK_LABEL(label), text ? text : "");
    }

    return label;
}

static void destroy_widget_cb(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    gtk_widget_destroy(widget);
}

static gint compare_category_names(gconstpointer a, gconstpointer b) {
    return g_utf8_collate((const gchar *)a, (const gchar *)b);
}

static const gchar *category_group_for_token(const gchar *token) {
    static const gchar *categories[] = {
        "AudioVideo", "Development", "Education", "Game", "Graphics",
        "Network", "Office", "Science", "Settings", "System", "Utility",
        NULL
    };

    if (g_strcmp0(token, "Audio") == 0 || g_strcmp0(token, "Video") == 0) {
        return "AudioVideo";
    }

    for (guint i = 0; categories[i]; i++) {
        if (g_strcmp0(token, categories[i]) == 0) {
            return categories[i];
        }
    }
    return NULL;
}

static const gchar *category_label(const gchar *category) {
    static const struct {
        const gchar *id;
        const gchar *label;
    } labels[] = {
        { "AudioVideo", "Audio & Video" },
        { "Development", "Development" },
        { "Education", "Education" },
        { "Game", "Games" },
        { "Graphics", "Graphics" },
        { "Network", "Internet" },
        { "Office", "Office" },
        { "Science", "Science" },
        { "Settings", "Settings" },
        { "System", "System" },
        { "Utility", "Utilities" }
    };

    for (guint i = 0; i < G_N_ELEMENTS(labels); i++) {
        if (g_strcmp0(category, labels[i].id) == 0) {
            return labels[i].label;
        }
    }

    return category;
}

static const gchar *category_icon(const gchar *category) {
    static const struct {
        const gchar *id;
        const gchar *icon;
    } icons[] = {
        { "Audio", "audio-x-generic" },
        { "AudioVideo", "applications-multimedia" },
        { "Development", "applications-development" },
        { "Education", "applications-science" },
        { "Game", "applications-games" },
        { "Graphics", "applications-graphics" },
        { "Network", "applications-internet" },
        { "Office", "applications-office" },
        { "Science", "applications-science" },
        { "Settings", "preferences-system" },
        { "System", "applications-system" },
        { "Utility", "applications-accessories" },
        { "Video", "video-x-generic" }
    };

    for (guint i = 0; i < G_N_ELEMENTS(icons); i++) {
        if (g_strcmp0(category, icons[i].id) == 0) {
            return icons[i].icon;
        }
    }

    return "applications-other";
}

static GtkWidget *create_category_button(LauncherPlugin *launcher,
                                         const gchar *category,
                                         const gchar *label,
                                         const gchar *icon_name) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
    GtkWidget *text = gtk_label_new(label);

    gtk_image_set_pixel_size(GTK_IMAGE(icon), 24);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "category-button");
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_widget_set_halign(text, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), text, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(button), box);
    gtk_widget_set_tooltip_text(button, label);

    if (category) {
        g_object_set_data_full(G_OBJECT(button), "category-id", g_strdup(category), g_free);
    } else {
        g_object_set_data(G_OBJECT(button), "all-categories", GINT_TO_POINTER(TRUE));
    }

    if ((!category && !launcher->selected_category) ||
        (category && g_strcmp0(category, launcher->selected_category) == 0)) {
        gtk_style_context_add_class(gtk_widget_get_style_context(button), "selected");
    }

    g_signal_connect(button, "clicked", G_CALLBACK(on_category_clicked), launcher);
    gtk_box_pack_start(GTK_BOX(launcher->category_box), button, FALSE, FALSE, 0);
    return button;
}

void update_category_sidebar(LauncherPlugin *launcher) {
    if (!launcher || !launcher->category_box) {
        return;
    }

    gtk_container_foreach(GTK_CONTAINER(launcher->category_box), destroy_widget_cb, NULL);
    create_category_button(launcher, NULL, "All Applications", "view-grid-symbolic");

    GHashTable *categories = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    for (GList *iter = launcher->app_list; iter; iter = iter->next) {
        AppInfo *app = (AppInfo *)iter->data;
        const gchar *desktop_categories = app && !app->is_hidden && app->desktop_info
            ? g_desktop_app_info_get_categories(app->desktop_info) : NULL;
        if (!desktop_categories) {
            continue;
        }

        gchar **tokens = g_strsplit(desktop_categories, ";", -1);
        for (gchar **token = tokens; *token; token++) {
            const gchar *group = category_group_for_token(*token);
            if (group) {
                g_hash_table_add(categories, g_strdup(group));
            }
        }
        g_strfreev(tokens);
    }

    GList *category_names = g_hash_table_get_keys(categories);
    category_names = g_list_sort(category_names, compare_category_names);
    for (GList *iter = category_names; iter; iter = iter->next) {
        const gchar *category = (const gchar *)iter->data;
        create_category_button(launcher, category, category_label(category), category_icon(category));
    }
    g_list_free(category_names);
    g_hash_table_destroy(categories);

    gtk_widget_show_all(launcher->category_box);
}

static gint clamp_int(gint v, gint lo, gint hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static gboolean app_visible_on_main_view(const LauncherPlugin *launcher, const AppInfo *app) {
    if (!app || !app->name) {
        return FALSE;
    }

    if (app->is_hidden) {
        return FALSE;
    }

    /* Apps in folders are hidden on the main view */
    if (app->folder_id && !launcher->selected_category) {
        return FALSE;
    }

    return TRUE;
}

static gboolean app_visible_in_folder(const AppInfo *app) {
    if (!app || !app->name) {
        return FALSE;
    }

    if (app->is_hidden) {
        return FALSE;
    }

    return TRUE;
}

static GtkWidget *create_folder_preview_widget(LauncherPlugin *launcher, const FolderInfo *folder_info) {
    /* Fallback to a regular folder icon when empty */
    if (!folder_info || !folder_info->apps) {
        GtkWidget *icon = gtk_image_new_from_icon_name(folder_info && folder_info->icon ? folder_info->icon : "folder",
                                                       GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(icon), launcher->icon_size);
        return icon;
    }

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 2);

    gint mini = launcher->icon_size / 2;
    if (mini < 16) {
        mini = 16;
    }

    gint idx = 0;
    for (GList *it = folder_info->apps; it != NULL && idx < 4; it = it->next) {
        AppInfo *app = (AppInfo *)it->data;
        if (!app_visible_in_folder(app)) {
            continue;
        }

        const gchar *icon_name = app->icon ? app->icon : "application-x-executable";
        GtkWidget *img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(img), mini);

        gint row = idx / 2;
        gint col = idx % 2;
        gtk_grid_attach(GTK_GRID(grid), img, col, row, 1, 1);

        idx++;
    }

    /* If all entries were filtered out (e.g. hidden), still show folder icon */
    if (idx == 0) {
        gtk_widget_destroy(grid);
        GtkWidget *icon = gtk_image_new_from_icon_name(folder_info->icon ? folder_info->icon : "folder",
                                                       GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(icon), launcher->icon_size);
        return icon;
    }

    return grid;
}

void launcher_update_layout_for_screen(LauncherPlugin *launcher) {
    if (!launcher || !launcher->overlay_window || !launcher->app_grid) {
        return;
    }

    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(launcher->overlay_window));
    GdkDisplay *display = gdk_screen_get_display(screen);
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    if (!monitor && gdk_display_get_n_monitors(display) > 0) {
        monitor = gdk_display_get_monitor(display, 0);
    }

    GdkRectangle geom = { 0 };
    if (monitor) {
        gdk_monitor_get_geometry(monitor, &geom);
    } else {
        /* Extremely defensive fallback */
        geom.width = 1920;
        geom.height = 1080;
    }

    gint sidebar_width = clamp_int(geom.width / 7, 180, 300);
    gint width = MAX(320, geom.width - sidebar_width - 96);
    gint height = geom.height;

    const gint reserved_vertical = 160;
    const gint spacing = 16;
    const gint target_button = 180;

    gint max_cols = clamp_int(width / (target_button + spacing), 3, 10);
    gint max_rows = clamp_int((height - reserved_vertical) / (target_button + spacing), 2, 5);

    gint cols = max_cols;
    gint rows = max_rows;

    gint button_w = (width - (cols + 1) * spacing) / cols;
    gint button_h = (height - reserved_vertical - (rows + 1) * spacing) / rows;
    gint button = clamp_int(MIN(button_w, button_h), 96, 220);

    gint icon = clamp_int(button * 3 / 5, 56, 144);

    launcher->grid_columns = cols;
    launcher->grid_rows = rows;
    launcher->apps_per_page = cols * rows;
    launcher->button_size = button;
    launcher->icon_size = icon;
    launcher->grid_spacing = spacing;

    gtk_grid_set_row_spacing(GTK_GRID(launcher->app_grid), launcher->grid_spacing);
    gtk_grid_set_column_spacing(GTK_GRID(launcher->app_grid), launcher->grid_spacing);

    if (launcher->search_box) {
        gint search_w = clamp_int(width / 3, 280, 480);
        gtk_widget_set_size_request(launcher->search_box, search_w, -1);
    }
    if (launcher->category_scrolled_window) {
        gtk_widget_set_size_request(launcher->category_scrolled_window, sidebar_width, -1);
    }
}

void create_overlay_window(LauncherPlugin *launcher) {
    GtkWidget *main_box, *search_box, *grid_container, *center_box;
    GdkScreen *screen;
    GdkVisual *visual;
    
    launcher->overlay_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_add_events(launcher->overlay_window,
                          GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK);
    gtk_window_set_type_hint(GTK_WINDOW(launcher->overlay_window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_decorated(GTK_WINDOW(launcher->overlay_window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(launcher->overlay_window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(launcher->overlay_window), TRUE);
    gtk_window_fullscreen(GTK_WINDOW(launcher->overlay_window));
    
    screen = gtk_widget_get_screen(launcher->overlay_window);
    visual = gdk_screen_get_rgba_visual(screen);
    if (visual && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(launcher->overlay_window, visual);
    }
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, get_css_style(), -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(launcher->overlay_window);
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    launcher->background = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(launcher->background), FALSE);
    gtk_widget_add_events(launcher->background, GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK);
    gtk_container_add(GTK_CONTAINER(launcher->overlay_window), launcher->background);
    g_signal_connect(launcher->background, "button-press-event",
                     G_CALLBACK(on_background_button_press), launcher);
    g_signal_connect(launcher->background, "scroll-event",
                     G_CALLBACK(on_scroll_event), launcher);
    g_signal_connect(launcher->overlay_window, "button-press-event",
                     G_CALLBACK(on_background_button_press), launcher);
    g_signal_connect(launcher->overlay_window, "scroll-event",
                     G_CALLBACK(on_scroll_event), launcher);

    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(launcher->background), main_box);

    search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    launcher->search_box = search_box;
    gtk_widget_set_halign(search_box, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(search_box), "search-container");
    gtk_widget_set_size_request(search_box, 400, -1);
    gtk_box_pack_start(GTK_BOX(main_box), search_box, FALSE, FALSE, 0);

    launcher->search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(launcher->search_entry), "Search");
    gtk_widget_set_hexpand(launcher->search_entry, TRUE);
    gtk_box_pack_start(GTK_BOX(search_box), launcher->search_entry, TRUE, TRUE, 0);

    launcher->back_button = gtk_button_new_with_label("Back");
    gtk_box_pack_start(GTK_BOX(search_box), launcher->back_button, FALSE, FALSE, 0);
    g_signal_connect(launcher->back_button, "clicked", G_CALLBACK(on_back_button_clicked), launcher);
    gtk_widget_set_no_show_all(launcher->back_button, TRUE);
    gtk_widget_hide(launcher->back_button);

    g_signal_connect(launcher->search_entry, "search-changed",
                     G_CALLBACK(on_search_changed), launcher);

    center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(center_box, GTK_ALIGN_FILL);
    gtk_widget_set_valign(center_box, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(center_box, TRUE);
    gtk_widget_set_vexpand(center_box, TRUE);
    gtk_widget_set_margin_start(center_box, 24);
    gtk_widget_set_margin_end(center_box, 24);
    gtk_box_pack_start(GTK_BOX(main_box), center_box, TRUE, TRUE, 0);

    launcher->category_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(launcher->category_scrolled_window),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(launcher->category_scrolled_window),
                                        GTK_SHADOW_NONE);
    gtk_widget_set_valign(launcher->category_scrolled_window, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand(launcher->category_scrolled_window, TRUE);
    gtk_widget_set_size_request(launcher->category_scrolled_window, 210, -1);

    launcher->category_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(launcher->category_scrolled_window), launcher->category_box);
    gtk_box_pack_start(GTK_BOX(center_box), launcher->category_scrolled_window, FALSE, FALSE, 12);
    update_category_sidebar(launcher);

    grid_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(grid_container, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(grid_container, TRUE);
    gtk_box_pack_start(GTK_BOX(center_box), grid_container, TRUE, TRUE, 0);

    launcher->app_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(launcher->app_grid), launcher->grid_spacing);
    gtk_grid_set_column_spacing(GTK_GRID(launcher->app_grid), launcher->grid_spacing);
    gtk_widget_set_halign(launcher->app_grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(launcher->app_grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(grid_container), launcher->app_grid, FALSE, FALSE, 0);

    launcher->page_dots = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(launcher->page_dots, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(launcher->page_dots), "page-dots");
    gtk_box_pack_start(GTK_BOX(main_box), launcher->page_dots, FALSE, FALSE, 0);

    populate_current_page(launcher);
    update_page_dots(launcher);

    g_signal_connect(launcher->overlay_window, "key-press-event",
                     G_CALLBACK(on_key_press), launcher);

    gtk_style_context_add_provider_for_screen(screen,
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

void hide_overlay(LauncherPlugin *launcher) {
    if (launcher->overlay_window) {
        gtk_widget_hide(launcher->overlay_window);
        gtk_entry_set_text(GTK_ENTRY(launcher->search_entry), "");

        if (launcher->filtered_list) {
            g_list_free(launcher->filtered_list);
        }
        launcher->filtered_list = g_list_copy(launcher->app_list);
        launcher->current_page = 0;
    }
}

void populate_current_page(LauncherPlugin *launcher) {
    if (!launcher) {
        return;
    }

    GList *iter;
    gint row, col;
    gint apps_per_page = launcher->apps_per_page > 0 ? launcher->apps_per_page : APPS_PER_PAGE;

    gtk_container_foreach(GTK_CONTAINER(launcher->app_grid), destroy_widget_cb, NULL);

    /* Compute total tiles and clamp current page */
    gint tile_count = 0;
    if (launcher->open_folder) {
        for (iter = launcher->open_folder->apps; iter != NULL; iter = g_list_next(iter)) {
            AppInfo *app = (AppInfo *)iter->data;
            if (app_visible_in_folder(app)) {
                tile_count++;
            }
        }
    } else {
        if (!launcher->selected_category) {
            tile_count += g_list_length(launcher->folder_list);
        }

        GList *apps_to_count = launcher->filtered_list ? launcher->filtered_list : launcher->app_list;
        for (iter = apps_to_count; iter != NULL; iter = g_list_next(iter)) {
            AppInfo *app = (AppInfo *)iter->data;
            if (app_visible_on_main_view(launcher, app)) {
                tile_count++;
            }
        }
    }

    launcher->total_pages = (tile_count + apps_per_page - 1) / apps_per_page;
    if (launcher->total_pages < 1) {
        launcher->total_pages = 1;
    }
    if (launcher->current_page >= launcher->total_pages) {
        launcher->current_page = launcher->total_pages - 1;
    }
    if (launcher->current_page < 0) {
        launcher->current_page = 0;
    }

    gint start_index = launcher->current_page * apps_per_page;
    gint end_index = start_index + apps_per_page;

    gint global_index = 0;
    gint grid_index = 0;

    if (!launcher->open_folder) {
        /* Display folders as part of the paged grid */
        for (iter = launcher->selected_category ? NULL : launcher->folder_list;
             iter != NULL; iter = g_list_next(iter)) {
            FolderInfo *folder_info = (FolderInfo *)iter->data;

            if (global_index >= start_index && global_index < end_index) {
                col = grid_index % launcher->grid_columns;
                row = grid_index / launcher->grid_columns;

                GtkWidget *button, *box, *preview, *label;
                button = gtk_button_new();
                gtk_style_context_add_class(gtk_widget_get_style_context(button), "folder");
                gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
                gtk_widget_set_size_request(button, launcher->button_size, launcher->button_size);

                box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
                gtk_container_add(GTK_CONTAINER(button), box);

                preview = create_folder_preview_widget(launcher, folder_info);
                gtk_box_pack_start(GTK_BOX(box), preview, FALSE, FALSE, 0);

                label = gtk_label_new(folder_info ? folder_info->name : "");
                gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

                /* Allow dropping apps onto folders */
                gtk_drag_dest_set(button,
                                  GTK_DEST_DEFAULT_ALL,
                                  app_dnd_targets,
                                  G_N_ELEMENTS(app_dnd_targets),
                                  GDK_ACTION_MOVE);
                g_signal_connect(button,
                                 "drag-data-received",
                                 G_CALLBACK(on_drag_data_received),
                                 launcher);

                g_signal_connect(button, "button-press-event", G_CALLBACK(on_folder_button_press_event), folder_info);
                g_signal_connect(button, "clicked", G_CALLBACK(on_folder_clicked), folder_info);

                gtk_grid_attach(GTK_GRID(launcher->app_grid), button, col, row, 1, 1);
                g_object_set_data(G_OBJECT(button), "folder-info", folder_info);
                g_object_set_data(G_OBJECT(button), "launcher", launcher);
                gtk_widget_show_all(button);

                grid_index++;
            }

            global_index++;
        }

        /* Display applications */
        GList *apps_to_display = launcher->filtered_list ? launcher->filtered_list : launcher->app_list;
        for (iter = apps_to_display; iter != NULL; iter = g_list_next(iter)) {
            AppInfo *app_info = (AppInfo *)iter->data;

            if (!app_visible_on_main_view(launcher, app_info)) {
                continue;
            }

            if (global_index >= start_index && global_index < end_index) {
                GtkWidget *button, *box, *icon, *label;

                col = grid_index % launcher->grid_columns;
                row = grid_index / launcher->grid_columns;

                button = gtk_button_new();
                gtk_style_context_add_class(gtk_widget_get_style_context(button), "app-button");
                gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
                gtk_widget_set_size_request(button, launcher->button_size, launcher->button_size);

                box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
                gtk_container_add(GTK_CONTAINER(button), box);

                if (app_info->icon) {
                    icon = gtk_image_new_from_icon_name(app_info->icon, GTK_ICON_SIZE_DIALOG);
                } else {
                    icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
                }
                gtk_image_set_pixel_size(GTK_IMAGE(icon), launcher->icon_size);
                gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);

                label = create_label_with_optional_highlight(launcher, app_info->name);
                gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
                gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
                gtk_label_set_max_width_chars(GTK_LABEL(label), 18);
                gtk_label_set_lines(GTK_LABEL(label), 2);
                gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

                gtk_drag_source_set(button,
                                    GDK_BUTTON1_MASK,
                                    app_dnd_targets,
                                    G_N_ELEMENTS(app_dnd_targets),
                                    GDK_ACTION_MOVE);
                gtk_drag_dest_set(button,
                                  GTK_DEST_DEFAULT_ALL,
                                  app_dnd_targets,
                                  G_N_ELEMENTS(app_dnd_targets),
                                  GDK_ACTION_MOVE);

                g_signal_connect(button, "drag-data-received",
                                 G_CALLBACK(on_drag_data_received), launcher);
                g_signal_connect(button, "drag-data-get",
                                 G_CALLBACK(on_drag_data_get), app_info);

                g_signal_connect(button, "drag-begin",
                                 G_CALLBACK(on_drag_begin), launcher);

                g_signal_connect(button, "button-press-event",
                                 G_CALLBACK(on_button_press_event), app_info);

                g_signal_connect(button, "clicked",
                                 G_CALLBACK(launch_application), app_info);

                gtk_grid_attach(GTK_GRID(launcher->app_grid), button, col, row, 1, 1);

                g_object_set_data(G_OBJECT(button), "app-info", app_info);
                g_object_set_data(G_OBJECT(button), "launcher", launcher);

                gtk_widget_show_all(button);

                grid_index++;
            }

            global_index++;
        }

        return;
    }

    /* Folder is open: display folder apps paged */
    for (iter = launcher->open_folder->apps; iter != NULL; iter = g_list_next(iter)) {
        AppInfo *app_info = (AppInfo *)iter->data;
        if (!app_visible_in_folder(app_info)) {
            continue;
        }

        if (global_index >= start_index && global_index < end_index) {
            GtkWidget *button, *box, *icon, *label;

            col = grid_index % launcher->grid_columns;
            row = grid_index / launcher->grid_columns;

            button = gtk_button_new();
            gtk_style_context_add_class(gtk_widget_get_style_context(button), "app-button");
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
            gtk_widget_set_size_request(button, launcher->button_size, launcher->button_size);

            box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
            gtk_container_add(GTK_CONTAINER(button), box);

            if (app_info->icon) {
                icon = gtk_image_new_from_icon_name(app_info->icon, GTK_ICON_SIZE_DIALOG);
            } else {
                icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
            }
            gtk_image_set_pixel_size(GTK_IMAGE(icon), launcher->icon_size);
            gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);

            label = create_label_with_optional_highlight(launcher, app_info->name);
            gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_label_set_max_width_chars(GTK_LABEL(label), 18);
            gtk_label_set_lines(GTK_LABEL(label), 2);
            gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

            gtk_drag_source_set(button,
                                GDK_BUTTON1_MASK,
                                app_dnd_targets,
                                G_N_ELEMENTS(app_dnd_targets),
                                GDK_ACTION_MOVE);
            gtk_drag_dest_set(button,
                              GTK_DEST_DEFAULT_ALL,
                              app_dnd_targets,
                              G_N_ELEMENTS(app_dnd_targets),
                              GDK_ACTION_MOVE);

            g_signal_connect(button, "drag-data-received",
                             G_CALLBACK(on_drag_data_received), launcher);
            g_signal_connect(button, "drag-data-get",
                             G_CALLBACK(on_drag_data_get), app_info);

            g_signal_connect(button, "drag-begin",
                             G_CALLBACK(on_drag_begin), launcher);

            g_signal_connect(button, "button-press-event",
                             G_CALLBACK(on_button_press_event), app_info);

            g_signal_connect(button, "clicked",
                             G_CALLBACK(launch_application), app_info);

            gtk_grid_attach(GTK_GRID(launcher->app_grid), button, col, row, 1, 1);

            g_object_set_data(G_OBJECT(button), "app-info", app_info);
            g_object_set_data(G_OBJECT(button), "launcher", launcher);

            gtk_widget_show_all(button);

            grid_index++;
        }

        global_index++;
    }
}

void update_page_dots(LauncherPlugin *launcher) {
    GList *children, *iter;
    gint i;

    children = gtk_container_get_children(GTK_CONTAINER(launcher->page_dots));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    gint apps_per_page = launcher->apps_per_page > 0 ? launcher->apps_per_page : APPS_PER_PAGE;

    gint tile_count = 0;
    if (launcher->open_folder) {
        for (iter = launcher->open_folder->apps; iter != NULL; iter = g_list_next(iter)) {
            AppInfo *app = (AppInfo *)iter->data;
            if (app_visible_in_folder(app)) {
                tile_count++;
            }
        }
    } else {
        if (!launcher->selected_category) {
            tile_count += g_list_length(launcher->folder_list);
        }

        GList *apps_to_count = launcher->filtered_list ? launcher->filtered_list : launcher->app_list;
        for (iter = apps_to_count; iter != NULL; iter = g_list_next(iter)) {
            AppInfo *app = (AppInfo *)iter->data;
            if (app_visible_on_main_view(launcher, app)) {
                tile_count++;
            }
        }
    }

    launcher->total_pages = (tile_count + apps_per_page - 1) / apps_per_page;
    if (launcher->total_pages < 1) {
        launcher->total_pages = 1;
    }

    if (launcher->current_page >= launcher->total_pages) {
        launcher->current_page = launcher->total_pages - 1;
    }
    if (launcher->current_page < 0) {
        launcher->current_page = 0;
    }

    for (i = 0; i < launcher->total_pages; i++) {
        GtkWidget *dot = gtk_button_new();
        gtk_style_context_add_class(gtk_widget_get_style_context(dot), "page-dot");

        if (i == launcher->current_page) {
            gtk_style_context_add_class(gtk_widget_get_style_context(dot), "active");
        }

        g_object_set_data(G_OBJECT(dot), "page-index", GINT_TO_POINTER(i));
        g_object_set_data(G_OBJECT(dot), "launcher", launcher);
        g_signal_connect(dot, "clicked", G_CALLBACK(on_dot_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(launcher->page_dots), dot, FALSE, FALSE, 0);
        gtk_widget_show(dot);
    }
}

const gchar* get_css_style(void) {
    return
    "window {\n"
    "  background-color: rgba(40, 40, 40, 0.85);\n"
    "}\n"
    "button.app-button {\n"
    "  background-color: transparent;\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  padding: 8px;\n"
    "  margin: 4px;\n"
    "  border-radius: 14px;\n"
    "}\n"
    "button.app-button:hover {\n"
    "  background-color: rgba(255, 255, 255, 0.1);\n"
    "}\n"
    "button.app-button:active {\n"
    "  background-color: rgba(255, 255, 255, 0.15);\n"
    "}\n"
    "button.app-button:focus {\n"
    "  outline: none;\n"
    "}\n"
    "button.app-button label {\n"
    "  color: rgba(255, 255, 255, 0.9);\n"
    "  font-size: 16px;\n"
    "  font-weight: 400;\n"
    "}\n"
    ".search-container {\n"
    "  background-color: rgba(255, 255, 255, 0.15);\n"
    "  border-radius: 12px;\n"
    "  border: 1px solid rgba(255, 255, 255, 0.2);\n"
    "  margin: 16px;\n"
    "}\n"
    "button.category-button {\n"
    "  background-color: transparent;\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  border-radius: 9px;\n"
    "  padding: 10px 14px;\n"
    "}\n"
    "button.category-button:hover {\n"
    "  background-color: rgba(255, 255, 255, 0.1);\n"
    "}\n"
    "button.category-button.selected {\n"
    "  background-color: rgba(255, 255, 255, 0.2);\n"
    "}\n"
    "button.category-button label {\n"
    "  color: rgba(255, 255, 255, 0.9);\n"
    "  font-size: 17px;\n"
    "  font-weight: 500;\n"
    "}\n"
    "entry {\n"
    "  background-color: transparent;\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  font-size: 19px;\n"
    "  padding: 8px 14px;\n"
    "  color: white;\n"
    "  caret-color: white;\n"
    "  font-weight: 300;\n"
    "}\n"
    "entry:focus {\n"
    "  outline: none;\n"
    "}\n"
    "entry text {\n"
    "  color: white;\n"
    "}\n"
    "entry text selection {\n"
    "  background-color: rgba(255, 255, 255, 0.3);\n"
    "  color: white;\n"
    "}\n"
    "box.page-dots {\n"
    "  padding: 14px;\n"
    "}\n"
    "button.page-dot {\n"
    "  background-color: rgba(255, 255, 255, 0.3);\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  border-radius: 50%;\n"
    "  min-width: 8px;\n"
    "  min-height: 8px;\n"
    "  margin: 0px 5px;\n"
    "  padding: 4px;\n"
    "}\n"
    "button.page-dot.active {\n"
    "  background-color: rgba(255, 255, 255, 0.9);\n"
    "}\n"
    "button.page-dot:hover {\n"
    "  background-color: rgba(255, 255, 255, 0.5);\n"
    "}\n"
    "button.folder {\n"
    "  background-color: rgba(255, 255, 255, 0.08);\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  padding: 8px;\n"
    "  margin: 4px;\n"
    "  border-radius: 16px;\n"
    "}\n"
    "button.folder:hover {\n"
    "  background-color: rgba(255, 255, 255, 0.15);\n"
    "}\n"
    "button.folder label {\n"
    "  color: rgba(255, 255, 255, 0.9);\n"
    "  font-size: 16px;\n"
    "  font-weight: 400;\n"
    "}\n";
}







































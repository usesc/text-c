#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "gtkapp"

#define PROGRAM_ICON "icon.jpg"

#define STYLESHEET "styles.css"
#define RESIZABLE TRUE

#ifdef _WIN32
#define SHELL "cmd.exe"
#elif __APPLE__
#define SHELL "/bin/bash"
#elif defined(__unix__) || defined(__unix)
#define SHELL "/bin/bash"
#else
#define SHELL "/bin/sh"
#endif

char *current_file = NULL;

off_t file_size(const char *file) {
  struct stat st;
  int ret = stat(file, &st);
  return ret == 0 ? st.st_size : -1;
}

char *file_data(const char *file) {
  gchar *contents = NULL;
  gsize length = 0;
  GError *error = NULL;

  if (g_file_get_contents(file, &contents, &length, &error)) {
    return contents;
  } else {
    g_printerr("Failed to read file %s: %s\n", file, error->message);
    g_clear_error(&error);
    return NULL;
  }
}

void load_css(void) {
  GtkCssProvider *provider;
  GdkDisplay *display;
  GdkScreen *screen;

  provider = gtk_css_provider_new();
  display = gdk_display_get_default();
  screen = gdk_display_get_default_screen(display);

  gtk_style_context_add_provider_for_screen(
    screen,
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );

  GError *error = NULL;
  if (!gtk_css_provider_load_from_path(provider, STYLESHEET, &error)) {
    g_printerr("Error loading CSS: %s\n", error->message);
    g_clear_error(&error);
  }

  g_object_unref(provider);
}

void open_pressed(GtkWidget *widget, gpointer data) {
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
    "Open File",
    NULL,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    "_Cancel", 
    GTK_RESPONSE_CANCEL,
    "_Open", 
    GTK_RESPONSE_ACCEPT,
    NULL
  );

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    if (filename) {
      char *contents = file_data(filename);
      if (contents) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data));
        gtk_text_buffer_set_text(buffer, contents, -1);
        free(contents);

        if (current_file) {
          g_free(current_file);
        }
        current_file = g_strdup(filename); 
      } 
      else {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data));
        gtk_text_buffer_set_text(buffer, "Error reading file.", -1);
      }
      g_free(filename);
    }
  }

  gtk_widget_destroy(dialog);
}

void save_to_file(const char *filename, GtkTextBuffer *buffer) {
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

  FILE *fptr = fopen(filename, "w");
  if (!fptr) {
    g_printerr("Failed to open file %s for writing\n", filename);
    g_free(text);
    return;
  }

  fputs(text, fptr);
  fclose(fptr);
  g_free(text);
}

void save_pressed(GtkWidget *widget, gpointer data) {
  if (current_file) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data));
    save_to_file(current_file, buffer);
  }
  else {
    GtkWidget *dialog = gtk_message_dialog_new(
      GTK_WINDOW(gtk_widget_get_toplevel(widget)),
      GTK_DIALOG_MODAL,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_OK,
      "No file is currently open to save."
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

static void on_toggle_terminal(GtkButton *button, gpointer user_data) {
  static GtkWidget *terminal = NULL;
  GtkWidget *vpaned = GTK_WIDGET(user_data);

  if (terminal) {
    gtk_widget_destroy(terminal);
    terminal = NULL;
  } 
  else {
    terminal = vte_terminal_new();
    gtk_paned_pack2(GTK_PANED(vpaned), terminal, TRUE, TRUE);
    gtk_widget_show(terminal);

    char *shell[] = {g_strdup(SHELL), NULL};
    if (!shell[0]) return;
    vte_terminal_spawn_async(
      VTE_TERMINAL(terminal),
      VTE_PTY_DEFAULT,
      NULL,
      shell,
      NULL,
      0,
      NULL,
      NULL,
      NULL,
      -1,
      NULL,
      NULL,
      NULL
    );
    g_free(shell[0]);
  }
}

void reload_pressed(GtkWidget *widget, gpointer data) {
  if (current_file) {
    char *contents = file_data(current_file);
    if (contents) {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data));
      gtk_text_buffer_set_text(buffer, contents, -1);
      free(contents);
    } 
    else {
      g_printerr("Error: Could not read file '%s'\n", current_file); 
    }
  } 
  else {
    g_printerr("Warning: No file is open to reload.\n");
  }
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  current_file = g_strdup("default.c");

  load_css();

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);
  gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);
  gtk_window_set_resizable(GTK_WINDOW(window), RESIZABLE);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  GError *error = NULL;
  GdkPixbuf *icon = gdk_pixbuf_new_from_file(PROGRAM_ICON, &error);
  if (!icon) {
    g_printerr("Error loading icon: %s\n", error->message);
    g_error_free(error);
  }

  gtk_window_set_icon(GTK_WINDOW(window), icon);
  g_object_unref(icon); 

  GtkWidget *text_view = gtk_text_view_new();
  gtk_widget_set_name(text_view, "textview");
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

  GtkWidget *toolbar = gtk_toolbar_new();
  GtkToolItem *open_button = gtk_tool_button_new(NULL, "Open");
  GtkToolItem *save_button = gtk_tool_button_new(NULL, "Save");
  GtkToolItem *term_button = gtk_tool_button_new(NULL, "Term");
  GtkToolItem *reload_button = gtk_tool_button_new(NULL, "Reload");

  g_signal_connect(open_button, "clicked", G_CALLBACK(open_pressed), text_view);
  g_signal_connect(save_button, "clicked", G_CALLBACK(save_pressed), text_view);
  g_signal_connect(reload_button, "clicked", G_CALLBACK(reload_pressed), text_view);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), open_button, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save_button, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), term_button, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), reload_button, -1);

  char *file_contents = file_data("default.c");
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  if (file_contents != NULL) {
    gtk_text_buffer_set_text(buffer, file_contents, -1);
    free(file_contents);
  } 
  else {
    gtk_text_buffer_set_text(buffer, "Failed to load file data", -1);
  }

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(
    GTK_SCROLLED_WINDOW(scrolled_window),
    GTK_POLICY_AUTOMATIC,
    GTK_POLICY_ALWAYS
  );
  gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

  GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_pack1(GTK_PANED(vpaned), scrolled_window, TRUE, FALSE);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  g_signal_connect(term_button, "clicked", G_CALLBACK(on_toggle_terminal), vpaned);

  gtk_widget_show_all(window);

  gtk_main();

  g_free(current_file);
  return 0;
}

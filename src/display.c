#include "display.h"
#include "fractal.h"
#include "iff.h"

int isPipe = FALSE;
int *image = NULL;
float *buf = NULL;
UBYTE color[3];
struct ViewData viewData;
struct FractalData data;
struct ColorData colorData;
static cairo_surface_t *surface = NULL;

/* Open stream. If file specification begins with '|' then open as a piped
 stream otherwise open as a file */

FILE *open_file(char *fspec, char *mode) {
  if (*fspec == '|') {
    ++fspec;
    isPipe = TRUE;
    return popen(fspec, mode);
  } else {
    isPipe = FALSE;
    return fopen(fspec, mode);
  }
}

/* Close stream. */

int close_file(FILE *stream) {
  int rc = 0;

  if (isPipe)
    rc = pclose(stream);
  else
    rc = fclose(stream);
  isPipe = FALSE;

  return rc;
}

int read_iff(char *file) {
  FILE *fp;
  struct Chunk header;
  long id, buflen;

  if (file == 0)
    fp = stdin;
  else if ((fp = fopen(file, "r")) == NULL)
    return (0);

  SafeRead(fp, &header, sizeof(header));
  if (header.ckID != ID_FORM) {
    fclose(fp);
    return (0);
  }

  SafeRead(fp, &id, sizeof(id));
  if (id != ID_FRCL) {
    fclose(fp);
    return (0);
  }

  for (;;) {
    SafeRead(fp, &header, sizeof(header));
    if (header.ckID == ID_ENDD)
      break;

    switch (header.ckID) {
    case ID_GLBL:
      SafeRead(fp, &viewData, sizeof(struct ViewData));
      SafeRead(fp, &data, sizeof(struct FractalData));
      SafeRead(fp, &colorData, sizeof(struct ColorData));
      break;
    case ID_DATA:
      buflen = viewData.xres * viewData.yres;

      if ((buf = calloc((long)buflen, sizeof(float))) == NULL) {
        fprintf(stderr, "main - insufficient memory!!!\n");
        return (0);
      }
      SafeRead(fp, buf, buflen * sizeof(float));

      if ((image = calloc((long)buflen, sizeof(int))) == NULL) {
        fprintf(stderr, "main - insufficient memory!!!\n");
      }

      break;
    }
  }
  fclose(fp);
  return (1);
}

static UBYTE *get_color(float fiter, int nindex, int shift,
                        int indices[MAX_INDICES], UBYTE comps[3][MAX_INDICES]) {
  int nc;
  if (fiter < 0.0) {
    for (nc = 0; nc < 3; nc++)
      color[nc] = 0;
  } else {
    int nlast = nindex - 1;
    float riter = fmodf(fiter + (float)shift, (float)indices[nlast]);
    float rf;
    int ni;
    UBYTE comp1, comp2;

    for (ni = 0; ni <= nlast; ni++) {
      if (indices[ni] > riter)
        break;
    }
    // if in the first interval or after the last then interpolate between last
    // and first colors
    if (ni == 0 || ni > nlast) {
      if (indices[0] > 0) {
        rf = riter / (float)indices[0];
        for (nc = 0; nc < 3; nc++) {
          comp1 = comps[nc][nlast];
          comp2 = comps[nc][0];
          color[nc] = (UBYTE)((float)comp1 + (float)(comp2 - comp1) * rf);
        }
      } else {
        for (nc = 0; nc < 3; nc++)
          color[nc] = comps[nc][0];
      }
    }
    // otherwise interpolate between colors at the start and end of the interval
    else {
      if (indices[ni] > indices[ni - 1]) {
        rf = (riter - (float)indices[ni - 1]) /
             (float)(indices[ni] - indices[ni - 1]);
        for (nc = 0; nc < 3; nc++) {
          comp1 = comps[nc][ni - 1];
          comp2 = comps[nc][ni];
          color[nc] = (UBYTE)((float)comp1 + (float)(comp2 - comp1) * rf);
        }
      } else {
        for (nc = 0; nc < 3; nc++)
          color[nc] = comps[nc][ni];
      }
    }
  }
  return color;
}

static void render_image(GtkDrawingArea *area, cairo_t *cr, int width,
                          int height, gpointer user_data) {
  long i, j, pos = 0;
  float fiter;
  UBYTE *c;

  pos = 0;

  for (j = 0; j < viewData.yres; j++) {
    for (i = 0; i < viewData.xres; i++) {
      fiter = buf[pos++];
      c = get_color(fiter, colorData.nindex, colorData.shift, colorData.indices,
                    colorData.comps);
      image[pos] = c[0] << 16 | c[1] << 8 | c[2];
    }
  }

  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);
}

static void close_window(GtkWindow *win) {
  if (surface)
    cairo_surface_destroy(surface);
  gtk_window_close(win);
  if (buf)
    free(buf);
  if (image)
    free(image);
}

static void
primary (GtkGestureClick *gesture,
         int              n_press,
         double           x,
         double           y,
         GtkWidget       *area)
{
  double dx, dy;

  if (viewData.xres > viewData.yres) {
    dx = data.size * (double)viewData.xres / viewData.yres;
    dy = data.size;
  } else {
    dx = data.size;
    dy = data.size * (double)viewData.yres / viewData.xres;
  }

  double fx = data.xc + dx * ((x / viewData.xres) - 0.5);
  double fy = data.yc + dy * ((y / viewData.yres) - 0.5);

  printf("1 %g %g\n", fx, fy);
}

static void
secondary (GtkGestureClick *gesture,
         int              n_press,
         double           x,
         double           y,
         GtkWidget       *area)
{
  printf("2 %g %g\n", x, y);
}

static void activate(GApplication *app, gpointer data) {
  GtkWidget *win = gtk_application_window_new(GTK_APPLICATION(app));
  GtkWidget *area = gtk_drawing_area_new();
  GtkGesture *first, *second;

  g_signal_connect(win, "destroy", G_CALLBACK(close_window), win);

  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, viewData.xres);
  surface = cairo_image_surface_create_for_data(
      (UBYTE *)image, CAIRO_FORMAT_RGB24, viewData.xres, viewData.yres, stride);

  gtk_window_set_title(GTK_WINDOW(win), "fractal");
  gtk_window_set_default_size(GTK_WINDOW(win), viewData.xres, viewData.yres);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), render_image, NULL,
                                 NULL);
  first = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (first), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller (area, GTK_EVENT_CONTROLLER (first));

  g_signal_connect (first, "pressed", G_CALLBACK (primary), area);
  second = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (second), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller (area, GTK_EVENT_CONTROLLER (second));

  g_signal_connect (second, "pressed", G_CALLBACK (secondary), area);
  gtk_window_set_child(GTK_WINDOW(win), area);

  gtk_window_present(GTK_WINDOW(win));
}

#define APPLICATION_ID "org.rl8n.display"

int main(int argc, char **argv) {
  GtkApplication *app;
  char *input;

  if (argc > 1)
    input = argv[1];
  else
    input = 0;

  if (!read_iff(input)) {
    free(buf);
    fprintf(stderr, "main - error reading image!!!\n");
    return 0;
  }

  app = gtk_application_new(APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  g_application_run(G_APPLICATION(app), 0, NULL);
  g_object_unref(app);
}

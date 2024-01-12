#include "display.h"

int xres, yres;
int *buf;
int isPipe = FALSE;
static cairo_surface_t *surface = NULL;

char pgetc(FILE *file) {
    int ich;
    char ch;

    ich = (int) fgetc(file);
    if (ich == EOF)
        return (0);
    ch = (char) ich;

    if (ch == '#') {
        do {
            ich = (int) fgetc(file);
            if (ich == EOF)
                return (0);
            ch = (char) ich;
        } while (ch != '\n');
    }

    return ch;
}

int getint(FILE *file) {
    char ch;
    int i;

    do {
        ch = pgetc(file);
    } while (ch == ' ' || ch == '\t' || ch == '\n');

    if (ch < '0' || ch > '9')
        return (0);

    i = 0;
    do {
        i = i * 10 + ch - '0';
        ch = pgetc(file);
    } while (ch >= '0' && ch <= '9');

    return i;
}

/* Open stream. If file specification begins with '|' then open as a piped
 stream otherwise open as a file */

FILE* open_file(char *fspec, char *mode) {
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

int read_ppm(char *fspec) {
    FILE *fp;
    int i, j, n, pos, max;
    long ich1, ich2, id, buflen;
    UBYTE rc, gc, bc;

    if (fspec != 0) {
		if ((fp = open_file(fspec, "r")) == NULL) {
			fprintf(stderr, "read - could not open map!!!\n");
			return (0);
		}
    } else {
    	fp = stdin;
    }

    ich1 = (long) fgetc(fp);
    if (ich1 == EOF) {
        fprintf(stderr, "read - premature EOF reading magic number\n");
        close_file(fp);
        return (0);
    }
    ich2 = (long) fgetc(fp);
    if (ich2 == EOF) {
        fprintf(stderr, "read - premature EOF reading magic number\n");
        close_file(fp);
        return (0);
    }
    id = ich1 * 256L + ich2;
    if ((id != PGM_FORMAT) && (id != RPGM_FORMAT) && (id != PPM_FORMAT)
            && (id != RPPM_FORMAT)) {
        fprintf(stderr, "read - not a PPM or PGM file!!!\n");
        close_file(fp);
        return (0);
    }

    xres = (int) getint(fp);
    yres = (int) getint(fp);

    max = getint(fp);
    i = 256 / (max + 1);
    n = 0;
    while (i > 1) {
        n++;
        i >>= 1;
    }

    buflen = (long) xres * yres;
	if ((buf = calloc((long) buflen, sizeof(int))) == NULL) {
		fprintf(stderr, "main - insufficient memory!!!\n");
		return (0);
	}
    pos = 0;
    for (i = 0; i < yres; i++) { /* process n lines/screen */
        switch (id) {
        case PGM_FORMAT:
            for (j = 0; j < xres; j++) {
            	rc = gc = bc = (UBYTE) (getint(fp) << n);
				buf[pos] = rc << 16 | gc << 8 | bc;
                pos++;
            }
            break;
        case RPGM_FORMAT:
            for (j = 0; j < xres; j++) {
            	rc = gc = bc = (UBYTE) (fgetc(fp) << n);
				buf[pos] = rc << 16 | gc << 8 | bc;
                pos++;
            }
            break;
        case PPM_FORMAT:
            for (j = 0; j < xres; j++) {
            	rc = (UBYTE) (getint(fp) << n);
            	gc = (UBYTE) (getint(fp) << n);
            	bc = (UBYTE) (getint(fp) << n);
				buf[pos] = rc << 16 | gc << 8 | bc;
                pos++;
            }
            break;
        case RPPM_FORMAT:
            for (j = 0; j < xres; j++) {
                rc = (UBYTE) (fgetc(fp) << n);
                gc = (UBYTE) (fgetc(fp) << n);
                bc = (UBYTE) (fgetc(fp) << n);
				buf[pos] = rc << 16 | gc << 8 | bc;
                pos++;
            }
            break;
        }
    }

    while ((long) fgetc(fp) != EOF)
        ;

    close_file(fp);
    return (1);
} /* end ReadPPM */

static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
}

static void close_window (GtkWindow *win)
{
    if (buf) free(buf);
    if (surface) cairo_surface_destroy (surface);
    gtk_window_close (win);
}

static void activate(GApplication *app, gpointer data) {
    GtkWidget *win = gtk_application_window_new(GTK_APPLICATION(app));
    GtkWidget *area = gtk_drawing_area_new();

    g_signal_connect (win, "destroy", G_CALLBACK (close_window), win);

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, xres);
    surface = cairo_image_surface_create_for_data((UBYTE *) buf, CAIRO_FORMAT_RGB24, xres, yres, stride);

    gtk_window_set_title(GTK_WINDOW(win), "fractal");
    gtk_window_set_default_size (GTK_WINDOW (win), xres, yres);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_function, NULL, NULL);
    gtk_window_set_child(GTK_WINDOW(win), area);

    gtk_window_present(GTK_WINDOW(win));
}

#define APPLICATION_ID "org.rl8n.display"

int main(int argc, char **argv)
{
    GtkApplication *app;
    char *input;

    if (argc > 1) input = argv[1];
    else input = 0;

    if (!read_ppm(input)) {
        free(buf);
        fprintf(stderr, "main - error reading image!!!\n");
        return 0;
    }

    app = gtk_application_new(APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
}

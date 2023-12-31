#include "display.h"

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

FILE *Open(char *fspec, char *mode) {
    if (*fspec == '|') {
        ++fspec;
        pipe = TRUE;
        return popen(fspec, mode);
    } else {
        pipe = FALSE;
        return fopen(fspec, mode);
    }
}

/* Close stream. */

int Close(FILE *stream) {
    int rc = 0;

    if (pipe)
        rc = pclose(stream);
    else
        rc = fclose(stream);
    pipe = FALSE;

    return rc;
}

int ReadImage(char *fspec) {
    FILE *fp;
    int i, j, n, pos, max;
    long ich1, ich2, id, buflen;
    UBYTE rc, gc, bc;

    if (fspec != 0) {
		if ((fp = Open(fspec, "r")) == NULL) {
			fprintf(stderr, "read - could not open map!!!\n");
			return (0);
		}
    } else {
    	fp = stdin;
    }

    ich1 = (long) fgetc(fp);
    if (ich1 == EOF) {
        fprintf(stderr, "read - premature EOF reading magic number\n");
        Close(fp);
        return (0);
    }
    ich2 = (long) fgetc(fp);
    if (ich2 == EOF) {
        fprintf(stderr, "read - premature EOF reading magic number\n");
        Close(fp);
        return (0);
    }
    id = ich1 * 256L + ich2;
    if ((id != PGM_FORMAT) && (id != RPGM_FORMAT) && (id != PPM_FORMAT)
            && (id != RPPM_FORMAT)) {
        fprintf(stderr, "read - not a PPM or PGM file!!!\n");
        Close(fp);
        return (0);
    }

    Width = (int) getint(fp);
    Height = (int) getint(fp);

    max = getint(fp);
    i = 256 / (max + 1);
    n = 0;
    while (i > 1) {
        n++;
        i >>= 1;
    }

    buflen = (long) Width * Height;
	if ((buf = calloc((long) buflen, sizeof(int))) == NULL) {
		fprintf(stderr, "main - insufficient memory!!!\n");
		return (0);
	}
    pos = 0;
    for (i = 0; i < Height; i++) { /* process n lines/screen */
        switch (id) {
        case PGM_FORMAT:
            for (j = 0; j < Width; j++) {
            	rc = gc = bc = (UBYTE) (getint(fp) << n);
				buf[pos] = rc << 16 | gc << 8 | bc;
                pos++;
            }
            break;
        case RPGM_FORMAT:
            for (j = 0; j < Width; j++) {
            	rc = gc = bc = (UBYTE) (fgetc(fp) << n);
				buf[pos] = rc << 16 | gc << 8 | bc;
                pos++;
            }
            break;
        case PPM_FORMAT:
            for (j = 0; j < Width; j++) {
            	rc = (UBYTE) (getint(fp) << n);
            	gc = (UBYTE) (getint(fp) << n);
            	bc = (UBYTE) (getint(fp) << n);
				buf[pos] = rc << 16 | gc << 8 | bc;
                pos++;
            }
            break;
        case RPPM_FORMAT:
            for (j = 0; j < Width; j++) {
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

    Close(fp);
    return (1);
} /* end ReadPPM */

void process_event(Display *display, Window window, XImage *ximage)
{
    XEvent ev;
    XNextEvent(display, &ev);
    switch(ev.type)
    {
    case Expose:
        XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, Width, Height);
        break;
    case ButtonPress:
    	printf("%d\n", ev.xbutton.x);
    	printf("%d\n", ev.xbutton.y);
    	printf("%d\n", ev.xbutton.button);
        exit(0);
    }
}

int main(int argc, char **argv)
{
    XImage *ximage;
    Display *display = XOpenDisplay(NULL);
    Visual *visual = DefaultVisual(display, 0);

    char *input;

    if (visual->class!=TrueColor) {
        fprintf(stderr, "Cannot handle non true color visual ...\n");
        exit(1);
    }

    if (argc > 1) input = argv[1];
    else input = 0;

    if (!ReadImage(input)) {
        free(buf);
        fprintf(stderr, "main - error reading image!!!\n");
    }

    Window window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, Width, Height, 1, 0, 0);

    ximage = XCreateImage(display, visual, DefaultDepth(display,DefaultScreen(display)), ZPixmap, 0, (char *)buf, Width, Height, 32, 0);
    XSelectInput(display, window, ButtonPressMask|ExposureMask);
    XMapWindow(display, window);

    while(1) {
        process_event(display, window, ximage);
    }
}

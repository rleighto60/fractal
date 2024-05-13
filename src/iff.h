/*
 * iff.h
 *
 */
#define MakeID(a, b, c, d)                                                     \
  ((long)(d) << 24L | (long)(c) << 16L | (b) << 8 | (a))

#define SafeRead(a, b, c)                                                      \
  if (fread(b, 1L, c, a) < c) {                                                \
    fclose(a);                                                                 \
    return FALSE;                                                                \
  }

#define SafeWrite(a, b, c)                                                     \
  if (fwrite(b, 1L, c, a) < c) {                                               \
    fclose(a);                                                                 \
    return FALSE;                                                                \
  }

#define SafeSeek(a, b)                                                         \
  if (fseek(a, b, SEEK_SET) != 0) {                                            \
    fclose(a);                                                                 \
    return FALSE;                                                                \
  }

#define ID_FORM MakeID('F', 'O', 'R', 'M')
#define ID_FRCL MakeID('F', 'R', 'C', 'L')
#define ID_GLBL MakeID('G', 'L', 'B', 'L')
#define ID_DATA MakeID('D', 'A', 'T', 'A')
#define ID_ENDD MakeID('E', 'N', 'D', 'D')

struct Chunk {
  long ckID, ckSize;
};

#define F_GB 'A'

#define INT9494(f,r,c)	((f) << 16) | ((r) << 8) | (c)
#define INTIS9494(c)	!!(((c) >> 16) & 0x7F)
#define INT9494F(c)	((c) >> 16) & 0x7F
#define INT9494R(c)	((c) >> 8) & 0x7F
#define INT9494C(c)	(c) & 0x7F
#define INTILL(c)	(1 << 23) | (c)

struct _conv {
	void	*buffer;
	size_t	size;

	int	(*char2int) (struct _conv*, const char *, ssize_t, CHAR_T **, size_t *);
	int	(*int2char) (struct _conv*, const CHAR_T *, ssize_t, char **, size_t *);
	int	(*file2int) (struct _conv*, const char *, ssize_t, CHAR_T **, size_t *);
	int	(*int2file) (struct _conv*, const CHAR_T *, ssize_t, char **, size_t *);
};
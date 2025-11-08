#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>  // size_t

/* Exit codes dùng thống nhất toàn repo */
#define EX_OK         0   /* thành công / không bế tắc */
#define EX_DEADLOCK   1   /* phát hiện bế tắc */
#define EX_BAD_INPUT  64  /* lỗi đầu vào/format (theo sysexits.h) */
#define EX_INTERNAL   70  /* lỗi nội bộ/chạy (theo sysexits.h) */

/* Cấp phát an toàn: thoát chương trình nếu lỗi cấp phát */
void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t sz);
void *xrealloc(void *p, size_t n);

/* Ngủ mili-giây (chịu EINTR) */
void  msleep(unsigned ms);

#endif /* UTIL_H */

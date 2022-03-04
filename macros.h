#define msg_log(fmt, ...) fprintf(stdout, "[MSG] " fmt "\n", ##__VA_ARGS__)
#define err_log(fmt, ...) fprintf(stderr, "[ERR] " fmt "\n", ##__VA_ARGS__)

#pragma once

typedef enum {
    CON_AP,
    CON_STA
} con_mode_t;

typedef uint32_t con_t;

void con_init(void);
void con_create(con_mode_t mode, int sockfd);
void con_delete(int sockfd);
size_t con_count(void);
bool con_get_con(int sockfd, con_t *con);
bool con_get_sock(con_t con, int *sockfd);

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../src/x3d.h"

#pragma once

// printf("%d\n", msg->header_magic);
// printf("%04x\n", msg->header_magic);
// printCharInHexadecimal(buffer, buffer[0]);
static void __attribute__((__unused__)) printCharInHexadecimal(uint8_t *str, int len)
{
    for (int i = 0; i < len; ++i)
    {
        uint8_t val = str[i];
        char tbl[]  = "0123456789ABCDEF";
        printf("0x");
        printf("%c", tbl[val / 16]);
        printf("%c", tbl[val % 16]);
        printf(", ");
    }
    printf("\n");
}

/**
 * Helper function to free unused result
 */
static void __attribute__((__unused__)) delete_msg(x3d_msg_t *msg)
{
    if (msg == NULL)
    {
        return;
    }
    if (msg->header_payload != NULL)
    {
        free(msg->header_payload);
    }
    if (msg->payload != NULL)
    {
        free(msg->payload);
    }
    free(msg);
}

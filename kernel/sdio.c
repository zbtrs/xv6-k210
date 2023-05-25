#include "include/types.h"
#include "include/printf.h"
#include "include/sdio.h"

static volatile uint32 * const sdio = (void *)(SDIO_BASE);

int send_cmd(int cmd, uint32 arg){
    REG32(sdio, SDIO_CMDARG) = arg;
    REG32(sdio, SDIO_CMD) = cmd | RES_expect | START_CMD;
    while (1)
    {
        printf("*");
        if (!(REG32(sdio, SDIO_CMD) & START_CMD))
        {
            break;
        }
    }
    printf("\n");
    return 0;
}

int send_cmd_with_info(int cmd, uint32 arg){
    REG32(sdio, SDIO_CMDARG) = arg;
    REG32(sdio, SDIO_CMD) = cmd | RES_expect | START_CMD;
    while (1)
    {
        printf("*");
        if (!(REG32(sdio, SDIO_CMD) & START_CMD))
        {
            break;
        }
    }
    printf("\n");
    printf("ctrl:%p\n", REG32(sdio, SDIO_CMD));
    printf("resp0:%p\n", REG32(sdio, SDIO_RESP0));
    printf("resp1:%p\n", REG32(sdio, SDIO_RESP1));
    printf("resp2:%p\n", REG32(sdio, SDIO_RESP2));
    printf("resp3:%p\n", REG32(sdio, SDIO_RESP3));
    return 0;
}

int sd_test()
{
    send_cmd_with_info(CMD0, 0);    //需替换为cmd52

    // send_cmd_with_info(CMD52, 0x80000C08);

    send_cmd_with_info(CMD8, 0x1AA);

    // send_cmd_with_info(CMD58, 0);

    // send_cmd_with_info(CMD5, 0);

    // uint32 r;
    // do
    // {
    //     send_cmd(CMD55, 0);
    //     send_cmd(ACMD41, 0x40FFFF00);
    //     r = 0x80000000 & REG32(sdio, SDIO_RESP0);
    //     printf("resp0:%p\n", REG32(sdio, SDIO_RESP0));
    // } while (r == 0);

    // send_cmd(CMD55, 0);
    // send_cmd(ACMD41, 0x40FFFF00);
    // printf("\n");
    // printf("ctrl:%p\n", REG32(sdio, SDIO_CMD));
    // printf("resp0:%p\n", REG32(sdio, SDIO_RESP0));
    // printf("resp1:%p\n", REG32(sdio, SDIO_RESP1));
    // printf("resp2:%p\n", REG32(sdio, SDIO_RESP2));
    // printf("resp3:%p\n", REG32(sdio, SDIO_RESP3));

    send_cmd(CMD55, 0);
    send_cmd(ACMD41, 0x40000000);
    printf("\n");
    printf("ctrl:%p\n", REG32(sdio, SDIO_CMD));
    printf("resp0:%p\n", REG32(sdio, SDIO_RESP0));
    printf("resp1:%p\n", REG32(sdio, SDIO_RESP1));
    printf("resp2:%p\n", REG32(sdio, SDIO_RESP2));
    printf("resp3:%p\n", REG32(sdio, SDIO_RESP3));

    send_cmd_with_info(CMD5, 0);

    // send_cmd_with_info(CMD16, 0x200);   //需替换为cmd52

    send_cmd_with_info(CMD24, 0);

    send_cmd_with_info(CMD52, 0x90022002);

    send_cmd_with_info(CMD52, 0);
    send_cmd_with_info(CMD52, 1);
    send_cmd_with_info(CMD52, 2);

    send_cmd_with_info(CMD24, 0);

    return 0;
}

int sd_init(){
    return 0;
}


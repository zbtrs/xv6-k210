#define SDIO_BASE 0x16020000

/* --- SDIO registers ----------------------------------------------------- */

/* Control Register */
#define SDIO_CTRL                       0x000

/* Power Enable Register */
#define SDIO_PWREN                      0x004

/* Clock Divider Register */
#define SDIO_CLKDIV                     0x008

/* SD Clock Source Register */
#define SDIO_CLKSRC                     0x00C

/* Clock Enable Register */
#define SDIO_CLKENA                     0x010

/* Time-out Register */
#define SDIO_TMOUT                      0x014

/* Card Type Register */
#define SDIO_CTYPE                      0x018

/* Block Size Register */
#define SDIO_BLKSIZ                     0x01C

/* Byte Count Register */
#define SDIO_BYTCNT                     0x020

/* Interrupt Mask Register */
#define SDIO_INTMASK                    0x024

/* Command Argument Register */
#define SDIO_CMDARG                     0x028

/* Command Register */
#define SDIO_CMD                        0x02C

/* Response Register 0 */
#define SDIO_RESP0                      0x030

/* Response Register 1 */
#define SDIO_RESP1                      0x034

/* Response Register 2 */
#define SDIO_RESP2                      0x038

/* Response Register 3 */
#define SDIO_RESP3                      0x03C

/* Masked Interrupt Status Register */
#define SDIO_MINTSTS                    0x040

/* Raw Interrupt Status Register */
#define SDIO_RINTSTS                    0x044

/* Status Register */
#define SDIO_STATUS                     0x048

/* FIFO Threshold Watermark Register */
#define SDIO_FIFOTH                     0x04C

/* Card Detect Register */
#define SDIO_CDETECT                    0x050

/* Write Protect Register */
#define SDIO_WRTPRT                     0x054

/* Transferred CIU Card Byte Count Register */
#define SDIO_TCBCNT                     0x05C

/* Transferred Host to BIU-FIFO Byte Count Register */
#define SDIO_TBBCNT                     0x060

/* Debounce Count Register */
#define SDIO_DEBNCE                     0x064

/* UHS-1 Register */
#define SDIO_UHS_REG                    0x074

/* Hardware Reset */
#define SDIO_RST_N                      0x078

/* Bus Mode Register */
#define SDIO_BMOD                       0x080

/* Poll Demand Register */
#define SDIO_PLDMND                     0x084

/* Descriptor List Base Address Register */
#define SDIO_DBADDR                     0x088

/* Internal DMAC Status Register */
#define SDIO_IDSTS                      0x08C

/* Internal DMAC Interrupt Enable Register */
#define SDIO_IDINTEN                    0x090

/* Current Host Descriptor Address Register */
#define SDIO_DSCADDR                    0x094

/* Current Buffer Descriptor Address Register */
#define SDIO_BUFADDR                    0x098

/* Data FIFO read/write */
#define SDIO_DATA                       0x100

#define REG32(p, i)	((p)[(i) >> 2])

/*CMD format*/
#define RES_expect 0x40
#define RES_length 0x80
#define START_CMD  0x80000000

/*CMD order*/
#define CMD0 0x40
#define CMD2 0x42
#define CMD5 0x45
#define CMD8 0x48
#define ACMD41 0x69
#define CMD58 0x7A
#define CMD16 0x50
#define CMD17 0x51
#define CMD55 0x77
#define CMD18 0x52  //多块读
#define CMD24 0x58  //单块读
#define CMD52 0x74
#define CMD53 0x75

/*funcitions*/
int sd_init(void);
int sd_test(void);






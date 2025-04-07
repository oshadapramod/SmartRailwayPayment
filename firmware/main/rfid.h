#ifndef RFID_H_
#define RFID_H_

#include <stdbool.h>
#include <stdint.h>

// RFID RC522 registers
#define COMMAND_REG       0x01
#define COM_IRQ_REG       0x04
#define DIV_IRQ_REG       0x05
#define ERROR_REG         0x06
#define STATUS_REG        0x07
#define FIFO_DATA_REG     0x09
#define FIFO_LEVEL_REG    0x0A
#define CONTROL_REG       0x0C
#define BIT_FRAMING_REG   0x0D
#define MODE_REG          0x11
#define TX_CONTROL_REG    0x14
#define TX_ASK_REG        0x15
#define CRC_RESULT_REG_L  0x22
#define CRC_RESULT_REG_H  0x21
#define T_MODE_REG        0x2A
#define T_PRESCALER_REG   0x2B
#define T_RELOAD_REG_L    0x2C
#define T_RELOAD_REG_H    0x2D
#define TMR_AUTO_REG      0x2E

// RFID RC522 commands
#define PCD_IDLE          0x00
#define PCD_TRANSCEIVE    0x0C
#define PCD_RESETPHASE    0x0F
#define PCD_CALCCRC       0x03

// MIFARE commands
#define PICC_REQIDL       0x26
#define PICC_ANTICOLL     0x93

// Other constants
#define MAX_LEN           16
#define MI_OK             0
#define MI_NOTAGERR       1
#define MI_ERR            2

// Function prototypes
void rfid_init(void);
bool rfid_card_present(void);
bool rfid_read_card_uid(uint8_t *card_uid, uint8_t *uid_size);
void rfid_reset(void);

#endif /* RFID_H_ */
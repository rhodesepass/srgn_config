#include "config_registry.h"

/* dependency/conflict lists (static constants) */
static const char *const REQ_CARDKB[] = {"i2c0"};
static const char *const REQ_LSM6[]   = {"i2c0"};

/*
 * Pin-based conflicts (from README pin usage):
 * - adc_pa123: PA1/PA2/PA3
 * - adc_pa1:   PA1
 * - uart1:     PA2/PA3
 * - i2s0_pa:   PA1/PA2/PA3/(PE3)
 * - i2s0_pe:   PA1/(PE5/PE6/PE3)
 */
static const char *const CON_ADC_PA123[] = {"adc_pa1", "uart1", "i2s0_pa", "i2s0_pe"};
static const char *const CON_ADC_PA1[]   = {"adc_pa123", "i2s0_pa", "i2s0_pe"};
static const char *const CON_UART1[]     = {"adc_pa123", "i2s0_pa"};
static const char *const CON_I2S_PA[]    = {"i2s0_pe", "adc_pa123", "adc_pa1", "uart1"};
static const char *const CON_I2S_PE[]    = {"i2s0_pa", "adc_pa123", "adc_pa1"};
static const char *const CON_UART2[]     = {"spi1"};
static const char *const CON_SPI1[]     = {"uart2"};


static const cfg_item_t ITEMS[] = {
    /* interface */
    {.id = "adc_pa123", .cat = CFG_CAT_INTERFACE, .title = "adc_pa123", .help = "Use PA1/PA2/PA3 as ADC pins", .min_rev_rank = -1, .max_rev_rank = -1, .conflicts = CON_ADC_PA123, .conflicts_count = 4},
    {.id = "adc_pa1",   .cat = CFG_CAT_INTERFACE, .title = "adc_pa1",   .help = "Use PA1 as ADC pin", .min_rev_rank = -1, .max_rev_rank = -1, .conflicts = CON_ADC_PA1, .conflicts_count = 3},
    {.id = "i2c0",      .cat = CFG_CAT_INTERFACE, .title = "i2c0",      .help = "Enable I2C0\n(PD0/PD12 as I2C pins)", .min_rev_rank = -1, .max_rev_rank = -1},
    {.id = "i2s0_pa",   .cat = CFG_CAT_INTERFACE, .title = "i2s0_pa",   .help = "Enable I2S0\n(PA1/PA2/PA3/PE3 routing)\n Only Available on Epass>=0.5", .min_rev_rank = 2, .max_rev_rank = -1, .conflicts = CON_I2S_PA, .conflicts_count = 4},
    {.id = "i2s0_pe",   .cat = CFG_CAT_INTERFACE, .title = "i2s0_pe",   .help = "Enable I2S0\n(PA1/PE5/PE6/PE3 routing)\n Only Available on Epass>=0.5", .min_rev_rank = 2, .max_rev_rank = -1, .conflicts = CON_I2S_PE, .conflicts_count = 3},
    {.id = "spi1",      .cat = CFG_CAT_INTERFACE, .title = "spi1",      .help = "Enable SPI1\n(PE7/PE8/PE9/PE10 as SPI pins)", .min_rev_rank = -1, .max_rev_rank = -1,.conflicts = CON_SPI1, .conflicts_count = 1},
    {.id = "uart1",     .cat = CFG_CAT_INTERFACE, .title = "uart1",     .help = "Enable UART1\n(PA2/PA3 as UART1 pins)", .min_rev_rank = -1, .max_rev_rank = -1, .conflicts = CON_UART1, .conflicts_count = 2},
    {.id = "uart2",     .cat = CFG_CAT_INTERFACE, .title = "uart2",     .help = "Enable UART2\n(PA7/PA8 as UART2 pins)", .min_rev_rank = -1, .max_rev_rank = -1,.conflicts = CON_UART2, .conflicts_count = 1},
    {.id = "usbhost",   .cat = CFG_CAT_INTERFACE, .title = "usbhost",   .help = "Set USB mode to USB Host", .min_rev_rank = -1, .max_rev_rank = -1},
    {.id = "usbhs",     .cat = CFG_CAT_INTERFACE, .title = "usbhs",     .help = "Enable USB2.0 High-Speed mode", .min_rev_rank = -1, .max_rev_rank = -1},

    /* ext */
    {.id = "cardkb",        .cat = CFG_CAT_EXT, .title = "cardkb",        .help = "Enable M5Stack CardKB support\n (requires i2c0)", .min_rev_rank = -1, .max_rev_rank = -1, .requires = REQ_CARDKB, .requires_count = 1},
    {.id = "lsm6ds3_pre0.4",.cat = CFG_CAT_EXT, .title = "lsm6ds3_pre0.4",.help = "Enable onboard IMU support for <=0.4\n (requires i2c0)", .min_rev_rank = -1, .max_rev_rank = 1, .requires = REQ_LSM6, .requires_count = 1},
};

const cfg_registry_t *cfg_registry_get(void) {
    static const cfg_registry_t reg = {
        .items = ITEMS,
        .count = sizeof(ITEMS) / sizeof(ITEMS[0]),
    };
    return &reg;
}

int device_rev_rank(device_rev_t rev) {
    switch (rev) {
        case DEVICE_REV_EPASS_0_2:     return 0;
        case DEVICE_REV_EPASS_0_3_0_4: return 1;
        case DEVICE_REV_EPASS_0_5:     return 2;
        case DEVICE_REV_EPASS_0_6:     return 3;
        default:                      return -1;
    }
}


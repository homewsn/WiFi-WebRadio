/*
 *  ======== ti_drivers_config.h ========
 *  Configured TI-Drivers module declarations
 *
 *  The macros defines herein are intended for use by applications which
 *  directly include this header. These macros should NOT be hard coded or
 *  copied into library source code.
 *
 *  Symbols declared as const are intended for use with libraries.
 *  Library source code must extern the correct symbol--which is resolved
 *  when the application is linked.
 *
 *  DO NOT EDIT - This file is generated for the CC3235SF
 *  by the SysConfig tool.
 */
#ifndef ti_drivers_config_h
#define ti_drivers_config_h

#define CONFIG_SYSCONFIG_PREVIEW

#define CONFIG_CC3235SF
#ifndef DeviceFamily_CC3220
#define DeviceFamily_CC3220
#endif

#include <ti/devices/DeviceFamily.h>

#include <stdint.h>

/* support C++ sources */
#ifdef __cplusplus
extern "C" {
#endif


/*
 *  ======== Crypto ========
 */

extern const uint_least8_t                  CONFIG_Crypto_0_CONST;
#define CONFIG_Crypto_0                     0
#define CONFIG_TI_DRIVERS_CRYPTO_COUNT      1

/*
 *  ======== GPIO ========
 */
extern const uint_least8_t CONFIG_GPIO_LED_0_CONST;
#define CONFIG_GPIO_LED_0 9

extern const uint_least8_t CONFIG_GPIO_LED_1_CONST;
#define CONFIG_GPIO_LED_1 10

extern const uint_least8_t CONFIG_GPIO_LED_2_CONST;
#define CONFIG_GPIO_LED_2 11

extern const uint_least8_t CONFIG_GPIO_VS1003_RESET_CONST;
#define CONFIG_GPIO_VS1003_RESET 17

extern const uint_least8_t CONFIG_GPIO_VS1003_XCS_CONST;
#define CONFIG_GPIO_VS1003_XCS 12

extern const uint_least8_t CONFIG_GPIO_VS1003_XDCS_CONST;
#define CONFIG_GPIO_VS1003_XDCS 13

extern const uint_least8_t CONFIG_GPIO_VS1003_DREQ_CONST;
#define CONFIG_GPIO_VS1003_DREQ 22

/* The range of pins available on this device */
extern const uint_least8_t GPIO_pinLowerBound;
extern const uint_least8_t GPIO_pinUpperBound;

/* LEDs are active high */
#define CONFIG_GPIO_LED_ON  (1)
#define CONFIG_GPIO_LED_OFF (0)

#define CONFIG_LED_ON  (CONFIG_GPIO_LED_ON)
#define CONFIG_LED_OFF (CONFIG_GPIO_LED_OFF)


/*
 *  ======== SPI ========
 */

extern const uint_least8_t              CONFIG_NWP_SPI_CONST;
#define CONFIG_NWP_SPI                  0
/*
 *  PICO: P07
 *  POCI: P06
 *  SCLK: P05
 */
extern const uint_least8_t              CONFIG_SPI_0_CONST;
#define CONFIG_SPI_0                    1
#define CONFIG_TI_DRIVERS_SPI_COUNT     2


/*
 *  ======== Timer ========
 */

extern const uint_least8_t                  CONFIG_TIMER_0_CONST;
#define CONFIG_TIMER_0                      0
extern const uint_least8_t                  CONFIG_TIMER_1_CONST;
#define CONFIG_TIMER_1                      1
extern const uint_least8_t                  CONFIG_TIMER_2_CONST;
#define CONFIG_TIMER_2                      2
#define CONFIG_TI_DRIVERS_TIMER_COUNT       3

/*
 *  ======== UART2 ========
 */

/*
 *  TX: P55
 *  RX: P57
 */
extern const uint_least8_t                  CONFIG_UART2_0_CONST;
#define CONFIG_UART2_0                      0
#define CONFIG_TI_DRIVERS_UART2_COUNT       1


/*
 *  ======== Watchdog ========
 */

extern const uint_least8_t                  CONFIG_WATCHDOG_0_CONST;
#define CONFIG_WATCHDOG_0                   0
#define CONFIG_TI_DRIVERS_WATCHDOG_COUNT    1


/*
 *  ======== Board_init ========
 *  Perform all required TI-Drivers initialization
 *
 *  This function should be called once at a point before any use of
 *  TI-Drivers.
 */
extern void Board_init(void);

/*
 *  ======== Board_initGeneral ========
 *  (deprecated)
 *
 *  Board_initGeneral() is defined purely for backward compatibility.
 *
 *  All new code should use Board_init() to do any required TI-Drivers
 *  initialization _and_ use <Driver>_init() for only where specific drivers
 *  are explicitly referenced by the application.  <Driver>_init() functions
 *  are idempotent.
 */
#define Board_initGeneral Board_init

#ifdef __cplusplus
}
#endif

#endif /* include guard */

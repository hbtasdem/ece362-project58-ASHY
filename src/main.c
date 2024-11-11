/**
 ******************************************************************************
 * @file    main.c
 * @author  Esharaqa Jahid, Sitara Iyer, Hilal B Tasdemir, Yashvi Agrawal
 * @date    Nov 8, 2024
 * @brief   ECE 362 Course Project 68 ASHY
 ******************************************************************************
 */

/*******************************************************************************/

/*******************************************************************************/

#include "stm32f0xx.h"

void set_char_msg(int, char);
void nano_wait(unsigned int);
void game(void);
void internal_clock();
void check_wiring();
void autotest();

int msg_index = 0;
uint16_t msg[8] = {0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700};
extern const char font[];

// uses spi1 for sd card
void init_sdcard_spi1()
{

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;

    GPIOB->MODER &= ~(GPIO_MODER_MODER10);                       // Clear mode
    GPIOB->MODER |= GPIO_MODER_MODER10_1;                        // Set to Alternate Function for PB10 (SCK)
    GPIOC->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);    // Clear modes
    GPIOC->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1); // Set to Alternate Function for PC2 (MOSI) and PC3 (MISO)

    GPIOB->AFR[1] |= (5 << ((10 - 8) * 4));           // Set AF5 for PB10
    GPIOC->AFR[0] |= (5 << (2 * 4)) | (5 << (3 * 4)); // Set AF5 for PC2 and PC3

    SPI2->CR1 &= ~SPI_CR1_SPE;

    SPI2->CR1 |= SPI_CR1_MSTR;
    SPI2->CR1 |= SPI_CR1_BR;                  // max baud rate
    SPI2->CR2 |= SPI_CR2_DS_3 | SPI_CR2_DS_0; // sets data size to 8 bits
    SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;   // enables software slave management and internal slave select
    SPI2->CR2 |= SPI_CR2_FRXTH;               // sets FIFO reception threshold for immediate 8-bit release

    SPI2->CR1 |= SPI_CR1_SPE; // enables SPI2

    GPIOC->MODER &= ~GPIO_MODER_MODER11;
    GPIOC->MODER |= GPIO_MODER_MODER11_0; // PC11 as output
    disable_sdcard();                     // In addition, configure PC11 as a GPIO output and set its ODR entry to be a 1 by calling disable_sdcard().
}

void disable_sdcard()
{
    // This function should set PC10 high to disable the SD card.
}

void enable_sdcard()
{
    // This function should set PC11 low to enable the SD card.
}
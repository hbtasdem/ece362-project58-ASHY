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
#include <math.h> // for M_PI
#include <stdint.h>
#include <stdio.h>

void set_char_msg(int, char);
void nano_wait(unsigned int);
void game(void);
void internal_clock();
void check_wiring();
// void autotest();

int msg_index = 0;
uint16_t msg[8] = {0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700};
extern const char font[];
uint8_t col;

#define N 1000
#define RATE 20000
short int wavetable[N];
int step0 = 0;
int offset0 = 0;
int step1 = 0;
int offset1 = 0;
uint32_t volume = 2048;

// uses spi2 for sd card
void init_spi2_slow()
{

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB1ENR_SPI2EN;

    GPIOB->MODER &= ~0xfc0;
    GPIOB->MODER |= 0xa80;

    GPIOB->AFR[0] |= (5 << (3 * 4)) | (5 << (4 * 4)) | (5 << (5 * 4));

    SPI2->CR1 &= ~SPI_CR1_SPE;
    SPI2->CR1 |= SPI_CR1_MSTR;
    SPI2->CR2 |= SPI_CR2_DS;
    SPI2->CR1 |= SPI_CR1_BR;                // max baud rate
    SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; // enables software slave management and internal slave select
    SPI2->CR2 |= SPI_CR2_FRXTH;             // sets FIFO reception threshold for immediate 8-bit release

    SPI2->CR1 |= SPI_CR1_SPE; // enables SPI1
}

void disable_sdcard()
{
    GPIOB->BSRR |= GPIO_BSRR_BS_2;
}

void enable_sdcard()
{
    GPIOB->BRR |= GPIO_BRR_BR_2;
}

void init_sdcard_io()
{
    init_spi2_slow();
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~0x30;
    GPIOB->MODER |= 0x10; // Configures PB2 as an output
    disable_sdcard();
}

void sdcard_io_high_speed()
{
    SPI2->CR1 &= ~SPI_CR1_SPE;
    // NOT SURE IF THIS IS THE CORRECT VALUE
    SPI2->CR1 |= SPI_CR1_BR_2; // Set the SPI1 Baud Rate register so that the clock rate is 12 MHz.
    //(You may need to set this lower if your SD card does not reliably work at this rate.)
    SPI2->CR1 |= SPI_CR1_SPE;
}

//===========================================================================
// init_wavetable()
// Write the pattern for a complete cycle of a sine wave into the
// wavetable[] array.
//===========================================================================
unsigned char buffer[10];
FILE *ptr;

// int __io_putchar(int ch)
// {
//     // Code to send `ch` to UART or other debug interface
//     return ch;
// }

void init_wavetable(void)
{
    ptr = fopen("test.bin", "rb"); // r for read, b for binary
    if (ptr == NULL)
    {
        return;
    }

    fread(buffer, sizeof(buffer), 1, ptr);
    fclose(ptr);
    for (int i = 0; i < N && i < sizeof(buffer); i++)
    {
        wavetable[i] = buffer[i];
    }
    // wavetable[i] = 32767 * sin(2 * M_PI * i / N);
    // TODO
    // read the data from a file on the sd card
}

//============================================================================
// set_freq()
//============================================================================
void set_freq(int chan, float f)
{
    if (chan == 0)
    {
        if (f == 0.0)
        {
            step0 = 0;
            offset0 = 0;
        }
        else
            step0 = (f * N / RATE) * (1 << 16);
    }
}

void setup_dac(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= 0x300;
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC1->CR &= ~DAC_CR_TSEL1;
    DAC1->CR |= DAC_CR_TEN1;
    DAC1->CR |= DAC_CR_EN1;
}

//============================================================================
// Timer 6 ISR
//============================================================================
// Write the Timer 6 ISR here.  Be sure to give it the right name.
void TIM6_IRQHandler()
{
    TIM6->SR &= ~TIM_SR_UIF; // Remember to acknowledge the interrupt here!
    offset0 += step0;

    if (offset0 >= (N << 16))
    {
        offset0 -= (N << 16);
    }

    int samp = wavetable[offset0 >> 16];
    samp *= volume;
    samp = samp >> 17;
    samp += 2048;
    DAC->DHR12R1 = samp;
}
//============================================================================
// init_tim6()
//============================================================================
void init_tim6(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = (480000 / RATE) - 1;
    TIM6->ARR = (100) - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] = 1 << TIM6_IRQn;
    TIM6->CR1 |= TIM_CR1_CEN;
    TIM6->CR2 &= ~TIM_CR2_MMS_0;
    TIM6->CR2 |= TIM_CR2_MMS_1;
    TIM6->CR2 &= ~TIM_CR2_MMS_2;
}

int main(void)
{
    internal_clock();
    // Initialize the display to something interesting to get started.
    msg[0] |= font['E'];
    msg[1] |= font['C'];
    msg[2] |= font['E'];
    msg[3] |= font[' '];
    msg[4] |= font['3'];
    msg[5] |= font['6'];
    msg[6] |= font['2'];
    msg[7] |= font[' '];

    return 1;

    // Comment this for-loop before you demo part 1!
    // Uncomment this loop to see if "ECE 362" is displayed on LEDs.
    for (;;)
    {
        asm("wfi");
    }
    // End of for loop

    // Demonstrate part 1
// #define SCROLL_DISPLAY
#ifdef SCROLL_DISPLAY
    for (;;)
        for (int i = 0; i < 8; i++)
        {
            print(&"Hello...Hello..."[i]);
            nano_wait(250000000);
        }
#endif

        // Demonstrate part 2
// #define SHOW_KEY_EVENTS
#ifdef SHOW_KEY_EVENTS
    show_keys();
#endif

    // Demonstrate part 3
// #define SHOW_VOLTAGE
#ifdef SHOW_VOLTAGE
    for (;;)
    {
        printfloat(2.95 * volume / 4096);
    }
#endif

    init_wavetable();
    setup_dac();
    init_tim6();

// #define ONE_TONE
#ifdef ONE_TONE
    for (;;)
    {
        float f = getfloat();
        set_freq(0, f);
    }
#endif

    // demonstrate part 4
// #define MIX_TONES
#ifdef MIX_TONES
    for (;;)
    {
        char key = get_keypress();
        if (key == 'A')
            set_freq(0, getfloat());
        if (key == 'B')
            set_freq(1, getfloat());
    }
#endif
}
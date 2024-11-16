/**
 ******************************************************************************
 * @file    main.c
 * @author  Esharaqa Jahid, Sitara Iyer, Hilal B Tasdemir, Yashvi Agrawal
 * @date    Nov 8, 2024
 * @brief   ECE 362 Course Project 68 ASHY
 ******************************************************************************
 */

/*******************************************************************************/
const char *username = "htasdem";
/*******************************************************************************/

#include "stm32f0xx.h"
#include <math.h> // for M_PI
#include <stdint.h>
#include <stdio.h>
#include "fifo.h"
#include "tty.h"
#include "commands.h"
#include "lcd.h"
#include "ff.h"

void internal_clock();

// DAC
#define N 1000
#define RATE 20000
#define BUFFER_SIZE 1024          // Adjust size as needed for your buffer
#define WAV_FILE_PATH "sine8.wav" // Path to your WAV file

short int wavetable[N];
int step0 = 0;
int offset0 = 0;
uint8_t col; // the column being scanned
uint32_t volume = 2048;
uint16_t msg[8] = {0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700};
extern const char font[];
void print(const char str[]);
float getfloat(void); // read a floating-point number from keypad
void printfloat(float f);

unsigned char buffer[10];
FILE *ptr;
FATFS fs;    // FatFS file system object
FIL file;    // File object to store information about the open file
FRESULT res; // Result of operations
UINT br = 0;

#define ONE_TONE
// #define MIX_TONES

int main(void)
{
    internal_clock();
    init_usart5();
    setbuf(stdin, 0);
    setbuf(stdout, 0);
    setbuf(stderr, 0);
    command_shell();
    init_wavetable();
    setup_dac();
    init_tim6();

    printf("test");
    for (;;)
    {
        char c = getchar();
        putchar(c);
    }

#ifdef ONE_TONE
    for (;;)
    {
        // float f = getfloat();
        set_freq(0, 1000);
    }
#endif

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

    return 1;
}

void init_usart5()
{
    // TODO
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    RCC->AHBENR |= RCC_AHBENR_GPIODEN;

    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;

    GPIOC->MODER &= ~(0x3000000); // configure pin PC12 to be routed to USART5_TX
    GPIOC->MODER |= 0x2000000;
    GPIOC->AFR[1] |= 0x20000;

    GPIOD->MODER &= ~(0xF0); // configure pin PD2 to be routed to USART5_RX
    GPIOD->MODER |= 0x20;
    GPIOD->AFR[0] |= 0x200;

    // Configure USART5:
    USART5->CR1 &= ~USART_CR1_UE;

    USART5->CR1 &= ~USART_CR1_M0;
    USART5->CR1 &= ~USART_CR1_M1;

    USART5->CR2 &= ~USART_CR2_STOP_0;
    USART5->CR2 &= ~USART_CR2_STOP_1;

    USART5->CR1 &= ~USART_CR1_PCE;
    USART5->CR1 &= ~USART_CR1_OVER8;

    USART5->BRR = 0x1A1;

    USART5->CR1 |= USART_CR1_TE;
    USART5->CR1 |= USART_CR1_RE;
    USART5->CR1 |= USART_CR1_UE;

    while (!(USART5->ISR & USART_ISR_TEACK) | !(USART5->ISR & USART_ISR_REACK))
        ;
}

int __io_putchar(int ch)
{
    // Code to send `ch` to UART or other debug interface
    return ch;
}

void init_spi1_slow()
{
    // PB3 (SCK), PB4 (MISO), and PB5 (MOSI)
    // AFR 0

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    GPIOB->MODER &= ~0xFC0;
    GPIOB->MODER |= 0xA80;
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL5);

    SPI1->CR1 &= ~SPI_CR1_SPE;

    SPI1->CR1 |= (SPI_CR1_BR | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI);
    SPI1->CR2 |= 0x700;
    SPI1->CR2 &= ~SPI_CR2_DS_3;
    SPI1->CR2 |= SPI_CR2_FRXTH;

    SPI1->CR1 |= SPI_CR1_SPE;
}

void enable_sdcard()
{
    GPIOB->BRR |= GPIO_BRR_BR_2;
}

void disable_sdcard()
{
    GPIOB->BSRR |= GPIO_BSRR_BS_2;
}

void init_sdcard_io()
{
    init_spi1_slow();
    GPIOB->MODER |= 0x10;
    disable_sdcard();
}

void sdcard_io_high_speed()
{
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= 0x8;
    SPI1->CR1 &= ~(SPI_CR1_BR_2 | SPI_CR1_BR_1);
    SPI1->CR1 |= SPI_CR1_SPE;
}

void setup_dma(void)
{

    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= DMA_CCR_EN;
    DMA1_Channel5->CMAR = (uint32_t)msg;
    DMA1_Channel5->CPAR = (uint32_t)&GPIOB->ODR;
    DMA1_Channel5->CNDTR = 8; // check if matches the data transfer
    // DMA1_Channel5->CCR = 0;
    DMA1_Channel5->CCR |= DMA_CCR_DIR;
    DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
    DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;
    DMA1_Channel5->CCR |= DMA_CCR_MINC;
    DMA1_Channel5->CCR |= DMA_CCR_CIRC;
    DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;

    // asm("wfi");
    // for (;;)
    //     ;
}

void enable_dma(void)
{
    DMA1_Channel5->CCR |= DMA_CCR_EN;
}

//============================================================================
// init_tim15() for DMA
//============================================================================
void init_tim15(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->PSC = 4799;
    TIM15->ARR = 9;
    TIM15->DIER |= TIM_DIER_UDE;
    TIM15->CR1 = TIM_CR1_CEN;
}
//===========================================================================
// init_wavetable()
// Write the pattern for a complete cycle of a sine wave into the
// wavetable[] array.
//===========================================================================
void init_wavetable(void)
{
    // f_open(ptr, "test.bin", 'r'); // r for read, b for binary
    // if (ptr == NULL)
    // {
    //     printf("k");
    //     return;
    // }

    // UINT br = 0;
    // f_read(ptr, buffer, sizeof(buffer), &br);
    // // fread(buffer, sizeof(buffer), 1, ptr);
    // f_close(ptr);
    // for (int i = 0; i < N && i < sizeof(buffer); i++)
    // {
    //     wavetable[i] = buffer[i];
    //     printf("vibes: %c", buffer[i]);
    // }
    // // wavetable[i] = 32767 * sin(2 * M_PI * i / N);
    // // TODO
    // // read the data from a file on the sd card
    // Mount the SD card
    if (f_mount(&FatFs, "", 1) != FR_OK)
    {
        printf("SD Card mount failed\n");
        return;
    }

    // Open the WAV file in read mode
    if (f_open(&file, WAV_FILE_PATH, FA_READ) != FR_OK)
    {
        printf("Failed to open WAV file\n");
        return;
    }

    // Read the WAV file header (first 44 bytes for standard WAV header)
    BYTE header[44]; // Standard WAV header size is 44 bytes
    if (f_read(&file, header, 44, &br) != FR_OK || br != 44)
    {
        printf("Error reading WAV header\n");
        f_close(&file);
        return;
    }

    // Now skip the header and start reading audio data
    // Assuming the WAV file is PCM data in the format: 8-bit, mono, 24000 Hz
    // The actual audio data starts after the header, so use f_read to fetch that data
    while (f_read(&file, buffer, sizeof(buffer), &br) == FR_OK && br > 0)
    {
        // Process the audio data in the buffer (e.g., DMA, DAC playback)
        for (int i = 0; i < br; i++)
        {
            wavetable[i] = buffer[i];               // Assuming wavetable[] is set up correctly
            printf("Read sample: %d\n", buffer[i]); // Print the byte value (adjust this if needed)
        }
    }

    // Close the file
    f_close(&file);
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
void TIM6_DAC_IRQHandler()
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
    NVIC->ISER[0] = 1 << TIM6_DAC_IRQn;
    TIM6->CR1 |= TIM_CR1_CEN;
    TIM6->CR2 &= ~TIM_CR2_MMS_0;
    TIM6->CR2 |= TIM_CR2_MMS_1;
    TIM6->CR2 &= ~TIM_CR2_MMS_2;
}


// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator mode selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with OSCFRQ= 32 MHz and CDIV = 1:1)
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (FSCM timer enabled)

// CONFIG2
#pragma config MCLRE = ON       // Master Clear Enable bit (MCLR pin is Master Clear function)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config LPBOREN = OFF    // Low-Power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = ON       // Brown-out reset enable bits (Brown-out Reset Enabled, SBOREN bit is ignored)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (VBOR) set to 1.9V on LF, and 2.45V on F Devices)
#pragma config ZCD = OFF        // Zero-cross detect disable (Zero-cross detect circuit is disabled at POR.)
#pragma config PPS1WAY = ON     // Peripheral Pin Select one-way control (The PPSLOCK bit can be cleared and set only once in software)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a reset)

// CONFIG3
#pragma config WDTCPS = WDTCPS_10// WDT Period Select bits (Divider ratio 1:32768)
#pragma config WDTE = ON        // WDT operating mode (WDT enabled regardless of sleep; SWDTEN ignored)
#pragma config WDTCWS = WDTCWS_6// WDT Window Select bits (window always open (100%); no software control; keyed access required)
#pragma config WDTCCS = LFINTOSC// WDT input clock selector (WDT reference clock is the 31.0kHz LFINTOSC output)

// CONFIG4
#pragma config BBSIZE = BB512   // Boot Block Size Selection bits (512 words boot block size)
#pragma config BBEN = OFF       // Boot Block Enable bit (Boot Block disabled)
#pragma config SAFEN = OFF      // SAF Enable bit (SAF disabled)
#pragma config WRTAPP = OFF     // Application Block Write Protection bit (Application Block not write protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block not write protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration Register not write protected)
#pragma config WRTSAF = OFF     // Storage Area Flash Write Protection bit (SAF not write protected)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low Voltage programming enabled. MCLR/Vpp pin function is MCLR.)

// CONFIG5
#pragma config CP = OFF         // UserNVM Program memory code protection bit (UserNVM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "sysconfig.h"
#include "hoermann.h"
#include "esp_interface.h"


static void pins_init(void)
{
  LATA   = 0b00000000;
  TRISA  = 0b00001000;
  ANSELA = 0b00000000;
  /*           |||||'-- RA0 - PGD */
  /*           ||||'--- RA1 - PGC */
  /*           |||'---- RA2 - not used */
  /*           ||'----- RA3 - !MCLR */
  /*           |'------ RA4 - not used */
  /*           '------- RA5 - not used */

  LATC   = 0b00010001;
  TRISC  = 0b00100010;
  ANSELC = 0b00000000;
  /*           |||||'-- RC0 - PIC_TX2 */
  /*           ||||'--- RC1 - PIC_RX2 */
  /*           |||'---- RC2 - PIC_DE */
  /*           ||'----- RC3 - !PIC_RE */
  /*           |'------ RC4 - PIC_TX1 */
  /*           '------- RC5 - PIC_RX2 */

  /* RS485-Interface */
  RC4PPS = 0x0F;      /* RC4 - Uart 1 Tx */
  RX1DTPPS = 0b10101; /* RC5 - Uart 1 Rx */
  
  /* ESP-Interface */
  RC0PPS = 0x11;      /* RC0 - Uart 2 Tx */
  RX2DTPPS = 0b10001; /* RC1 - Uart 2 Tx */
}

static void timer_init(void)
{
  /* Caution: Errata 5.1 
   * Operation of Timer0 is Incorrect when FOSC/4 is Used as the Clock Source
   * Clearing the T0ASYNC bit in the T0CON1 register when Timer0 is configured
   * to use FOSC/4 may cause incorrect behavior. This issue is only valid when
   * FOSC/4 is used as the clock source.
   * Work around: Set the T0ASYNC bit in the T0CON1 register when using FOSC/4
   * as the Timer0 clock. */
  
  TMR0L = 0;
  TMR0H = ((uint8_t)((1e-3 * FCY)/256))-1;
  T0CON1 = 0b01111000;
  /*         ||||''''-- T0CKPS - Prescaler Rate Select bit 1:256 */
  /*         |||'------ T0ASYNC - The input to the TMR0 counter is not synchronized to system clocks */
  /*         '''------- T0CS - Timer0 Clock Source select bits HFINTOSC */
  T0CON0 = 0b10000000;
  /*         | ||''''-- T0OUTPS - Timer0 output postscaler (divider) select bits 1:1 */
  /*         | |'------ T016BIT - Timer0 is an 8-bit timer */
  /*         | '------- T0OUT - Timer0 Output bit (read-only) */
  /*         '--------- T0EN - The module is enabled and operating */
}

int main(void)
{
  pins_init();
  timer_init();
  hoermann_init();
  esp_interface_init();
  
   /* Enable interrupts */
  PEIE = 1;
  GIE = 1;
  
  while(1)
  {
    if(TMR0IF == 1)
    {
      /* 1ms Task */
      TMR0IF = 0;
      CLRWDT();
      hoermann_run();
      esp_interface_run();
    }
  }
  
  return 0;
}

void __interrupt() isr(void)
{
  if(RC1IF == 1)
  {
    hoermann_rx_isr();
  }
  if(TX1IF == 1)
  {
    hoermann_tx_isr();
  }
  if(RC2IF == 1)
  {
    esp_rx_isr();
  }
  if(TX2IF == 1)
  {
    esp_tx_isr();
  }
}

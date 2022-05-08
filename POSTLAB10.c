/* 
 * File:   POSTLAB10.c
 * Author: diego
 *
 * Created on 5 de mayo de 2022, 05:27 PM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT        // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF                   // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF                  // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF                  // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF                     // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF                    // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF                  // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF                   // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF                  // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF                    // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V               // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF                    // Flash Program Memory Self Write Enable bits (Write protection off)

#include <xc.h>
#include <stdint.h>
#include<stdio.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint8_t inicio = 0;                 // indica si hubo un cambio en el valor ingresado
uint8_t reinicio = 1;               // indica  si se reinicio o no el menú
uint8_t accion = 0;                 // almacena el valor del teclado ingresado en consola
uint8_t ult_ASCII = 0;              // ultimo valor ingresado de ASCII

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void Menu(void);                 
void setup(void);                       
void mensaje(unsigned char *texto);   // Función para mostrar texto en consola
void espacio(void);                    // Función para dar espacio en consola    

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.RCIF){              // badera de TX/RX
        accion = RCREG;             // valor ingresado en el teclado para decisión de acción a realizar
        inicio = 1;                 // ingreso a menu
    }
    if(PIR1bits.ADIF){              // Bandera de ADC
        if(ADCON0bits.CHS == 0){    // Verificación de interrupción por canal AN0
            PORTB = ADRESH;         // Asignación de valor del POT en puerto B
        }
        PIR1bits.ADIF = 0;          // Limpieza de bandera de interrupción del módulo ADC
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();                    
    while(1){     
        while(reinicio == 1){   // menu si/no
            Menu();             
            reinicio = 0;       // Apagar bandera de reinicio
            inicio = 0;         // Apagar bandera de cambio en el ingreso
        }
        
        if(inicio == 1){            // inicio de menu de deciones
            while (TXIF != 1);      // Verificación de registro TXREG
            TXREG = accion;         // ingreso de valor de acción a realizar
            
            if(accion == 49){       // Opción 1
                espacio();
                mensaje("Valor de potenciómetro (0-255) en PORTB.");
                espacio();
                mensaje("Presione cualquier tecla para regresar al menu.");
                espacio();
                
                while(accion == 49){                  //Lectura de pot
                    if(ADCON0bits.GO == 0){          
                        ADCON0bits.CHS = 0b0000;     
                        __delay_us(40);              
                        ADCON0bits.GO = 1;           
                    }
                }
                
            }
            else if(accion == 50){      // Opción 2
                espacio();
                mensaje("Valor ingresado por teclado en ASCII, visible en PORTB.");
                espacio();
                mensaje("Ingrese '0' para reiniciar menu.");
                espacio();
                
                while(accion != 48){            // ENVÍO DE ASCCI A PORTB
                    PORTB = accion;                 // Asignación de valor al PORTB
                }
            } 
            else{                       // Protección contra graciosos
                ult_ASCII = accion;
                espacio();
                mensaje("invalido. Presione cualquier tecla");
                espacio();
                while(ult_ASCII == accion);
            }
            
            while (TXIF != 1);          // Verificación de registro TXREG
            TXREG = 0x0C;               // Asignación de valor ingresado en registro TXREG
            reinicio = 1;                // Activar bandera de reinicio de menú
        }        
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    ANSEL = 0b001;              // AN0 como entrada analógicas
    ANSELH = 0x00;              // I/O digitales
    TRISA = 0b001;              // AN0 entrada
    TRISB = 0x00;               // PORTB salida
    PORTA = 0x00;               // Limpieza del PORTA
    PORTB = 0x00;               // Limpieza del PORTB
    
    // Oscilador
    OSCCONbits.IRCF = 0b0100;   // 1 MHz
    OSCCONbits.SCS = 1;         // Interno
    
    // ADC
    ADCON0bits.ADCS = 0b01;         // Fosc/8
    ADCON1bits.VCFG0 = 0;           // VDD
    ADCON1bits.VCFG1 = 0;           // VSS
    ADCON0bits.CHS = 0b0000;        // CH AN0
    ADCON1bits.ADFM = 0;            // justificado a la izquierda
    ADCON0bits.ADON = 1;            // Habilitación del ADC
    __delay_us(40);                 // timing acción ADC
    
    // Comunicación serial
    // SYNC = 0, BRGH = 1, BRG16 = 1, SPBRG=25 <- Valores de tabla 12-5
    TXSTAbits.SYNC = 0;         // Comunicación ascincrona (full-duplex)
    TXSTAbits.BRGH = 1;         // Baud rate de alta velocidad 
    BAUDCTLbits.BRG16 = 1;      // 16-bits para generar el baud rate
    SPBRGH = 0;
    SPBRG = 25;                 // Baud rate ~9600, error -> 0.16%
    RCSTAbits.SPEN = 1;         // Habilitar comunicacion serial
    RCSTAbits.RX9 = 0;          // Únicamente 8 bits
    TXSTAbits.TXEN = 1;         // Habilitar transmisor
    RCSTAbits.CREN = 1;         // Habilitar receptor
    INTCONbits.GIE = 1;         // Habilitar interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitar interrupciones de periféricos
    PIE1bits.RCIE = 1;          // Habilitar interrupciones de recepción
    PIE1bits.ADIE = 1;          // Habilitamos interrupcion de ADC
    PIR1bits.ADIF = 0;          // Limpiamos bandera de ADC
   
}

/*------------------------------------------------------------------------------
 * FUNCIONES 
 ------------------------------------------------------------------------------*/
// Menú de ingresos
void Menu(void)
{ 
    mensaje("Qué desea realizar:");    
    espacio();
    mensaje("1. Lectura de pot");
    espacio();
    mensaje("2. Valor en ASCII de valores teclado");
    espacio();
}

/*------------------------------------------------------------------------------
 * SUBFUNCIONES (FACILITADORES DE COMUNICACIÓN SERIAL)
 ------------------------------------------------------------------------------*/

// Función para texto en comunicación serial 
void mensaje(unsigned char *texto){    
    while (*texto != '\0'){
        while (TXIF != 1);
        TXREG = *texto;
        *texto++;
    }    
}

// Función para espacio en comunicación serial
void espacio(void)
{ 
    while (TXIF != 1);
    TXREG = 0x0D;  
}


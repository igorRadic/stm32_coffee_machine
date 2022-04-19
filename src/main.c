/*
******************************************************************************
File:     main.c
Info:     Projekt iz kolegija UGRS - Aparat za kavu

pinovi:   PB6 - UART - Tx - bijela žica sa TTL to USB kabla
	      PB7 - UART - Rx - zelena žica sa TTL to USB kabla

	      PC6 - servo motor za dodavanje kave
	      PC7 - servo motor za dodavanje šeæera
	      PC8 - servo motor za dodavanje mlijeka

	      PD8 - relej za pumpu za vodu
		  PD9 - relej za grijaè za vodu

		  PE15 - temperaturni senzor

******************************************************************************
*/


/* Includes */
#include "stm32f4xx.h"
#include "uart.h"
#include "defines.h"
#include <string.h>
#include <stdio.h>
#include <tm_stm32f4_onewire.h>
#include <tm_stm32f4_ds18b20.h>

/* Private variables */
volatile int mjera_kave = 900;    // vrijednosti Delay-a za koji je otvor otvoren
volatile int mjera_secera = 1300;
volatile int mjera_mlijeka = 600;
volatile int mjera_vode = 4200;

/* Private functions */
// ------------------------------------ funkcije vezane za komunikaciju putem UART-a ----------------------------------------//



void ucitaj_narudzbu(char* narudzba){ // uèitavanje narudžbe putem UART-a, narudžba se sprema u char polje narudžba

	char znak;
	int counter = 0;

	while(1){

		znak = USART_GetChar();       // primi znak

		if(znak > 47 && znak < 58){   // provjeri jel broj

			narudzba[counter] = znak; // spremi broj u polje od 3 elementa
			counter++;

			if(counter == 3){

				return;

			}

		}

	}

}

void ispis_narudzbe(char* narudzba){ // ispis narudžbe putem UART-a radi lakseg debugiranja

	USART_PutString("\n");
	USART_PutString("=====================\n");
	USART_PutString("Narudzba       : ");

	for(int i = 0; i < 3; i++){      // prikaz dobivena 3 broja

		USART_PutChar(narudzba[i]);

	}

	USART_PutString("\n");

	char buff[70];
	sprintf(buff, "Jacina kave je   : %d \n" , narudzba[0] - 48);
	USART_PutString(buff);
	sprintf(buff, "Kolicina secera  : %d \n" , narudzba[1] - 48);
	USART_PutString(buff);
	sprintf(buff, "Kolicina mlijeka : %d \n" , narudzba[2] - 48);
	USART_PutString(buff);
	USART_PutString("=====================\n");

}

void oglasi_pripravnost_korisniku(){

	USART_PutChar('9');
	USART_PutString("\n");

}

void obavijesti_kava_je_spremna(){

	USART_PutChar('7');
	USART_PutString("\n");

}

// -------------------------------------- funkcije vezane za digitalne izlaze za releje -------------------------------------//


void config_releji(void){

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); // ukljuèivanje takta za port D
	GPIO_InitTypeDef GPIO_InitStructure;                  // inicijalizacijska struktura

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 |GPIO_Pin_9; // 8 - pumpa, 9 - grijac
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;		  // izlazni pinovi
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;        // push-pull konfiguracija
	GPIO_Init(GPIOD, &GPIO_InitStructure);                // zapisivanje u konfiguracijske registre porta D

}

void natoci_vodu(){

	GPIO_ResetBits(GPIOD,GPIO_Pin_8); // ukljuci pumpu za vodu

	Delayms(mjera_vode);

	GPIO_SetBits(GPIOD,GPIO_Pin_8);   // iskljuci pumpu za vodu

}

void ukljuci_grijac(){

	GPIO_ResetBits(GPIOD,GPIO_Pin_9);

}

void iskljuci_grijac(){

	GPIO_SetBits(GPIOD,GPIO_Pin_9);

}

// ----------------------------------------------- funkcije za PWM servo motore ---------------------------------------------//


void config_PWM(void){

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);   // Ukljuèivanje takta za port C
    GPIO_InitTypeDef GPIO_InitStructurePWM;                 // Inicijalizacijska struktura za GPIO

    GPIO_InitStructurePWM.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8; // Inicijalizacija PC6, PC7 i PC8 (TIM3 Ch1, Ch2 i Ch3)
    GPIO_InitStructurePWM.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructurePWM.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructurePWM.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructurePWM.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructurePWM);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3); // Alternativna funkcija pina PB6, PB7 i PB8 je PWM signal s TIM3
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);


    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    // Ukljuèivanje takta za TIM3
    TIM_TimeBaseInitTypeDef TIM3_TimeBaseStructure;         // Struktura za PWM GPIO

    // Podešavanje TIM3 vremenske baze - frekvencija PWM signala je 50Hz
    TIM3_TimeBaseStructure.TIM_Period = 19999; // ne smije biti veci od (2^16)-1 = 65535, da se to ne dogodi mozemo povecat prescaler
    TIM3_TimeBaseStructure.TIM_Prescaler = 84;
    TIM3_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM3_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM3_TimeBaseStructure);
    TIM_Cmd(TIM3, ENABLE);                                  // Pokreni timer


    // Podešavanje karakteristika PWM signala
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // Podešavanje
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 0;              // poèetna vrijednost duty cycle-a = 0%

    // PWM signal na kanal 1
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // PWM signal na kanal 2
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // PWM signal na kanal 3
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

}

void pusti_kavu(){

	TIM3->CCR1 = 1200; // otvori otvor , vrijednosti u mikrosekundama za duty cycle

	Delayms(mjera_kave);

	TIM3->CCR1 = 700; // otvori otvor

	Delayms(mjera_kave);

}

void pusti_secer(){

	TIM3->CCR2 = 1200;

	Delayms(mjera_secera);

	TIM3->CCR2 = 600;

	Delayms(mjera_secera);

}

void pusti_mlijeko(){

	TIM3->CCR3 = 1500;

	Delayms(mjera_mlijeka);

	TIM3->CCR3 = 2200;

	Delayms(mjera_mlijeka);

}

// ------------------------------------------- funkcije za temperaturni senzor ----------------------------------------------//


void config_temperaturni_senzor(TM_OneWire_t* OneWire1, uint8_t* adresa_senzora){

	TM_OneWire_Init(OneWire1, GPIOE, GPIO_Pin_15);     // Inicijalizacija OneWire instance za pin PE15

	uint8_t senzor_je_spojen;

	senzor_je_spojen = TM_OneWire_First(OneWire1);     // provjera ima li spojenih OneWire senzora

	if(senzor_je_spojen){

	TM_OneWire_GetFullROM(OneWire1, adresa_senzora);   // dohvaæanje 8 bajtne rom adrese spojenog senzora na temelju koje se može manipulirati odreðenim senzorom

	TM_DS18B20_SetResolution(OneWire1, adresa_senzora, TM_DS18B20_Resolution_9bits); // postavljanje 9 bitne rezolucije senzora

	TM_DS18B20_DisableAlarmTemperature(OneWire1, adresa_senzora); // onemoguæavanje alarma za postizanje odreðene temperature

	}

}

int Dohvati_temperaturu(TM_OneWire_t* OneWire1, uint8_t* adresa_senzora){

	float temperatura;

	TM_DS18B20_StartAll(OneWire1); // poèetak pretvorbe temperature u svim senzorima koji su spojeni na pin iz OneWire strukture

	while (!TM_DS18B20_AllDone(OneWire1)); // èekanje dok svi senzori koji su spojeni ne obave pretvorbu temeperature

	if (TM_DS18B20_Read(OneWire1, adresa_senzora, &temperatura)){ // èitanje temperature sa senzora

		return (int)temperatura;

	}

	return 0;

}


int main(void)
{

	char narudzba[3];
	TM_OneWire_t OneWire1;
	uint8_t adresa_senzora[8];
	int temperatura_vode;

 	SystemInit();
	USART1_Init();
	config_PWM();
	TM_DELAY_Init();
	config_releji();
	config_temperaturni_senzor(&OneWire1, adresa_senzora);

	GPIO_SetBits(GPIOD,GPIO_Pin_8); // postavljanje releja u poèetni polozaj
	GPIO_SetBits(GPIOD,GPIO_Pin_9);

	Delayms(5000); // delay kako se ne bi primila neodreðena poruka Wi-fi modula pri ukljuèivanju aparata za kavu

	oglasi_pripravnost_korisniku();

	/* Infinite loop */
	while (1)
	{

		ucitaj_narudzbu(narudzba);

		//ispis_narudzbe(narudzba);

		for(int i = 0; i < narudzba[0] - 48; i++){

			pusti_kavu();

		}

		for(int i = 0; i < narudzba[1] - 48; i++){

			pusti_secer();

		}

		for(int i = 0; i < narudzba[2] - 48; i++){

			pusti_mlijeko();

		}

		temperatura_vode = Dohvati_temperaturu(&OneWire1, adresa_senzora);  // provjera temperature vode te grijanje vode dok temperatura ne dosegne 75 stupnjeva

		if(temperatura_vode <= 75){

			ukljuci_grijac();

			while(temperatura_vode <= 75){

			temperatura_vode = Dohvati_temperaturu(&OneWire1, adresa_senzora);
			Delayms(1000);

			}

		iskljuci_grijac();

		}

		natoci_vodu();

		Delayms(7000); // Sigurnosni delay kako bi sva voda izašla iz cijevi

		obavijesti_kava_je_spremna();

		Delayms(1000);

		oglasi_pripravnost_korisniku();


	}

}





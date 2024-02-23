#ifndef F_CPU
#define F_CPU 16000000UL	//Defini��o da Frequ�ncia de Funcionamento do uProcessador -> 16MHz
#endif
//-------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------- Defini��es de Bibliotecas-----------------------------------------------------
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>
#include <string.h>
#define BAUD 9600
#define UBBRR_US F_CPU/16/BAUD-1
#define DS3231_REGISTO_ESCRITA 0xD0
#define DS3231_REGISTO_LEITURA 0xD1
//-------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------ Vari�veis Globais ------------------------------------------------------------
typedef struct rtc_dados		//Estrutura referente a informa��es provenientes do RTC
{							
	int segundos;
	int minutos;
	int horas;
	int diasemana;
	int dia;				
	int mes;
	int ano;
	int temperatura;
} RTC_Dados;

char* dias_Semana[7]={"Seg","Ter","Qua","Qui","Sex","Sab","Dom"};
unsigned int contador_timer_0; int LED, porta1 = 0, porta2 = 0, porta3 = 0, porta4 = 0, flag_timer_0, flag_timer_1;
unsigned long contador_timer_1;
void init(void); void pisca_LED(void);
RTC_Dados c_dados;
volatile char leituraUsart;
static char relogio_buffer[100] = {0}, portas_buffer[10], programas_buffer[50];
char diaProg, portaProg, onOFFProg, hora1Prog, hora2Prog, min1Prog, min2Prog;
//-------------------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------I2C/TWI-------------------------------------------------------------------
void TWI_inic(void)														//Fun��o de Inicializa��o de Comunica��o I2C 
{
	TWSR= 0x01;								//Prescaler = 4
	TWBR = 0x0F;							
	TWCR = (1 << TWEN);						
}
void ligar_TWI(void)													//Ligar Comunica��o I2C
{
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}
void parar_TWI(void)													//Parar Comunica��o I2C
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	_delay_ms(100);//
}
void envia_TWI(uint8_t data)											//Envia Dados Por I2C e avan�a uma posi��o no registo
{
	TWDR = data;
	TWCR=(1<<TWINT)|(1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}
uint8_t receb_TWI_ACK(int ackBit)										//Envia Dados Por I2C sem avan�ar posi��o no registo
{
	TWCR = (1<< TWINT) | (1<<TWEN) | (ackBit<<TWEA);
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}
//-------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------- Comunica��o USART------------------------------------------------------------
void USART_Init(unsigned int ubrr)										//Fun��o de Inicializa��o de Comunica��o USART
{
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<USBS0)|(1<<UCSZ00)|(1<<UCSZ01);
}									
void USART_enviaCaractere(unsigned char ch)								//Fun��o de envio de caracter por USART
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = ch;
}
void USART_enviaString(char *s)											//Envio de conjuntos de caracteres por USART
{
	unsigned int i=0;
	while (s[i] != '\x0')
	{
		USART_enviaCaractere(s[i++]);
	};
}
char receberUSART()														//Receber caracteres por USART
{
	while ( !(UCSR0A & (1<<RXC0)) );
	
	return UDR0;
}													
//-------------------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------- Fun��es de Convers�o ---------------------------------------------------------
unsigned int conversor_8bit_int(uint8_t valor)			//Conversor de variavel de 8 bits para inteiro
{
	int valor_conv;
	valor_conv = ((valor>>4)*10)+(valor & 0xF);
	return valor_conv;
}
uint8_t conversor_int_8bit(int valor)					//Conversor de inteiro para variavel de 8 bits
{
	int valor_conv;
	valor_conv = (valor%10)|((valor/10)<<4);
	return valor_conv;
}
//-------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------- Fun��es EEPROM ------------------------------------------------------------
void escreve_EEPROM(unsigned int registo, unsigned char dados)		//Fun��o de escrita na mem�ria EEPROM
{				
	while(EECR & (1 << EEPE));
	EEAR = registo;
	EEDR = dados;
	EECR |= 1 << EEMPE;
	EECR |= 1 << EEPE;
}
unsigned int ler_EEPROM(unsigned int registo)							//Leitura da memoria de um registo da mem�ria EEPROM 
{
	while (EECR & (1 << EEPE));
	EEAR = registo;
	EECR |= (1 << EERE);
	return EEDR;
}
void apagar_EEPROM()													//Apaga toda a mem�ria EEPROM
{
	for(int i=0; i<=1023;i++)
	{
		escreve_EEPROM(i,0);
	}
	USART_enviaString("\nMEMORIA APAGADA\n");
}
void mostrar_EEPROM()													//Impress�o de todos os dados contidos na mem�ria EEPROM, atrav�s de comunica��o USART
{
	for(int i = 0;i<=100;i++){
		memset(programas_buffer,0,sizeof(programas_buffer));
		sprintf(programas_buffer, "\nValor %d: %u", i, ler_EEPROM(i));
		USART_enviaString(programas_buffer);
	}
	USART_enviaString("\n\n");
	_delay_ms(3000);
}
//-------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------Fun��es Referentes a Programa��o--------------------------------------------------
void lerDadosProgramacao(){															
	diaProg = receberUSART();							//Fun��o de aquisi��o de dados provenientes do m�dulo WI-FI, que s�o adquiridos atrav�s da p�gina HTML, referentes �s programa��es de hor�rios.	
	_delay_ms(200);										//Assim que os mesmos v�o sendo adquiridos de forma organizada, s�o atribuidos �s vari�veis correspondentes.
	portaProg = receberUSART();
	_delay_ms(200);										//Nota: Est�o a ser utilizados delays, para que nao exista um cruzamento de aquisi��o e atribui��o de dados entre todas as vari�veis.
	onOFFProg = receberUSART();							
	_delay_ms(200);
	hora1Prog = receberUSART();
	_delay_ms(200);
	hora2Prog = receberUSART();							
	_delay_ms(200);
	min1Prog = receberUSART();
	_delay_ms(200);
	min2Prog = receberUSART();
	_delay_ms(200);
	USART_enviaString("\nDados Recebidos\n");			//Assim que todos os valores est�o atribuidos, � imprimida uma mensagem na Serial
}
void gravarDadosEEPROM(unsigned int posicaoInic){		//Fun��o que grava os valores obtidos pelo modulo WI-FI referentes �s programa��es horarias, na mem�ria interna do uProcessador (EEPROM).
	escreve_EEPROM(posicaoInic+1,diaProg - '0');		//Avan�a uma posi��o e inicia atribui��o dos valores nas posi��es correspondentes, sendo que a primeira posi��o � destinada � ativa��o do hor�rio.
	escreve_EEPROM(posicaoInic+2,portaProg - '0');
	escreve_EEPROM(posicaoInic+3,onOFFProg - '0');		
	escreve_EEPROM(posicaoInic+4,hora1Prog - '0');
	escreve_EEPROM(posicaoInic+5,hora2Prog - '0');
	escreve_EEPROM(posicaoInic+6,min1Prog - '0');
	escreve_EEPROM(posicaoInic+7,min2Prog - '0');
}
void atribuirProgEEPROM(){								//Fun��o de atribui��o de valores na memoria da EEPROM
	int flag_EEPROM = ler_EEPROM(0);
	int i = 0;
	if(flag_EEPROM == 0){
		escreve_EEPROM(0,1);							//Marca a primeira posi��o da memoria para a atribui��o do primeiro horario.
		gravarDadosEEPROM(0);							//Inicia a grava��o das restantes posi��es da memoria, referentes ao primeiro hor�rio.			
		USART_enviaString("Programado com sucesso.");	//Imprime mensagem na Serial.
	}
	else{
		while(flag_EEPROM != 0){						//Faz busca nas posi��es de memoria, at� encontrar oito posi�oes livres
			i += 8;										
			flag_EEPROM = ler_EEPROM(i);				//Avan�a 8 posi��es devido ao tamanho requerido para cada hor�rio.
			if(i >= 1020)
			{
				USART_enviaString("Memoria Cheia");		//Se percorrer toda a mem�ria, e n�o encontrar nenhuma posi��o livre, imprime mensagem na Serial referindo que a mem�ria encontra-se cheia. 
				return;
			}
		}
		escreve_EEPROM(i,1);							//Se se verificar uma posi��o livre, ent�o avan�a com a atribui��o de dados. 
		gravarDadosEEPROM(i);
		USART_enviaString("Programado com sucesso.");
	}
}
void lerProgramasProg(int posicaoInic){												//Fun��o referente � leitura dops diferentes hor�rios programados na EEPROM.
	int horas_temp, minutos_temp, diasemana_temp, porta_temp, onOFF_temp;
	if(ler_EEPROM(posicaoInic) == 1){												//Verifica se algum horario se encontra marcado				
		horas_temp = (ler_EEPROM(posicaoInic+4)*10)+ler_EEPROM(posicaoInic+5);		//Leitura e atribui��o das horas
		minutos_temp = (ler_EEPROM(posicaoInic+6)*10)+ler_EEPROM(posicaoInic+7);	//Leitura e atribui��o dos minutos
		diasemana_temp = ler_EEPROM(posicaoInic+1);									//Leitura e atribui��o do/s dia/s da semana
		porta_temp = ler_EEPROM(posicaoInic+2);										//Leitura e atribui��o da/s porta/s
		onOFF_temp = ler_EEPROM(posicaoInic+3);										//Leitura e atribui��o do modo Ligar ou Desligar

		if(horas_temp == c_dados.horas && minutos_temp == c_dados.minutos){			//Verifica se existe igualdade entre a hora programada e a informa��o da hora do RTC 
			if(diasemana_temp == c_dados.diasemana || diasemana_temp == 8){			//Verifica se existe igualdade entre o dia programado e a informa��o do dia do RTC ou em caso de programa��o di�ria
				switch(porta_temp){													//Verifica a porta programada
					case 1:	if(onOFF_temp == 1){									//Porta 1 - Ligar
								PORTD |= (1<<PORTD4);
								porta1 = 1;
							}
							else{													//Porta 1 - Desligar
								PORTD &= ~(1<<PORTD4);
								porta1 = 0;
							}
							break;	
					case 2:	if(onOFF_temp == 1){									//Porta 2 - Ligar
								PORTD |= (1<<PORTD5);
								porta2 = 1;
							}
							else{													//Porta 2 - Desligar
								PORTD &= ~(1<<PORTD5);
								porta2 = 0;
							}
							break;
					case 3: if(onOFF_temp == 1){									//Porta 3 - Ligar
								PORTD |= (1<<PORTD6);
								porta3 = 1;
							}
							else{													//Porta 3 - Desligar
								PORTD &= ~(1<<PORTD6);
								porta3 = 0;
							}
							break;
					case 4:	if(onOFF_temp == 1){									//Porta 4 - Ligar
								PORTD |= (1<<PORTD7);
								porta4 = 1;
							}
							else{													//Porta 4 - Desligar
								PORTD &= ~(1<<PORTD7);
								porta4 = 0;
							}
							break;
					case 5: if(onOFF_temp == 1){												//Todas as Portas - Ligar
								PORTD |= (1<<PORTD4)|(1<<PORTD5)|(1<<PORTD6)|(1<<PORTD7);
								porta1 = porta2 = porta3 = porta4 = 1;
							}
							else{																//Todas as Portas - Desligar
								PORTD &= ~(1<<PORTD4);
								PORTD &= ~(1<<PORTD5);
								PORTD &= ~(1<<PORTD6);
								PORTD &= ~(1<<PORTD7);
								porta1 = porta2 = porta3 = porta4 = 0;
							}
							break;
					default: break;
				}
			}
		}
	}
}

void percorrerProgramas(){											//Percorre os diferentes horarios inseridos.
	int i = 0;														//Cada hor�rio possui 8 registos da EEPROM, seguindo o seguinte plano:
	while(ler_EEPROM(i) != 0){										// 1 - Ativa��o de Horario ..... 2 - Dia da Semana ..... 3 - Porta ..... 4 - Ligar/Desligar ..... 5 - Hora(1) ..... 6 - Hora(2) ..... 7 - Minutos (1) ..... 8 Minutos (2)  
		lerProgramasProg(i);		
		i += 8;														//Nota: Foram utilizadas oito posi��es de memoria, o que prefaz um total de [ (1024/8) = 128 ], possiveis horarios.
		if(i >= 1020){
			return;
		}
	}
}      
//-------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------ Fun��es RTC ------------------------------------------------------------
unsigned int RTC_Temperatura()										//Leitura de temperatura no modulo RTC
{
	int temper;
	ligar_TWI();	
	envia_TWI(DS3231_REGISTO_ESCRITA);								//Indica o modo de escrita, para indicar em qual posi��o do registo do RTC, dever� ser iniciada a leitura.
	envia_TWI(0x11);												//Posi��o de registo referente � temperatura adquirida pelo m�dulo. 
	ligar_TWI();
	envia_TWI(DS3231_REGISTO_LEITURA);
	temper = conversor_8bit_int(receb_TWI_ACK(0));					//Atribui � vari�vel o valor inteiro convertido (de 8 bits), a leitura do registo e p�ra o avan�o de posi��es de registo. 
	return temper;													//Retorna o valor lido
}
void RTC_ler_relogio()												//Fun��o referente � aquisi��o da data e hora, presentes no modulo RTC
{
	ligar_TWI();
	envia_TWI(DS3231_REGISTO_ESCRITA);								//Indica o modo de escrita, para indicar em qual posi��o do registo do RTC, dever� ser iniciada a leitura.
	envia_TWI(0x00);												//Posi��o inicial do registo
	ligar_TWI();
	envia_TWI(DS3231_REGISTO_LEITURA);								//Indica o modo de leitura
	c_dados.segundos = conversor_8bit_int(receb_TWI_ACK(1));		//ATribui � vari�vel o valor inteiro convertido de 8 bits, a leitura do registo e avan�a uma posi��o de registo. 
	c_dados.minutos = conversor_8bit_int(receb_TWI_ACK(1));			//...
	c_dados.horas = conversor_8bit_int(receb_TWI_ACK(1));			//...
	c_dados.diasemana = conversor_8bit_int(receb_TWI_ACK(1));		//...
	c_dados.dia = conversor_8bit_int(receb_TWI_ACK(1));				//...
	c_dados.mes = conversor_8bit_int(receb_TWI_ACK(1));				//...
	c_dados.ano = conversor_8bit_int(receb_TWI_ACK(1));				//...
	c_dados.temperatura = RTC_Temperatura();						//Inicia leitura e atribui��o da temperatura.
	parar_TWI();													//Termina comunica��o.
}
void RTC_Editar_relogio(int _segundos,int _minutos,int _horas,int _diasemana,int _dia,int _mes,int _ano) {			//Fun��o referente � atribui��o da data e hora no modulo RTC 
	ligar_TWI();
	envia_TWI(DS3231_REGISTO_ESCRITA);								//Modo de escrita 
	envia_TWI(0x00);												//Posi��o inicial
	envia_TWI(conversor_int_8bit(_segundos));						//Envia o valor 8 bits convertido de inteiro.			
	envia_TWI(conversor_int_8bit(_minutos));						//Neste modo automaticamente � avan�ada uma posi��o do registo. 
	envia_TWI(conversor_int_8bit(_horas));							//...
	envia_TWI(conversor_int_8bit(_diasemana));						//...
	envia_TWI(conversor_int_8bit(_dia));							//...
	envia_TWI(conversor_int_8bit(_mes));							//...
	envia_TWI(conversor_int_8bit(_ano));							//...	
	parar_TWI();													//Termina comunica��o.
}
void imprimir_Relogio()												//Fun��o referente ao envio de dados (Data - Hora - Portas)
{
	memset(relogio_buffer,0,sizeof(relogio_buffer));
	sprintf(relogio_buffer, "\nX%02u:%02u:%02u %s, %02u-%02u-%02u %02u %u%u%u%uC",c_dados.horas,c_dados.minutos,c_dados.segundos,dias_Semana[c_dados.diasemana-1],c_dados.dia,c_dados.mes,2000+(c_dados.ano),c_dados.temperatura, porta1, porta2, porta3, porta4);
	USART_enviaString(relogio_buffer);
}
void enviarInfoPortas(){											//Fun��o referente ao envio de portas.
	memset(portas_buffer,0,sizeof(portas_buffer));
	sprintf(portas_buffer,"%d%d%d%dC", porta1, porta2, porta3, porta4);
	USART_enviaString(portas_buffer);
}																	//Nota: Esta fun��o apenas � utilizada no primeiro carregamento e em caso de alguma falha com o modulo WI-FI.
//-------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------- Pisca LED 1Hz -----------------------------------------------------------
void pisca_LED(void){
	if(contador_timer_0 >= 1000){
	switch(LED){
		case 1:	LED = 0;
				PORTB &= ~(1 << PORTB0);							// Desliga o LED, coloca a sa�da PB1 a 0
				contador_timer_0 = 0;								// Inicia contador
				break;
			
		case 0:	LED = 1;
				PORTB |= (1 << PORTB0);								//Liga o LED, coloca a sa�da PB1 a 1
				contador_timer_0 = 0;								//Inicia contador
				break;
	}
	RTC_ler_relogio();
	}
	
}
//-------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------- Fun��o Atividade ---------------------------------------------------------
void atividade()													//Fun��o referente a atividades a executar de acordo com a informa��o enviada pelo modulo WI-FI/HTML 						
{
	char info = receberUSART();					
	switch(info){
		case '*':	enviarInfoPortas();								//Envia informa��es de portas.
							break;
		case '>':	lerDadosProgramacao();							//Grava horarios estabelicidos pelo utilizador.
					atribuirProgEEPROM();
							break;
		case '<':	apagar_EEPROM();								//Apaga horarios anteriormente gravados.
							break;
		case '�':	if(porta1 == 0){								//Muda estado - Porta 1
						PORTD |= (1<<PORTD4);
						porta1 = 1;
					}
					else{
						PORTD &= ~(1<<PORTD4);
						porta1 = 0;
					}
							break;
		case '�':	if(porta2 == 0){								//Muda estado - Porta 2
						PORTD |= (1<<PORTD5);
						porta2 = 1;
					}
					else{
						PORTD &= ~(1<<PORTD5);
						porta2 = 0;
					}
							break;	
		case '{':	if(porta3 == 0){								//Muda estado - Porta 3
						PORTD |= (1<<PORTD6);
						porta3 = 1;
					}
					else{
						PORTD &= ~(1<<PORTD6);
						porta3 = 0;
					}
							break;
		case '}':	if(porta4 == 0){								//Muda estado - Porta 4
						PORTD |= (1<<PORTD7);
						porta4 = 1;
					}
					else{
						PORTD &= ~(1<<PORTD7);
						porta4 = 0;
					}
							break;
		case 'z':	imprimir_Relogio();								//Envia informa��es de data, hora e portas.
							break;
		case 'X':	mostrar_EEPROM();								//Imprime horarios programados na mem�ria interna do uProcessador.
					break;
		default: break;
	}
}
int main(void)
{
	init();
	while(1){
		if(flag_timer_1 == 1){
			percorrerProgramas();									//Se flag_timer_1 = 1, ent�o realiza a verifica��o dos diferentes horarios programados, 
			flag_timer_1 = 0;										//e verifica se existe uma igualdade para realizar a mudan�a de estado das portas. 
		}
		atividade();
	}
}

ISR (TIMER0_COMPA_vect)
{
	contador_timer_1++;
	contador_timer_0++;	
	pisca_LED();
	if(contador_timer_1 >= 2000)						//A cada segundo -> flag_timer_1 = 1 
	{													//Realiza a verifica��o dos diferentes horarios programados, 
		flag_timer_1 = 1;								//e verifica se existe uma igualdade para realizar a mudan�a de estado das portas. f
		contador_timer_1 = 0;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------- Inicializa��es ATMega328P ---------------------------------------------------
void init(void)
{
	cli();														//Desativa��o de Interrupu��es
	DDRB |= (1 << PORTB0) | (1<<PORTB1);						//Defini��o de PB1 como sa�da
																//Defini��o de Timer 0 para LED piscar 	
	DDRD |= (1<<PORTD4) | (1<<PORTD5) | (1<<PORTD6) | (1<<PORTD7); 										
	 
	TCCR0A |= (1 << WGM01);										//Timer em modo CTC
	TCCR0B |= (1 << CS00) | (1 << CS01);						//Prescaler 64 e inicia a contagem
	OCR0A = 124;												//Tempo de incremento para 1ms - [1KHz = 16 MHz / ( 2 * 64 *( 1 + OCR0A ))]
																//OCR0A = 124
	TIMSK0 |= (1 << OCIE0A);									//Programa ISR - Timer0
	
	contador_timer_0 = 0;									
	contador_timer_1 = 0;
	flag_timer_1 = 0;
	
	TWI_inic();
	USART_Init(UBBRR_US);
	
	sei();									//Habilita��o das interrup��es.	
}
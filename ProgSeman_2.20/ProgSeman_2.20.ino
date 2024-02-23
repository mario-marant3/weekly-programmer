//-------------------------------------------------------------------------------
//---------------------------Definição de Bibliotecas----------------------------

#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <String.h>

//-------------------------------------------------------------------------------
//------------------------- Variáveis Globais -----------------------------------

WiFiServer server(80);
//const char* ssid = "NOS_Internet_Movel_09E7"; //VARIÁVEL QUE ARMAZENA O NOME DA REDE SEM FIO EM QUE VAI CONECTAR
const char* ssid = "Wireless-N";
//const char* ssid = "Vodafone-AE7247";
//const char* password = "24421742"; //VARIÁVEL QUE ARMAZENA A SENHA DA REDE SEM FIO EM QUE VAI CONECTAR
const char* password = "1992Marant3";
//const char* password = "M3J3XX33JM79THD9";
char buffer[50], hora[20], data[40], temper[30], diaSemProg, portaProg, onOffProg;
int porta1 = 0, porta2 = 0, porta3 = 0, porta4 = 0, flagCliente;
//-----------------------------------------------------------------------------------
//-----------------------Funções de Atualização de Dados----------------------------- 

void atualizarDados(){              //Função referente à atualização de dados (Horas, Data e Temperatura), recebidos pela Serial
  int j=0, h=0;
  for(int i=2;i<=50;i++)
  {
  if(i<=9){
      hora[i-2] = buffer[i];        //Atualiza a hora
      }

  if(i>10 && i<=25) {
      data[j] = buffer[i];          //Atualiza a data
      j++;
      }

  if(i>26 && i<=28){
      temper[h] = buffer[i];        //Atualiza a temperatura
      h++;
      }
   }
}

void pedirDados(){                  //Função referente ao pedido de dados (Horas, Data e Temperatura) ao ATMega328P
  Serial.write("z");                //Envia caracter 'z' referente ao pedido de dados do RTC
  delay(50);    
  if (Serial.available()) //LÊ SERIAL ENVIADA PELO ATMEG328P
  {
    Serial.readBytesUntil('C',buffer,50);       //Recebe string proveniente da função "imprimir_Relogio()" do ATMega328P
    if(buffer[1] != 'X'){                       //Por forma a evitar cruzamento de dados, a string recebida deverá possuir 'X' como caracter inicial. 
    return;
  }
  }
  atualizarDados();                             //Atualiza dados (Horas, Data e Temperatura)
  porta1 = buffer[30] - '0';
  porta2 = buffer[31] - '0';
  porta3 = buffer[32] - '0';
  porta4 = buffer[33] - '0';
}
void pedirInfoPortas(){         //Função referente ao pedido de informação de portas
  Serial.write("*");       //Envia caracter '*' referente ao pedido de informação de portas
  delay(1000);
  if (Serial.available()) 
  {
    Serial.readBytesUntil('C',buffer,50);   //Recebe string proveniente da função "enviarInfoPortas()" do ATMega328P
  }
  porta1 = buffer[0] - '0';
  porta2 = buffer[1] - '0';
  porta3 = buffer[2] - '0';
  porta4 = buffer[3] - '0';
}                               //Nota: Esta função apenas é utilizada no primeiro carregamento e em caso de alguma falha com o modulo WI-FI.

//---------------------- Função Setup (Inicializações) --------------------------------------------------

void setup() {
Serial.begin(9600);               //Inicializa a Serial(Usart)
 
Serial.println("");                
Serial.println("");                  
Serial.print("Conectando a ");    
Serial.print(ssid);               //Imprime na Serial informação da tentativa de conexão com a rede Wi-Fi
 
WiFi.begin(ssid, password);       //Passa os parametros para a função referente à realização da conexão com a rede Wi-Fi 
 
while (WiFi.status() != WL_CONNECTED) {         //Enquanto não está conectado com a rede Wi-Fi 
delay(500); 
Serial.print(".");
}
Serial.println("");                             
Serial.print("Conectado a rede sem fio ");      
Serial.println(ssid);                         
server.begin();                                         //Inicia o servidor para receber dados do utiilizador

Serial.println("Servidor iniciado");
 
Serial.print("IP para se conectar ao NodeMCU: "); 
Serial.print("http://"); 
Serial.println(WiFi.localIP());                         //Imprime o IP inserido na rede, para que seja possivel a conexão com qualquer dispositivo
pedirInfoPortas();                                      //Pede dados de portas ao ATMega328P          
delay(1000);
pedirDados();                                           //Pede dados (Hora, Data e Temperatura) ao ATMega328P  
delay(1000);
}

//------------------------ Função Loop ---------------------------------
void loop() {
WiFiClient client = server.available();                 //Verifica se algum cliente está conectado ao servidor
if (!client) {                                          //Enquanto não existe qualquer cliente, volta executar o pedido de dados ao ATMega328P.
  pedirDados();
  delay(1000);
  return; 
}                                                       //Assim que existir um cliente no servidor
String request;

while(!client.available()){                             //Enquanto cliente estiver conectado

request = client.readStringUntil('\r');                 //Faz leitura de uma requisição
client.flush();                                         //Aguarda até que todos os dados de saida sejam enviados

client.println("HTTP/1.1 200 OK"); //Versão de HTTP
client.println("Content-Type: text/html");              //Tipo de conteúdo
client.println("");
//-----------------------------------------------------------------------------------------------------------------------------
//---------------------------------Pagina HTML--------------------------------------------------------------------------------
client.print("<!DOCTYPE html> <html> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
client.print("<script> function enviar() { alert(\"PROGRAMADO!!!\"); } function apagar(){alert(\"Memória Apagada!\");}</script>");
client.print("<style> body {background-color: rgba(111, 190, 190, 0.5);} .content { max-width:425px; margin: auto; } h1 {text-align: center;}");
client.print(".button { border: black; border-style: double; width: 150px; height:150px; background-color: #ffffff;");
client.print("text-align: center; text-deccvdvcdcoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; opacity: 1; transition: all 2s; cursor: pointer; }");
client.print(".buttonON { border: black; border-style: double; width: 150px; height:150px; background-color: #1700FF;");
client.print("text-align: center; text-deccvdvcdcoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; opacity: 1; transition: all 2s; cursor: pointer; }");
client.print(".buttonSemana {background-color: #ffffff; border: none; color: black; padding: 15px 15px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px;");
client.print(".buttonSemanaON {background-color: #ffffff; border: none; color: black; padding: 15px 15px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px;");
client.print("");
client.print("</style> </head> <body> <div class=\"content\";> <h1>Programador Semanal</h1>");

client.print("<center><strong>");                                    //Mostra a Hora
client.print(hora);
client.print("<center><strong>");                                    //Mostra a Data  
client.print(data);
client.print("<center><strong>Temperatura: ");                       //Mostra a Temperatura
client.print(temper);
client.print("<span>&#8451;</span></strong></p></center>");
//-------------------------------------------------------------------------------------------------------------------
//------------------------------- PORTA 1 ---------------------------------------------------------------------------
//---------Envio de caracter '«' para a Serial e realiza a alteração do tipo de botão(Ligado ou Desligado)------------
if(porta1 == 0){
  if (request.indexOf("/button1") != -1)  {
     Serial.write("«");                                                                                          
     porta1 = 1;
     client.print("<a href=\"/button1\"\"><button id=\"button1\" class=\"buttonON\">Porta 1</button>");
     }
  else{
    client.print("<a href=\"/button1\"\"><button id=\"button1\" class=\"button\">Porta 1</button>");
    porta1 = 0;
  }
}
else{
  if (request.indexOf("/button1") != -1)  {
     Serial.write("«");
     porta1 = 0;
     client.print("<a href=\"/button1\"\"><button id=\"button1\" class=\"button\">Porta 1</button>");
     }
  else{
    client.print("<a href=\"/button1\"\"><button id=\"button1\" class=\"buttonON\">Porta 1</button>");
    porta1 = 1;
  }
}
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------- PORTA 2 --------------------------------------------------------------------------
//---------Envio de caracter '»' para a Serial e realiza a alteração do tipo de botão(Ligado ou Desligado)------------
if(porta2 == 0){
  if (request.indexOf("/button2") != -1)  {
     Serial.write("»");
     porta2 = 1;
     client.print("<a href=\"/button2\"\"><button id=\"button2\" class=\"buttonON\">Porta 2</button>");
     }
  else{
    client.print("<a href=\"/button2\"\"><button id=\"button2\" class=\"button\">Porta 2</button>");
    porta2 = 0;
  }
}
else{
  if (request.indexOf("/button2") != -1)  {
     Serial.write("»");
     porta2 = 0;
     client.print("<a href=\"/button2\"\"><button id=\"button2\" class=\"button\">Porta 2</button>");
     }
  else{
     client.print("<a href=\"/button2\"\"><button id=\"button2\" class=\"buttonON\">Porta 2</button>");
     porta2 = 1;
  }
}
//---------------------------------------------------------------------------------------------------------------------
//------------------------------------ PORTA 3-------------------------------------------------------------------------
//---------Envio de caracter '{' para a Serial e realiza a alteração do tipo de botão(Ligado ou Desligado)------------
if(porta3 == 0){
  if (request.indexOf("/button3") != -1)  {
     Serial.write("{");
     porta3 = 1;
     client.print("<a href=\"/button3\"\"><button id=\"button3\" class=\"buttonON\">Porta 3</button>");
       }
  else{
     client.print("<a href=\"/button3\"\"><button id=\"button3\" class=\"button\">Porta 3</button>");
     porta3 = 0;
  }
}
else{
  if (request.indexOf("/button3") != -1)  {
     Serial.write("{");
     porta3 = 0;
     client.print("<a href=\"/button3\"\"><button id=\"button3\" class=\"button\">Porta 3</button>");
     }
  else{
     client.print("<a href=\"/button3\"\"><button id=\"button3\" class=\"buttonON\">Porta 3</button>");
     porta3 = 1;
  }
}
//---------------------------------------------------------------------------------------------------------------------
//-------------------------------------- PORTA 4 ----------------------------------------------------------------------
//---------Envio de caracter '}' para a Serial e realiza a alteração do tipo de botão(Ligado ou Desligado)------------
if(porta4 == 0){
  if (request.indexOf("/button4") != -1)  {
     Serial.write("}");
     porta4 = 1;
     client.print("<a href=\"/button4\"\"><button id=\"button4\" class=\"buttonON\">Porta 4</button>");
     }
  else{
     client.print("<a href=\"/button4\"\"><button id=\"button4\" class=\"button\">Porta 4</button>");
     porta4 = 0;
  }
}
else{
  if (request.indexOf("/button4") != -1)  {
     Serial.write("}");
     porta4 = 0;
     client.print("<a href=\"/button4\"\"><button id=\"button4\" class=\"button\">Porta 4</button>");
     }
  else{
     client.print("<a href=\"/button4\"\"><button id=\"button4\" class=\"buttonON\">Porta 4</button>");
     porta4 = 1;
  }
}
//------------------------------------------------------------------------------------------------------------
//-------------------------------------Continuação------------------------------------------------------------
client.print("<hr/><hr/>");
client.println("<center><a href=\"/AtualizarHora\"\"><button>ATUALIZAR DATA/HORA/TEMPERATURA </button></a></center>");

if (request.indexOf("/AtualizarHora") != -1)  {                           //Função de Requisição de Dados do ATMega328P
   pedirDados();
}
client.print("<center><a href=\"buttonProg\"\"><button>Programar</button></a></center>");

if (request.indexOf("/buttonProg") != -1)                                 //Função de requisição do Modo de Programação (Entrada de Dados, Apagar ou Mostrar Dados)
{ 
  String pagPROG ="<br><br><form action=\"/get\"> <select name=\"selecDia\" id=\"selecDia\" > <option value=\"segFeir0\">Segunda-Feira</option> <option value=\"terFeir1\">Ter&ccedil;a-Feira</option> <option value=\"quaFeir2\">Quarta-Feira</option> <option value=\"quiFeir3\">Quinta-Feira</option> <option value=\"sexFeir4\">Sexta-Feira</option> <option value=\"sabado 5\">Sabado</option> <option value=\"domingo6\">Domingo</option> <option value=\"todosDi7\">Todos os Dias</option> </select> <select id=\"port\" name=\"port\"> <option value=\"port1\">Porta 1</option> <option value=\"port2\">Porta 2</option> <option value=\"port3\">Porta 3</option> <option value=\"port4\">Porta 4</option> <option value=\"todPo\">Todas as Portas</option> </select> <select id=\"onOFF\" name=\"onOFF\"> <option value=\"Ligar\">Ligar</option> <option value=\"Desli\">Desligar</option> </select> <br> <input type=\"time\" id=\"horaMin\" value=\"Hora\" name=\"horaMin\"> <input type=\"submit\" href=\"Submit\" value=\"Enviar\"> </form>";
  client.print(pagPROG);
  client.print("<br><a href=\"apagar\"\"><button>Apagar Memoria</button></a></center>");
  client.print("<a href=\"mostrar\"\"><button>Mostrar Memoria</button></a></center>");
}

if(request.indexOf("/apagar") != -1)                                      //Função de requisição referente à função de Apagar Dados
{
  Serial.write("<");
  delay(200);
}

if(request.indexOf("/mostrar") != -1)                                     //Função de requisição referente à função de Mostrar Dados na Serial
{
  Serial.write("X");
  delay(2000);
}

if(request.indexOf("/get") != -1)                                         //Função de requisição referente à função de envio de dados de programação para o ATMega328P
{                                                                         //Com o método "GET", o utilizador poderá introduzir, através de um pequeno formulário o horario requerido.
    String buffPROG = request;
    char c = buffPROG[25];
    switch(c){
      case '0': diaSemProg = '1';
                       break;
      case '1': diaSemProg = '2';
                       break;
      case '2': diaSemProg = '3';
                       break;
      case '3': diaSemProg = '4';
                       break;
      case '4': diaSemProg = '5';                                          //Guarda informação do/s dia/s da semana
                       break;
      case '5': diaSemProg = '6';
                       break;
      case '6': diaSemProg = '7';
                       break;
      case '7': diaSemProg = '8';
                       break;                                                 
    }
    c = buffPROG[36];
    switch(c){
      case '1': portaProg = '1';
                       break;
      case '2': portaProg = '2';
                       break;
      case '3': portaProg = '3';                                          //Guarda informação da/s porta/s
                       break;
      case '4': portaProg = '4';
                       break;
      case 'o': portaProg = '5';
                                                      
    }
    c = buffPROG[44];
    switch(c){
      case 'L': onOffProg = '1';
                       break;
      case 'D': onOffProg = '2';                                          //Guarda informação do modo de operação Ligar ou Desligar
                       break;                                                      
    }
    Serial.write(">");                                                     //Envio de caracter '>', que prepara ATMega328P, para receber dados de programação
    delay(200);
    Serial.write(diaSemProg);
    delay(200);
    Serial.write(portaProg);
    delay(200);                                       
    Serial.write(onOffProg);
    delay(200);
    Serial.write(buffPROG[58]);                                            //Nota: Estão a ser utilizados delays, para que nao exista um cruzamento de aquisição e atribuição de dados entre todas as variáveis.
    delay(200);
    Serial.write(buffPROG[59]);
    delay(200);
    Serial.write(buffPROG[63]);
    delay(200);
    Serial.write(buffPROG[64]);
}
client.print("</div></body></html>");
}
}

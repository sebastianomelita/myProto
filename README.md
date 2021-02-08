# myProto
Simple basic protocol without ack
Implementazione di un semplice protocollo per to peer senza rilevazione delle collisioni Si adopera su un bus interfacciato con un transceiver RS485. 

Si può usare per realizzare un sistema multimaster con stazioni che trasmettono indipendentemente l'una dall'altra senza la supervisione di un dispositivo centrale (master).

Il transceiver provato è un MAX485 con piedino di controllo della direzione. Dovrebbe funzionare anche con un transceiver col controllo automatico della direzione (piedino con una impostazione qualsiasi).
Sostanzialmente è un rimaneggiamento del codice citato di seguito:
 * @file 	ModbusRtu.h
 * @version     1.21
 * @date        2016.02.21
 * @author 	Samuel Marco i Armengol
 * @contact     sammarcoarmengol@gmail.com
 * @contribution Helium6072
 
 Trama: 
 
        |---DA---|---SA---|---GROUP---|---SI---|---BYTE_CNT---|---PAYLOAD---|
 
        |---1B---|---1B---|----1B-----|---1B---|------1B------|---VARIABLE--|
 
 - DA: destination address - 1byte (1-254, 255 indirizzo di broadcast)
 
 - SA: source addresss - 1byte da 1 a 254
 
 - GROUP: group addresss - 1byte da 1 a 254 (per inviare a tutti membri del gruppo DA deve essere 0xFF o 255)
 
 - SI: service identifier (ACK, MSG) - 1byte
 
 - BYTE_CNT: numero byte complessivi (+payload -CRC) - 1byte
 
  
 Il buffer di trasmissione memorizza un solo messaggio ed è a comune tra trasmissione e ricezione.

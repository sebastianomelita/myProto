/**
 * @license
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; version
 *  2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **/
 
//#include <SoftwareSerial.h>
#include <inttypes.h>
#include "Arduino.h"
#include "myProto.h"
uint8_t u8Buffer[MAX_BUFFER]; // buffer unico condiviso per TX e RX (da usare alternativamente)
uint8_t _txpin;
uint16_t u16InCnt, u16OutCnt, u16errCnt;
uint8_t u8lastRec; // per rivelatore di fine trama (stop bit)
uint8_t mysa;
uint8_t mygroup;
uint32_t u32time, u32difsTime;
Stream *port;


uint8_t getMySA(){
	return mysa;
}

uint8_t getMyGroup(){
	return mygroup;
}

void init(Stream *port485, uint8_t _txpin485, uint8_t mysa485, uint8_t mygroup485, uint32_t u32speed=9600){
	port = port485;
	_txpin = _txpin485;
	mysa = mysa485;
	mygroup = mygroup485;
	u16InCnt = u16OutCnt = u16errCnt = 0;
	static_cast<HardwareSerial*>(port)->begin(u32speed);
}

extern void rcvEventCallback(modbus_t* rcvd);

bool sendMsg(modbus_t *tosend){
	tosend->u8sa = mysa;
	tosend->u8group = mygroup;
	bool sent = false;
	//DEBUG_PRINTLN(((int)u8state);
	DEBUG_PRINT("Msg DA: ");
	DEBUG_PRINTLN((uint8_t)tosend->u8da);
	DEBUG_PRINT("Msg SA: ");
	DEBUG_PRINTLN((uint8_t)tosend->u8sa);
	sent = true;
	parallelToSerial(tosend);
	sendTxBuffer(u8Buffer[ BYTE_CNT ]); //trasmette sul canale
	return sent;
}

inline void parallelToSerial(const modbus_t *tosend){
	//copia header
	u8Buffer[ DA ] = tosend->u8da;
	u8Buffer[ SA ] = tosend->u8sa;
	u8Buffer[ GROUP ] = tosend->u8group;
	u8Buffer[ SI ] = tosend->u8si;
	u8Buffer[ BYTE_CNT ] = tosend->msglen + PAYLOAD;
	//copia payload
	for(int i=0; i < tosend->msglen; i++){
		u8Buffer[i+PAYLOAD] = tosend->data[i];
	}
}

int8_t poll(modbus_t *rt, uint8_t *buf)
{
    // controlla se è in arrivo un messaggio
	uint8_t u8current;
    u8current = port->available(); // vede se è arrivato un "pezzo" iniziale del messaggio (frame chunk)

    if (u8current == 0) return 0;  // se non è arrivato nulla per ora basta, ricontrolla al prossimo giro

    // controlla se c'è uno STOP_BIT dopo la fine, se non c'è allora la trama non è ancora completamente arrivata
    if (u8current != u8lastRec)
    {
        // aggiorna ogni volta che arriva un nuovo carattere!
		u8lastRec = u8current;
        u32time = millis();
		//Serial.println("STOP_BIT:");
        return 0;
    }
	// Se la distanza tra nuovo e vecchio carattere è minore di uno stop bit ancora la trama non è completa
    if ((unsigned long)(millis() -u32time) < (unsigned long)STOP_BIT) return 0;
	
	int8_t i8state = getRxBuffer();  // altrimenti recupera tutto il messaggio e mettilo sul buffer
	
    if (i8state < PAYLOAD + 1) // se è palesemente incompleto scartalo
    {
        u16errCnt++;
        return i8state;
    }
	DEBUG_PRINTLN("msg completo");
	//Serial.print("DA messaggio: ");
	//Serial.println((uint8_t)u8Buffer[ DA ]);
	//Serial.print("SA mio: ");
	//Serial.println((uint8_t)mysa);
    if ((u8Buffer[ DA ] != mysa) && !((u8Buffer[ GROUP ] == mygroup)) && (u8Buffer[ DA ] == 255))return 0;
	DEBUG_PRINTLN("msg destinato a me");
	
	rt->data = buf;
	rcvEvent(rt, i8state); // il messaggio è valido allora genera un evento di "avvenuta ricezione"
    rcvEventCallback(rt);  
	return i8state;
}
//----------------------------------------------------------------------------------------------------------
void sendTxBuffer(uint8_t u8BufferSize){
	//DEBUG_PRINT("size1: ");
	//DEBUG_PRINTLN((u8BufferSize);

	if (_txpin > 1)
    {
        // set RS485 transceiver to transmit mode
        digitalWrite( _txpin, HIGH );
    }
	DEBUG_PRINT("size: ");
	DEBUG_PRINTLN(u8BufferSize);
	for(int i=0;i<u8BufferSize;i++){
		DEBUG_PRINT(u8Buffer[i]);
	}
	DEBUG_PRINTLN();
    // transfer buffer to serial line
    port->write( u8Buffer, u8BufferSize );
    if (_txpin > 1)
    {
        // must wait transmission end before changing pin state
        // soft serial does not need it since it is blocking
        // ...but the implementation in SoftwareSerial does nothing
        // anyway, so no harm in calling it.
        port->flush();
        // return RS485 transceiver to receive mode
        digitalWrite( _txpin, LOW );
    }
    while(port->read() >= 0);
    // increase MSG counter
    u16OutCnt++;
}

int8_t getRxBuffer()
{
    boolean bBuffOverflow = false;
	// ripritina il transceiver RS485  in modo ricezione (default)
    if (_txpin > 1) digitalWrite( _txpin, LOW );
	//DEBUG_PRINT("received: ");
    uint8_t u8BufferSize = 0;
    while ( port->available() ) // finchè ce ne sono, leggi tutti i caratteri disponibili
    {							// e mettili sul buffer di ricezione
        u8Buffer[ u8BufferSize ] = port->read();
		DEBUG_PRINT("(");
		DEBUG_PRINT((char) u8Buffer[ u8BufferSize ]);
		DEBUG_PRINT(":");
		DEBUG_PRINT((uint8_t) u8Buffer[ u8BufferSize ]);
		DEBUG_PRINT("),");
        u8BufferSize ++;
		// segnala evento di buffer overflow (un attacco hacker?)
        if (u8BufferSize >= MAX_BUFFER){
			u16InCnt++;
			u16errCnt++;
			return ERR_BUFF_OVERFLOW;
		}
    }
	DEBUG_PRINTLN("");
    u16InCnt++;
    return u8BufferSize;
}

void rcvEvent(modbus_t* rcvd, uint8_t msglen){
	// converti da formato seriale (array di char) in formato parallelo (struct)
	// header
	rcvd->u8da = u8Buffer[ SA ];
	rcvd->u8group = u8Buffer[ GROUP ];
	rcvd->u8si = u8Buffer[ SI ];
	rcvd->msglen = u8Buffer[ BYTE_CNT ];
	// payload
	for(int i=0; i < msglen-PAYLOAD; i++){
		rcvd->data[i] = u8Buffer[i+PAYLOAD];
		DEBUG_PRINT(rcvd->data[i]);
		DEBUG_PRINT(",");
	}
	
	// notifica l'evento di ricezione all'applicazione con una callback
}
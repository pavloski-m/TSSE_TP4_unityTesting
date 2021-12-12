/*
 * HX711_driver.c
 *
 *  Created on: 11 oct. 2021
 *      Author: pablo
 */

#include "HX711_driver.h"

// ======== Para configurar la ganancia =======================/

#define   	FACTOR_ESCALA_MG   	719  //Número por el que se divide la cuenta para que el valor se de en milgramos
#define		TARE_BUFFER			4
#define		MS_TARE_SAMPLES		10

/**
 * Se define una estructura interna al driver que no es visible al usuario.
 * Se utiliza para cargar las funciones definidas especificamente como port para
 * el hardware especifico.
 */

static port_HX711_t port_HX711_ctrl; //


// Función para inicializar el driver con las funciones del port y el módulo

bool_t init_HX711_Driver(module_t *modulo, gpioMap_t ADDO, gpioMap_t ADSK, uint8_t channelGain){

	bool_t assertInit;

	port_HX711_ctrl.configParameters = initHx711;
	port_HX711_ctrl.getRawData = readRawValue;
	port_HX711_ctrl.initUpdtISR = initISR_HX711;
	port_HX711_ctrl.getRawISRData = readRawValueISR;
	port_HX711_ctrl.sleepmodule = sleepHX711;
	port_HX711_ctrl.awakemodule = awakeHX711;


	assertInit = port_HX711_ctrl.configParameters (ADDO, ADSK, channelGain);


	// Se cargan la variables del módulo que se pasó por referencia

	modulo->offset = (int32_t) updateTare(modulo);

	modulo->rawData = modulo->offset;
	modulo->filterValue = FILTER_INIT;
	modulo->escala = FACTOR_ESCALA_MG;
	modulo->estado_continuo = ONE_READ;
	modulo->processedData = 0;


	return assertInit; 	// se puede pasar GPIO para configurar individualmente los módulos
}


// Función que toma la muestra para actualizar la tara
uint32_t updateTare (module_t *modulo){

	uint32_t suma = 0;

	for(int i=0; i<5; i++){						//ciclos para limpiar la lect
		port_HX711_ctrl.getRawData();
		delay(1);
	}

	for(int i=0; i<TARE_BUFFER; i++){

		suma += ((port_HX711_ctrl.getRawData())/TARE_BUFFER);
		delay(MS_TARE_SAMPLES);
	}
	modulo->offset = (uint32_t) suma;

	return ((uint32_t) suma);

}

// Función para activar la conversión continua en el port
void activateISRConvertion(module_t *modulo){

	port_HX711_ctrl.initUpdtISR(CONVERTION, FILTER_CONT);

	modulo->estado_continuo = CONVERTION;

}

// Función para activar la conversión continua en el port
void disableISRConvertion(module_t *modulo){

	port_HX711_ctrl.initUpdtISR(ONE_READ, FILTER_CONT);

	modulo->estado_continuo = ONE_READ;

}


int32_t	actualizarDatoISR (module_t *modulo){

	int32_t ISRout = 0;
	ISRout = (((int32_t)port_HX711_ctrl.getRawISRData()) - modulo->offset)/FACTOR_ESCALA_MG;
	modulo->processedData = ISRout;
	return ISRout;
}

//Función para actualizar el valor
int32_t actualizarDato (module_t *modulo){

	static int32_t prevValue=0;

	int32_t datoaux = (int32_t)(port_HX711_ctrl.getRawData() - modulo->offset)/FACTOR_ESCALA_MG;

	float fdatoaux = (float) datoaux;
	float fprevVal = (float) prevValue;
	float ffilter  = ((float) modulo->filterValue) / 100;

	fprevVal = fprevVal*(1-ffilter) + fdatoaux*ffilter;

	prevValue = (int32_t) fprevVal;

	modulo->processedData = prevValue;

	return prevValue;
}


// Cuando quiero tener solo un dato crudo.. no se si será útil...
int32_t one_time_read_raw (void){
	return (port_HX711_ctrl.getRawData());
}


//PENDIENTE
void calibrate(void){

	// rutina de calibración
	// se actualiza  el valor de escala
	// (Raw value con peso1 - Offset) / peso1 verdadero;

}

void HX711state (bool_t estado){
	if(!estado){
		port_HX711_ctrl.sleepmodule();
	}
	else {port_HX711_ctrl.awakemodule();
};
}

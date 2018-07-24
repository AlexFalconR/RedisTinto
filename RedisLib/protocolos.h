#ifndef PROTOCOLOS_H_
#define PROTOCOLOS_H_

typedef enum{
	OKCONN, //Conexion correcta
	DN_ALIVE, //Módulo conectado
	DN_KEEPALIVE, //¿Módulo conectado?
//-------Operaciones entre coordinador y planificador---------------------------
	GET_KEY_PLANIF, //Operacion get para planificador desde coordinador
	SET_KEY_PLANIF, //Operacion set para planificador desde coordinador
	STORE_KEY_PLANIF, //Operacion store para planificador desde coordinador
	STATUS, //Operacion status, que devuelve el valor de una clave y luego la instancia
//-------Estado de la operacion realizada---------------------------------------
	OPER_ERROR_DESCONEXION_PLANIF, //Operacion falladada por desconexion del planificador
	OPER_ERROR_DESCONEXION_ESI, // Operacion termino con error, matar esi
	OPER_ERROR_SIZE_CLAVE, // Abortar el esi
	OPER_ERROR_CLAVE_NO_IDENTIFICADA, // Abortar el esi
	OPER_ERROR_CLAVE_INACCESIBLE, // Encola el esi
	OPER_ERROR_CLAVE_INACCESIBLE_ABORTA, // Abortar el esi
	OPER_ERROR_INST, //Error instancia que existía, pero ya no
	OPER_ERROR_DESCONEXION_INST,//Instancia Desconectada
	OPER_OK, // La operacion termino con exito
	OPER_ERROR, //Error de operacion
	OPER_STATUS, //Operacion status que se envia desde el planificador hacia el coordinador
	RESULTADO_STATUS,
//--------Operaciones que realiza el esi, que envia informacion al coordinador-
	GET_KEY, //Operacion get de coordinador desde esi
	SET_KEY, //Operacion setKey de coordinador desde esi
	SET_VALUE, //Operacion setValue de coordinador desde esi
	STORE_KEY, //Operacion store de coordinador desde esi
//--------Conexion de instancia a coordinador---------------------------------
	NOMBRE_INSTANCIA, //Conexion de instancia a coordinador, envia el nombre
	TAM_ENTRADAS, //Conexion de instancia a coordinador, envia el tamaño de las entradas
	CANT_ENTRADAS, //Conexion de instancia a coordinador, envia la cantidad de entradas
//--------Tipos de conexion---------------------------------------------------
	ESI, //Tipo de conexion, ESI
	COORD, //Tipo de conexion, Coordinador
	PLANIF, //Tipo de conexion, Planificador
	INST, //Tipo de conexion, Instancia
//--------Operaciones de instancia enviadas desde coordinador-----------------
	SET_KEY_INST,
	SET_VALUE_INST,
	STORE_KEY_INST,
	COMPACTAR_INST,
	OK_OP,
	OK_COMPACTACION,
	NECESITA_COMP,
	START_DUMP,
	STOP_DUMP,
//-------Estado del esi------------------------------------------------------
	EJECUTAR,
	OK_COORDINADOR,
	ERROR_COORDINADOR,
	ABORTEN,
//-------Finalizacion de la operacion, mensaje entre coordinador y esi----
	OK_OPER,
	ERROR_OPER,
	FINAL_OPER,
	CONTINUAR_OPER
}t_proto;

#endif

#include "Planificador.h"


pthread_mutex_t	mutex_console = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]){

	ruta_config = string_duplicate(argv[1]);


	sockets_clientes = list_create();
	crear_colas();
	crear_flags_para_consola();
	levantar_logger();

	// Config
	parsear_config();

		// Servidor
		conectarse_coord();
		crear_servidor();
		// Consola
		pthread_create(&t_consola,NULL,(void*)&ejecutar_consola, NULL);
		select_clientes();


	exit_gracefully(0);
}

void planificador_status(char* clave){
	enviarMensajeConProtocolo(s_coordinador,clave, OPER_STATUS);
}

void liberar_recursos(){
	list_destroy(sockets_clientes);
	liberar_cola(cola_listos);
	liberar_cola(cola_bloqueados);
	liberar_cola(cola_ejecutando);
	liberar_cola(cola_terminados);
	liberar_lista_recursos();
	log_info(logger, "Planificador terminado");
	log_destroy(logger);
	free(ruta_config);
	free(IP_COORDINADOR);
	free(ALGORITMO);
	pthread_mutex_destroy(&mutex_console);

}


void liberar_lista_recursos(){
	int pos;
	for(pos = 0; pos < list_size(recursos); pos++){
		t_recurso* recurso = list_get(recursos, pos);

		free(recurso->clave);
		liberar_cola(recurso->esis);
		free(recurso);
	}
	list_destroy(recursos);
}

void liberar_cola(t_queue* cola){
	int pos;
	for(pos = 0; pos < queue_size(cola); pos++){
		t_esi* esi = queue_pop(cola);
		free(esi);
	}
	queue_destroy(cola);
}

void select_clientes() {
	log_info(logger, "esperando conexiones de clientes");

	int max_fds;
	int select_status;
	max_fds = s_servidor;

	while (1) {
		// en cada iteracion hay que blanquear(FD_ZERO) el fds_esis, que es un bitarray donde queda guardado
		// que sockets tienen cambios despues del select y tambien volver a setear(FD_SET) los filedescriptors en el fds_set.
		//ademas de volver a calcular cual es el filedescriptor mas alto
		construir_fds(&max_fds);


		select_status = select(max_fds + 1, &fds_cliente, NULL, NULL, NULL);
		//select_status puede ser:
		// < 0 => error
		// == 0 => no paso nada
		// > 0 => hubo algun cambio en los sockets o entro otro nuevo

		if (select_status < 0) {
			perror("select failed");
			log_error(logger, "fallo select");
			return;
		} else if (select_status > 0) {
			leer_cambios_select();
		}
	}
}

void leer_cambios_select() {
	int i;
	int socket_actual;

	//nuevo cliente
	if (FD_ISSET(s_servidor, &fds_cliente)) {
		log_info(logger, "se recibio una nueva conexion");
		atender_nueva_conexion();
	}

	//recibo data de uno ya conectado
	//TODO handlear si el cliente se desconecta abruptamente(seguramente hay que usar alguna signal o algo asi)
	for (i = 0; i < list_size(sockets_clientes); i++) {
		socket_actual = get_socket_posicion(i);
				if (FD_ISSET(socket_actual, &fds_cliente)) {
					recibir_data_de_cliente(socket_actual);
				}
	}
}

void construir_fds(int* max_actual) {
	int i;
	int socket_actual;
	FD_ZERO(&fds_cliente);
	FD_SET(s_servidor, &fds_cliente);
	for (i = 0; i < list_size(sockets_clientes); i++) {
		socket_actual = get_socket_posicion(i);
		FD_SET(socket_actual, &fds_cliente);
		if (*max_actual < socket_actual) {
			*max_actual = socket_actual;
		}
	}
}

int get_socket_posicion(int posicion) {
	return (int) list_get(sockets_clientes,posicion);
}

void atender_nueva_conexion() {
	int nuevo_cliente;
	t_proto protocolo;
	t_esi* esi_recibido;
	//conecto cliente en un nuevo socket

	int autenticacion= 0;

	if ((nuevo_cliente = esperarConexionAuth(s_servidor,&autenticacion)) < 0) {
		log_error(logger, "no se pudo crear conexion");
		exit_with_error("no se pudo crear conexion");
	}

	list_add(sockets_clientes,(void*) nuevo_cliente);

	// Enviamos un protocolo de confirmacion
	protocolo = OKCONN;
	char* protocolo_string = string_itoa(protocolo);
	enviarMensajeConProtocolo(nuevo_cliente, protocolo_string, protocolo);
	free(protocolo_string);

	switch(autenticacion) {
		case ESI:
			esi_recibido = malloc(sizeof(t_esi));
			esi_recibido -> id = nuevo_cliente;
			queue_push(cola_nuevos, esi_recibido);
			mover_esi_manual(&cola_nuevos, COLA_NUEVOS, cola_listos, COLA_LISTOS, esi_recibido);
			if(queue_size(cola_ejecutando) == 0){
				int algoritmo = definir_algoritmo();
				t_esi* esi_a_ejecutar = planificar(algoritmo);
				enviar_mensaje_a_esi( esi_a_ejecutar );
			}
			break;
		case COORD:
			s_coordinador = nuevo_cliente;
			break;
		default:
			log_error(logger, "Nuevo cliente envio un identificador invalido: %d", autenticacion);
	}
}

int definir_algoritmo(){
	if (string_equals_ignore_case(ALGORITMO, "FIFO")){
		return FIFO;
	}else if (string_equals_ignore_case(ALGORITMO, "SJF-SD")){
		return SJF_SIN_DESALOJO;
	}else if (string_equals_ignore_case(ALGORITMO, "SJF-CD")){
		return SJF_CON_DESALOJO;
	}else if (string_equals_ignore_case(ALGORITMO, "HRRN")){
		return HRRN;
	}
	return -1; //Significa error de algoritmo
}

void finalizar_esis(){
	int cantidad_elemento = list_size(sockets_clientes);
	int pos;
	for(pos = 0; pos < cantidad_elemento; pos++){
		enviarProtocolo((int) list_get(sockets_clientes, pos), ABORTEN);
	}
}

void dejar_de_escuchar_socket(int socket){
	int cantidad_elementos = list_size(sockets_clientes);
	int pos;
	for(pos = 0; pos < cantidad_elementos; pos++){
		if ((int) list_get(sockets_clientes, pos) == socket){
			list_remove(sockets_clientes, pos);
		}
	}
}

void recibir_data_de_cliente(int socket) {
	int tipo;
//	t_esi* esi_encontrado;

	//conecto cliente en un nuevo socket

	if((tipo = recibirProtocolo(socket)) < 0) {
		if (tipo != -1){
			log_error(logger, "Error al recibir protocolo de cliente");
			exit_with_error("Error al recibir protocolo de cliente");
		}else{
			if(socket == s_coordinador){
				log_error(logger, "Se desconecto el Coordinador, por lo tanto se abortará la ejecución de los ESIS y del planificador");
				finalizar_esis();
				exit_gracefully(0);
			}else{
				dejar_de_escuchar_socket(socket);
				log_error(logger, "Se desconecto un Esi");
			}
		}
	}

	if(tipo ==  FINAL_OPER || tipo == OK_OPER || tipo ==  OPER_ERROR_SIZE_CLAVE ||
			tipo == OPER_ERROR_CLAVE_NO_IDENTIFICADA ||
			tipo == OPER_ERROR_CLAVE_INACCESIBLE || tipo == OPER_ERROR_INST ||
			tipo == OPER_ERROR_DESCONEXION_INST || tipo == OPER_ERROR){
		atender_esi(tipo, socket);
	}else if (tipo == GET_KEY_PLANIF || tipo == SET_KEY_PLANIF || tipo == STORE_KEY_PLANIF || tipo == START_DUMP || tipo == STOP_DUMP
			|| tipo == RESULTADO_STATUS){
		atender_coordinador(tipo);
	}else if (tipo == DN_KEEPALIVE){
		enviarProtocolo(socket, DN_ALIVE);
	}else{
		log_error(logger, "Cliente envio un identificador invalido: %d", tipo);
	}
}


void atender_esi(int protocolo, int socket) {
	//t_esi* esi_ejecutando = queue_peek(cola_ejecutando);

		//char* mensaje;
		switch(protocolo){
		case FINAL_OPER:
			log_info(logger, "ESI %d: FINAL_OPER", socket);
			matar_esi_ejecutando(socket);
			if(queue_size(cola_listos) != 0){
				enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			}
			break;
		case OK_OPER:
			log_info(logger, "ESI %d: OK_OPER", socket);
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			break;
		case OPER_ERROR_SIZE_CLAVE:
			matar_esi_ejecutando();
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			log_info(logger, "Ocurrio un error en la ejecucion del ESI %d: tamano de clave.", socket);
			break;
		case OPER_ERROR_CLAVE_NO_IDENTIFICADA:
			matar_esi_ejecutando();
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			log_info(logger, "Ocurrio un error en la ejecucion del ESI %d: utilizo una clave no identificada por Planificador.",socket);
			break;
		case OPER_ERROR_CLAVE_INACCESIBLE:
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			log_info(logger, "Ocurrio un error en la ejecucion del ESI %d: la clave es inaccesible.", socket);
			break;
		case OPER_ERROR_INST:
			matar_esi_ejecutando();
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			log_info(logger, "Ocurrio un error en la ejecucion del ESI %d: error en instancia.", socket);
			break;
		case OPER_ERROR_DESCONEXION_INST:
			matar_esi_ejecutando();
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			log_info(logger, "Ocurrio un error en la ejecucion del ESI %d: Se desconecto la instancia de la clave",socket);
			break;
		case OPER_ERROR:
			matar_esi_ejecutando();
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			log_info(logger, "Ocurrio un error en el coordinador: OPER_ERROR.");
			break;
		case OPER_ERROR_CLAVE_INACCESIBLE_ABORTA:
			matar_esi_ejecutando(socket);
			enviar_mensaje_a_esi(planificar(definir_algoritmo()));
			log_info(logger, "Ocurrio un error en la ejecucion del ESI %d: la clave es inaccesible.", socket);
			log_info(logger, "Se abortara ESI, por ser una operacion STORE.", socket);
			break;
		default:
			log_error(logger, "Protocolo de ESI %d no identificado",socket);
			break;
		}
}

void crear_colas(){
	extern t_queue* cola_listos;
	extern t_queue* cola_bloqueados;
	extern t_queue* cola_ejecutando;
	extern t_queue* cola_terminados;
	extern t_queue* cola_nuevos;
	cola_listos = queue_create();
	cola_bloqueados = queue_create();
	cola_ejecutando = queue_create();
	cola_terminados = queue_create();
	cola_nuevos = queue_create();
	recursos = list_create();
}

void crear_flags_para_consola(){
	extern bool planificador_bloqueado;
	planificador_bloqueado = false;
}

void levantar_logger() {
	logger = abrir_logger(ruta_log, programa, LOG_LEVEL_INFO);
	log_info(logger, "Logger iniciado");
}

void levantar_logger_consola() {
	logger_consola = abrir_logger(ruta_log_consola, programa, LOG_LEVEL_INFO);
	log_info(logger_consola, "Logger iniciado");
}

void parsear_config() {
	log_info(logger, "Levantando configuracion");
	t_config* config = config_create(ruta_config);
	PUERTO_PLANIFICADOR = config_get_int_value(config,"PUERTO");
	PUERTO_COORDINADOR = config_get_int_value(config,"PUERTOCOORDINADOR");
	IP_COORDINADOR = string_duplicate(config_get_string_value(config,"IPCOORDINADOR"));
	ALGORITMO = string_duplicate(config_get_string_value(config,"ALGORITMO"));
	ESTIMACION = config_get_int_value(config,"ESTIMACION");

	if(config_has_property(config, "CLAVESBLOQUEADAS")){
		CLAVES_BLOQUEADAS = config_get_array_value(config, "CLAVESBLOQUEADAS");
		int i = 0;
		while(CLAVES_BLOQUEADAS[i]){
			t_recurso* recurso = malloc(sizeof(t_recurso));
			t_esi* esi = malloc(sizeof(t_esi));
			esi->id = -1;
			recurso->clave = string_duplicate(CLAVES_BLOQUEADAS[i]);
			recurso->esis = queue_create();
			queue_push(recurso->esis, (void*) esi);
			list_add(recursos, recurso);
			i++;
		}
		int pos;
		for(pos = 0; pos < cantidad_Elementos(CLAVES_BLOQUEADAS); pos++){
			free(CLAVES_BLOQUEADAS[pos]);
		}
		free(CLAVES_BLOQUEADAS);

	}

	ALPHA = config_get_int_value(config,"ALPHA");
	config_destroy(config);
	log_info(logger, "Configuracion levantada");
}

int cantidad_Elementos(char ** array) {
	size_t count = 0;
	while (array[count] != NULL)
		count++;
	return count;
}

void exit_with_error(char* error_msg) {
	log_error(logger, error_msg);
	exit_gracefully(1);
}

void exit_gracefully(int return_nr) {
	liberar_recursos();
	exit(return_nr);
}

t_esi* planificar(int algoritmo){
	planificador_bloqueado = true;
	pthread_mutex_lock(&mutex_console);
	t_esi* esi_a_ejecutar = NULL;
	int id_sin_esi = -1;
	switch(algoritmo){

		case FIFO:
			if(queue_size(cola_ejecutando) > 0 ){
				esi_a_ejecutar = queue_peek(cola_ejecutando);
			}else if (queue_size(cola_listos) > 0){
				esi_a_ejecutar = queue_peek(cola_listos);
				mover_esi(cola_listos, COLA_LISTOS, cola_ejecutando, COLA_EJECUTANDO);
			}
		break;

		case SJF_SIN_DESALOJO:
			if(queue_size(cola_ejecutando) > 0 ){
				esi_a_ejecutar = queue_peek(cola_ejecutando);
			}else if (queue_size(cola_listos) > 0){
				esi_a_ejecutar = planificar_sjf();
				mover_esi_manual(&cola_listos, COLA_LISTOS, cola_ejecutando, COLA_EJECUTANDO, esi_a_ejecutar);
			}
		break;

		case SJF_CON_DESALOJO:
			if (queue_size(cola_listos) == 0 && queue_size(cola_ejecutando) > 0){
				esi_a_ejecutar = queue_peek(cola_ejecutando);
			}else if(queue_size(cola_listos) > 0){
				esi_a_ejecutar = planificar_sjf();
				if(queue_size(cola_ejecutando) > 0){
					t_esi* esi_ejecutando = queue_peek(cola_ejecutando);
					if(esi_ejecutando->rafaga_restante <= esi_a_ejecutar->estimacion){
						esi_a_ejecutar = esi_ejecutando;
					}else{
						mover_esi(cola_ejecutando, COLA_EJECUTANDO, cola_listos, COLA_LISTOS);
						mover_esi_manual(&cola_listos, COLA_LISTOS, cola_ejecutando, COLA_EJECUTANDO, esi_a_ejecutar);
					}
				}else{
				mover_esi_manual(&cola_listos, COLA_LISTOS, cola_ejecutando, COLA_EJECUTANDO, esi_a_ejecutar);
				}
			}
		break;

		case HRRN:

			if(queue_size(cola_ejecutando) > 0 ){


				esi_a_ejecutar = queue_peek(cola_ejecutando);
			}else if (queue_size(cola_listos) > 0){
				esi_a_ejecutar = planificar_hrrn();
				mover_esi_manual(&cola_listos, COLA_LISTOS, cola_ejecutando, COLA_EJECUTANDO, esi_a_ejecutar);
			}
		break;

		default:
			return NULL;
		break;

	}

	pthread_mutex_unlock(&mutex_console);
	planificador_bloqueado = false;
	if(esi_a_ejecutar != NULL){
		return esi_a_ejecutar;
	}else{
		t_esi* esi_aux = malloc(sizeof(t_esi));
		esi_aux->id=id_sin_esi;
		return esi_aux;
	}

}


t_esi* planificar_hrrn(){
	t_queue* cola_esis_a_ejecutar = queue_create();

	void get_elemento(t_esi* esi){
		bool comparar_estimacion(t_esi* esi_a_comparar){
			return (esi_a_comparar->estimacion + esi_a_comparar->tiempo_esperando)/esi_a_comparar->estimacion < (esi->estimacion + esi->tiempo_esperando)/ esi->estimacion;
		}
		if(!queue_any_satisfy(cola_listos, (void*) comparar_estimacion)){
			queue_push(cola_esis_a_ejecutar, esi);
		}
	}

	queue_iterate(cola_listos, (void*) get_elemento);
	t_esi* esi_a_ejecutar = queue_pop(cola_esis_a_ejecutar);

	//liberar_cola(cola_esis_a_ejecutar);
	return esi_a_ejecutar != NULL ? esi_a_ejecutar : NULL;

}

t_esi* planificar_sjf(){
	t_queue* cola_esis_a_ejecutar = queue_create(); // porque puede haber varios esis con la misma estimacion y hay que desempatar con fifo

	void get_elemento(t_esi* esi){
		bool comparar_estimacion(t_esi* esi_a_comparar){
			return esi_a_comparar->estimacion < esi->estimacion;
		}
		if(!queue_any_satisfy(cola_listos, (void*) comparar_estimacion)){
			queue_push(cola_esis_a_ejecutar, esi);
		}
	}
	queue_iterate(cola_listos, (void*) get_elemento);

	t_esi* esi_a_ejecutar = queue_pop(cola_esis_a_ejecutar);
	//liberar_cola(cola_esis_a_ejecutar);

	return esi_a_ejecutar != NULL ? esi_a_ejecutar : NULL;
}

void enviar_mensaje_a_esi(t_esi* esi){
	if(esi->id != -1){
		int socket = esi->id;
		enviarProtocolo(socket, EJECUTAR);
	}else{
		log_info(logger,"No hay esis disponibles\n");
		free(esi);
	}
}

void crear_servidor(){
	s_servidor = crearServidor(PUERTO_PLANIFICADOR);
}


void atender_coordinador(int protocolo){
	char* mensaje;
	char* valor_status;
	char* instancia_status;
	t_proto respuesta;
	switch(protocolo){
	case GET_KEY_PLANIF:
		mensaje = esperarMensaje(s_coordinador);
		log_info(logger, "Get: %s", mensaje);
		resultado_ultima_operacion = get_recurso(mensaje);
		enviar_mensaje_a_coordinador(resultado_ultima_operacion);
		break;
	case SET_KEY_PLANIF:
		mensaje = esperarMensaje(s_coordinador);
		log_info(logger, "Set: %s", mensaje);
		respuesta = set_recurso(mensaje);
		enviar_mensaje_a_coordinador(respuesta);
		break;
	case STORE_KEY_PLANIF:
		mensaje = esperarMensaje(s_coordinador);
		log_info(logger, "Store: %s", mensaje);
		respuesta = store_recurso(mensaje);
		enviar_respuesta_store(respuesta);
		break;
	case START_DUMP:
		mensaje = string_new();
		pausar();
		//log_info(logger, "Las instancias estan ejecutando el dump");
		break;
	case STOP_DUMP:
		mensaje = string_new();
		continuar();
		//log_info(logger, "Las instancias terminaron de ejecutando el dump");
		break;
	case RESULTADO_STATUS:
		mensaje = string_new();
		valor_status = esperarMensaje(s_coordinador);
		instancia_status = esperarMensaje(s_coordinador);
		log_info(logger, "El valor es: %s", valor_status);
		log_info(logger, "Instancia a la que corresponde/ra es: %s", instancia_status);
		free(valor_status);
		free(instancia_status);
		break;
	}
	free(mensaje);
}

void enviar_mensaje_a_coordinador(bool respuesta){
	int socket = s_coordinador;
	if(respuesta){
		enviarProtocolo(socket, CONTINUAR_OPER);
	}else{
		enviarProtocolo(socket, ERROR_OPER);
	}
}

void enviar_respuesta_store(t_proto proto_respuesta) {
	enviarProtocolo(s_coordinador, proto_respuesta);
}

void conectarse_coord(){
	log_info(logger, "Intentando conectar con el coordinador");
	int p_resp;
	s_coordinador = conectarAuth(PUERTO_COORDINADOR,IP_COORDINADOR,PLANIF,&p_resp);
	if(p_resp == OKCONN){
		list_add(sockets_clientes,(void*) s_coordinador);
		printf("Conectado a coordinador con socket %d \n",s_coordinador);
	}else{
		printf("error de coneccion \n");
	}
}

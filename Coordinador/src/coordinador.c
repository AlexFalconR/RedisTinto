/*
 ============================================================================
 Name        : Coordinador.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "coordinador.h"

int main(int argc, char* ruta[]) {
	ruta_config = string_duplicate(ruta[1]);
	iniciar_coordinador();
	iniciar_servidor();
	iniciar_select();
}

void iniciar_select() {
	log_info(logger, "comenzando select...");

	int max_fds;
	int select_status;

	max_fds = s_servidor;

	while (1) {
		// en cada iteracion hay que blanquear(FD_ZERO) el fds_esis, que es un bitarray donde queda guardado
		// que sockets tienen cambios despues del select y tambien volver a setear(FD_SET) los filedescriptors en el fds_set.
		//ademas de volver a calcular cual es el filedescriptor mas alto
		construir_fds(&max_fds);

		log_info(logger, "esperando conexiones");

		select_status = select(max_fds + 1, &fds_esis, NULL, NULL, NULL);
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

void construir_fds(int* max_actual) {
	int i;
	int socket_actual;
	FD_ZERO(&fds_esis);
	FD_SET(s_servidor, &fds_esis);
	for (i = 0; i < list_size(sockets_clientes); i++) {
		socket_actual = get_socket_posicion(i);
		FD_SET(socket_actual, &fds_esis);
		if (*max_actual < socket_actual) {
			*max_actual = socket_actual;
		}
	}
}

int get_socket_posicion(int posicion) {
	return (int) list_get(sockets_clientes, posicion);
}

void leer_cambios_select() {
	int i;
	int socket_actual;

	//nuevo cliente
	if (FD_ISSET(s_servidor, &fds_esis)) {
		log_info(logger, "se recibio una nueva conexion");
		atender_nueva_conexion();
	}

	//recibo data de uno ya conectado
	//TODO handlear si el cliente se desconecta abruptamente(seguramente hay que usar alguna signal o algo asi)
	for (i = 0; i < list_size(sockets_clientes); i++) {
		socket_actual = get_socket_posicion(i);
		if (FD_ISSET(socket_actual, &fds_esis)) {
			log_info(logger, "se recibio data de un ciente conectado");
			recibir_data_de_cliente(socket_actual);
		}
	}
}

void atender_nueva_conexion() {
	int nuevo_cliente;
	//conecto cliente en un nuevo socket

	int autenticacion = 0;

	if ((nuevo_cliente = esperarConexionAuth(s_servidor, &autenticacion)) < 0) {
		log_error(logger, "no se pudo crear conexion");
		exit_with_error("no se pudo crear conexion");
	}

	list_add(sockets_clientes, (void*) nuevo_cliente);

	// Enviamos un protocolo de confirmacion
	char* protocolo_okconn = string_itoa(OKCONN);
	enviarMensajeConProtocolo(nuevo_cliente, protocolo_okconn, OKCONN);
	free(protocolo_okconn);

	switch (autenticacion) {
	case ESI:
		crear_esi(nuevo_cliente);
		break;
	case PLANIF:
		crear_planificador(nuevo_cliente);
		break;
	case INST:
		crear_instancia(nuevo_cliente);
		break;
	default:
		log_error(logger, "Nuevo cliente envio un identificador invalido: %d",
				autenticacion);
	}
}

void crear_esi(int socket) {
	t_esi* nuevo_esi;
	contador_esis++;
	nuevo_esi = malloc(sizeof(t_esi));
	nuevo_esi->id = contador_esis;
	nuevo_esi->socket = socket;
	list_add(lista_esis, (void*) nuevo_esi);
	log_info(logger, "Se conecto un ESI en el socket: %d", socket);
}

void crear_planificador(int socket) {
	socket_planificador = socket;
	log_info(logger, "Se conecto el Planificador en el socket: %d", socket);
}

void crear_instancia(int socket) {
	//contador_instancias++;
	char* nombre = esperarMensaje(socket);
	char* tamanio_entradas = string_itoa(TAMANO_ENTRADAS);
	enviarMensajeConProtocolo(socket,tamanio_entradas, TAM_ENTRADAS);
	free(tamanio_entradas);
	char* cantidad_entradas = string_itoa(CANTIDAD_ENTRADAS);
	enviarMensajeConProtocolo(socket,cantidad_entradas, CANT_ENTRADAS);
	free(cantidad_entradas);
	int tamanio = list_size(lista_instancias);
	int pos = 0;
	for(; pos < tamanio; pos++){
		t_instancia* instancia = list_get(lista_instancias, pos);
		if (string_equals_ignore_case(instancia->id_instancia, nombre)){
			instancia->socket = socket;
			instancia->conectado = 1;
			break;
		}
	}
	if(pos == tamanio){
		//TODO agegar campo que indique si esta levantada o caida la instancia, estado solamente se esta usando para el equitative load
		t_instancia* nueva_instancia = malloc(sizeof(t_instancia));
		nueva_instancia->id_instancia = string_duplicate(nombre);
		nueva_instancia->espacio_disponible = TAMANO_ENTRADAS;
		nueva_instancia->estado = 0;
		nueva_instancia->socket = socket;
		nueva_instancia->conectado = 1;

		log_info(logger, "Se conecto una INST en el socket: %d", socket);

		list_add(lista_instancias, (void*) nueva_instancia);

		//si estamos en el algoritmo key explicit reasignamos las letras
		if (string_equals_ignore_case(ALG_DISTRIBUCION, "KE")) {
			reasignar_letras_a_almacenar();
		}
	}
	free(nombre);
}

int definir_deconexion(int socket){
	if (socket == socket_planificador){
		return PLANIF;
	}else if (buscar_socket_instancias(socket) == EXIT_SUCCESS){
		return INST;
	}else
		return ESI;
}

int buscar_socket_instancias(int socket){
	int cantidad_elementos = list_size(lista_instancias);
	for (int pos = 0; pos < cantidad_elementos; pos++){
		t_instancia* instancia = list_get(lista_instancias, pos);
		if (instancia->socket == socket){
			instancia->conectado = 0;
			return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}

void finalizar_esi(){
	int cantidad_elementos = list_size(lista_esis);
	if (cantidad_elementos != 0){
		for(int pos = 0; pos < cantidad_elementos; pos++){
			t_esi* esi = list_get(lista_esis, pos);
			enviarProtocolo(esi->socket, ABORTEN);
		}
	}
}

void finalizar_instancias(){
	int cantidad_elementos = list_size(lista_instancias);
	if (cantidad_elementos != 0){
		for(int pos = 0; pos < cantidad_elementos; pos++){
			t_instancia* instancia = list_get(lista_instancias, pos);
			enviarProtocolo(instancia->socket, ABORTEN);
		}
	}
}

void dejar_de_escuchar_socket(int socket){
	int cantidad_elementos = list_size(sockets_clientes);
	for(int pos = 0; pos < cantidad_elementos; pos++){
		if ((int) list_get(sockets_clientes, pos) == socket){
			list_remove(sockets_clientes, pos);
		}
	}
}

void recibir_data_de_cliente(int socket) {
	int accion;
	t_esi* esi_encontrado;
	t_instancia* instancia_encontrada;

	//conecto cliente en un nuevo socket

	if ((accion = recibirProtocolo(socket)) < 0) {
		if (accion != -1){
			log_error(logger, "Error al recibir protocolo de cliente");
			exit_with_error("Error al recibir protocolo de cliente");
		}else{
			int modulo_desconectado = definir_deconexion(socket);
			if (modulo_desconectado == PLANIF){
				log_error(logger, "Se desconecto el planificador, se terminará la ejecución del coordinador");
				finalizar_esi();
				finalizar_instancias();
				exit_gracefully(0);
			}else if (modulo_desconectado == INST){
				dejar_de_escuchar_socket(socket);
				log_error(logger, "Se desconecto una instancias");
			}else{
				dejar_de_escuchar_socket(socket);
				log_error(logger, "Se desconecto un ESI");

			}
		}
	}

	switch (accion){
		case GET_KEY:
		case SET_KEY:
		case STORE_KEY:
			if (!(get_esi_from_socket(socket, &esi_encontrado))) {
				log_error(logger, "No se encontro ESI en lista de esis");
				exit_with_error("No se encontro ESI en lista de esis");
			}
			log_info(logger, "recibo data de ESI en socket: %d", socket);
			atender_esi(esi_encontrado, accion);
			break;
		case OPER_OK:
		case OPER_ERROR:
		case OPER_STATUS:
			atender_planificador(accion);
			break;
		case NECESITA_COMP:
			if (!(get_instancia_from_socket(socket, &instancia_encontrada))) {
				log_error(logger,
						"No se encontro instancia en lista de instancias");
				exit_with_error("No se encontro instancia en lista de instancias");
			}
			atender_instancia(instancia_encontrada);
			break;
		case START_DUMP:
			enviarProtocolo(socket_planificador, START_DUMP);
			log_info(logger_operaciones, "Las instancias estan ejecutando el dump");
			char* mensaje = esperarMensaje(socket);
			if (string_equals_ignore_case(mensaje, "Stop_Dump")){
				enviarProtocolo(socket_planificador, STOP_DUMP);
				log_info(logger_operaciones, "Las instancias terminaron de ejecutando el dump");
			}
			free(mensaje);
			break;
		/*case STOP_DUMP:
			enviarProtocolo(socket_planificador, STOP_DUMP);
			log_info(logger_operaciones, "Las instancias terminaron de ejecutando el dump");
			break;*/
		default:
			log_error(logger, "Cliente envio un identificador invalido: %d",
							accion);
	}

}

void atender_esi(t_esi* esi, int accion) {
	char* clave;
	int termino;
	switch (accion) {
	case GET_KEY:
		clave = esperarMensaje(esi->socket);
		int existe = existe_clave_guardada(clave);
		if (existe == CLAVE_EXISTENTE || existe == CLAVE_GENERADA) {
			if (keep_alive(socket_planificador) == EXIT_SUCCESS) {
				enviarMensajeConProtocolo(socket_planificador, clave,
						GET_KEY_PLANIF);
				int continuar = recibirProtocolo(socket_planificador);
				if (continuar == CONTINUAR_OPER && existe == CLAVE_EXISTENTE) {
					t_clave* clave_encontrada = buscar_clave(clave);
					t_instancia* instancia = buscar_instancia(clave_encontrada);
					if (keep_alive(instancia->socket) == EXIT_SUCCESS) {
						char * mensaje = string_duplicate(
							"Se bloquea la clave ");
						string_append(&mensaje, clave);
						log_info(logger, mensaje);
						log_info(logger_operaciones,
							"Operacion finalizada: GET");
						enviarProtocolo(esi->socket,OPER_OK);
						free(mensaje);
						sleep(RETARDO);
					} else {
							instancia->conectado = 0;
							log_error(logger_operaciones,
									"La operacion GET no pudo ser ejecutada, ya que la instancia que almacenaba la clave se encuentra desconectada");
							enviarProtocolo(esi->socket,
									OPER_ERROR_DESCONEXION_INST);
						}
				} else {
					if(continuar == CONTINUAR_OPER){
							char * mensaje = string_duplicate(
							"Se bloquea la clave ");
							string_append(&mensaje, clave);
							log_info(logger, mensaje);
							log_info(logger_operaciones,
							"Operacion finalizada: GET");
							//MENSAJE A ESI DE OPERACION FINALIZADA
							enviarProtocolo(esi->socket,OPER_OK);
							free(mensaje);
							sleep(RETARDO);
						}else{
						//clave ya estaba tomada por otro esi
						log_error(logger_operaciones,
							"Operacion no realizada: GET, por estar la clave bloqueada");
						enviarProtocolo(esi->socket,OPER_ERROR_CLAVE_INACCESIBLE);
						sleep(RETARDO);
					}
				}
			} else {
				log_error(logger_operaciones,
						"No se realizo la operación ya que no se encuentra levantado el planificador");
				//informo a esi del error
				enviarProtocolo(esi->socket,OPER_ERROR_DESCONEXION_PLANIF);
				exit_with_error(
						"Se aborta el programa por falta de planificador");
			}
		} else {
			enviarProtocolo(esi->socket,OPER_ERROR);
			log_error(logger, "No se pudo guardar la nueva clave");
			log_error(logger_operaciones, "Operacion GET no realizada");
			sleep(RETARDO);
		}
		free(clave);
		break;
	case SET_KEY:
		clave = esperarMensaje(esi->socket);
		int proto = recibirProtocolo(esi->socket);
		if (proto == SET_VALUE) {
			char* valor = esperarMensaje(esi->socket);
			//COMINEZA INTERCAMBIO DE MENSAJES CON PLANIFICADOR
			if (keep_alive(socket_planificador) == EXIT_SUCCESS) {
				//MENSAJE A PLANIFICADOR ¿CLAVE TOMADA?
				enviarMensajeConProtocolo(socket_planificador, clave,
									SET_KEY_PLANIF);
				int continuar = recibirProtocolo(socket_planificador);
				if (continuar == CONTINUAR_OPER){
					t_clave* clave_encontrada = buscar_clave(clave);
					if (clave_encontrada == NULL || !(string_equals_ignore_case(clave_encontrada->valor, valor))){
						if (clave_encontrada == NULL){
							int termino = ejecutar_set(clave, valor,
								GUARDAR_EN_MEMORIA);
							if (termino != -1) {
								enviarProtocolo(esi->socket, OPER_OK);
							} else {
								enviarProtocolo(esi->socket,OPER_ERROR);
							}
						}else{
							t_instancia* instancia = buscar_instancia(clave_encontrada);
							if(keep_alive(instancia->socket) == EXIT_SUCCESS){
								int termino = enviar_set_a_instancia(clave, valor, instancia->socket,clave_encontrada->id_instancia);
								if (termino == EXIT_SUCCESS) {
									enviarProtocolo(esi->socket, OPER_OK);
								} else {
									enviarProtocolo(esi->socket,OPER_ERROR);
								}
							}else{
								int termino = ejecutar_set(clave, valor,GUARDAR_EN_MEMORIA);
								if (termino != -1) {
									enviarProtocolo(esi->socket, OPER_OK);
								} else {
									enviarProtocolo(esi->socket,OPER_ERROR);
								}
							}
						}
					}else{
						log_info(logger_operaciones, "La clave %s ya posee el valor pretendido.", clave);
						enviarProtocolo(esi->socket, OPER_OK);
					}
				}else{
					log_error(logger_operaciones,
						"Operacion no realizada: SET, falló por clave bloqueada");
					enviarProtocolo(esi->socket, OPER_ERROR_CLAVE_INACCESIBLE);
				}
			} else {
				log_error(logger_operaciones,
						"No se realizo la operación ya que no se encuentra conectado el planificador");
				enviarProtocolo(esi->socket, OPER_ERROR_DESCONEXION_PLANIF);
				exit_with_error(
						"Se aborta el programa por falta de planificador");
			}
			free(valor);
		} else {
			log_error(logger, "Se esperaba recibir un valor para el SET");
			exit_with_error("Se esperaba recibir un valor para el SET");
		}
		free(clave);
		break;
	case STORE_KEY:
		clave = esperarMensaje(esi->socket);
		//COMIENZA INTERCAMBIO DE MENSAJES CON PLANIFICADOR
		if (keep_alive(socket_planificador) == EXIT_SUCCESS) {
			enviarMensajeConProtocolo(socket_planificador, clave,
					STORE_KEY_PLANIF);
			int continuar = recibirProtocolo(socket_planificador);

			switch(continuar) {
				case CONTINUAR_OPER:
					termino = ejecutar_storage(clave);
					switch(termino) {
						//errores que puede devolver store
						case OPER_ERROR_DESCONEXION_INST:
						case OPER_ERROR_CLAVE_NO_IDENTIFICADA:
						case OPER_ERROR_INST:
						case OPER_ERROR_CLAVE_INACCESIBLE:
							enviarProtocolo(esi->socket, termino);
							break;
						default: // store termino bien
							enviarProtocolo(esi->socket, OPER_OK);
							log_info(logger_operaciones,"Operacion realizada: STORE");
					}
					break;
				default://hubo algun error en el planificador, se lo mando al esi
					log_error(logger_operaciones, "Operacion no realizada: STORE, por error en planificador");
					enviarProtocolo(esi->socket, continuar);
				break;
			}
		} else {
			log_error(logger_operaciones,
					"No se realizo la operación ya que no se encuentra conectado el planificador");
			enviarProtocolo(esi->socket, OPER_ERROR_DESCONEXION_PLANIF);
			exit_with_error("Se aborta el programa por falta de planificador");
		}
		free(clave);
		break;
	default:
		break;
	}
}

void atender_instancia(t_instancia* instancia) {

}


int ejecutar_storage(char* clave_a_guardar) {
	t_clave* clave_encontrada = buscar_clave(clave_a_guardar);
	if (clave_encontrada != NULL) {
		t_instancia* instancia = buscar_instancia(clave_encontrada);
		if (instancia != NULL) {
			if (keep_alive(instancia->socket) == EXIT_SUCCESS) {
				//ENVIAR MENSAJE A LA INSTANCIA PARA QUE CREE EL ARCHIVO DE LA CLAVE INDICADA
				enviarMensajeConProtocolo(instancia->socket, clave_a_guardar,
						STORE_KEY_INST);
				return recibirProtocolo(instancia->socket);
			}else{
				list_remove(lista_instancias,
				buscar_posicion_instancia(instancia->id_instancia));
				liberar_instancia(instancia);
				log_error(logger_operaciones,
					"Fallo el STORE, ya que la instancia se desconecto");
				return OPER_ERROR_DESCONEXION_INST; //ERROR POR DESCONEXION DE INSTANCIA
			}
		}else{
			//INSTANCIA NO DISPONIBLE
			log_error(logger, "No se encuentra disponible la instancia que poseia la clave");
			log_error(logger_operaciones, "Operacion no reaizada: STORE");
			return OPER_ERROR_INST; //INSTANCIA NO DISPONIBLE
		}
	}
	//NO EXISTE CLAVE
	log_error(logger, "No existe la clave");
	log_error(logger_operaciones, "Operacion no reaizada: STORE");
	sleep(RETARDO);
	return OPER_ERROR_CLAVE_NO_IDENTIFICADA; //CLAVE NO IDENTIFICADA
}

t_instancia* buscar_instancia(t_clave* clave_encontrada) {
	int tamanio = list_size(lista_instancias);
	for (int pos = 0; pos < tamanio; pos++) {
		t_instancia* instancia = list_get(lista_instancias, pos);
		if (string_equals_ignore_case(instancia->id_instancia,
				clave_encontrada->id_instancia)) {
			return instancia;
		}
	}
	return NULL;
}

t_clave* buscar_clave(char* clave_a_guardar) {
	int tamanio = list_size(lista_claves);
	for (int pos = 0; pos < tamanio; pos++) {
		t_clave* clave = list_get(lista_claves, pos);
		if (string_equals_ignore_case(clave->clave, clave_a_guardar)) {
			return clave;
		}
	}
	return NULL;
}

int cantidadElementos(char ** array) {
	size_t count = 0;
	while (array[count] != NULL)
		count++;
	return count;
}

int ejecutar_get(char* clave) {
	return existe_clave_guardada(clave);
}

int existe_clave_guardada(char* obtener_clave) {
	int cantidad_elementos = list_size(lista_claves);
	for (int pos = 0; pos < cantidad_elementos; pos++) {
		t_clave* clave = list_get(lista_claves, pos);
		if (string_equals_ignore_case(clave->clave, obtener_clave))
			return CLAVE_EXISTENTE;
	}
	//GENERAR NUEVA CLAVE VACIA
	return CLAVE_GENERADA;
}

int ejecutar_set(char* ingresar_nueva_clave, char* ingresar_nuevo_valor,
		t_tipo_guardado tipo) {

	if (string_equals_ignore_case(ALG_DISTRIBUCION, "EL")) {
		int termino = guardar_clave_equitative_load(ingresar_nueva_clave,
				ingresar_nuevo_valor, tipo);
		if (termino == EXIT_SUCCESS) {
			if (tipo != GUARDAR_EN_LISTA){
				if (keep_alive(socket_planificador) == EXIT_SUCCESS) {
					log_info(logger_operaciones,
						"Operacion reaizada: SET\tClave guardada: %s",
						ingresar_nueva_clave);
					sleep(RETARDO);
					return EXIT_SUCCESS;
				} else {
					log_error(logger_operaciones,
						"No se realizo la operación ya que no se encuentra levantado el planificador");
					exit_with_error(
						"Se aborta el programa por falta de planificador");
					sleep(RETARDO);
					return -1;
				}
			}else{
				sleep(RETARDO);
				return EXIT_SUCCESS;
			}
		} else {
			//COMIENZO DE CAMBIO DE MENSAJE
			//KEEPALIVE CON PLANIFICADOR, SI SE CAES, SE ABORTA EL PROGRAMA
			if (keep_alive(socket_planificador) == EXIT_SUCCESS) {
				log_info(logger_operaciones, "Operacion no reaizada: SET");
				sleep(RETARDO);
				return -1;
			} else {
				log_error(logger_operaciones,
						"No se realizo la operación ya que no se encuentra levantado el planificador");
				exit_with_error(
						"Se aborta el programa por falta de planificador");
				sleep(RETARDO);
				return -1;
			}
		}
	}
	if (string_equals_ignore_case(ALG_DISTRIBUCION, "LSU")) {
		int termino = guardar_clave_lsu(ingresar_nueva_clave,
				ingresar_nuevo_valor, tipo);
		if (keep_alive(socket_planificador) == EXIT_SUCCESS) {
			if (termino == EXIT_SUCCESS) {
				log_info(logger_operaciones,
						"Operacion reaizada: SET\tClave guardada: %s",
						ingresar_nueva_clave);
				sleep(RETARDO);
				return EXIT_SUCCESS;
			} else {
				log_error(logger_operaciones, "Operacion no reaizada: SET");
				sleep(RETARDO);
				return -1;
			}
		} else {
			log_error(logger_operaciones,
					"No se realizo la operación ya que no se encuentra levantado el planificador");
			exit_with_error("Se aborta el programa por falta de planificador");
			sleep(RETARDO);
			return -1;
		}
	}
	if (string_equals_ignore_case(ALG_DISTRIBUCION, "KE")) {
		int termino = guardar_clave_key_explicit(ingresar_nueva_clave,
				ingresar_nuevo_valor, tipo);
		if (keep_alive(socket_planificador) == EXIT_SUCCESS) {
			if (termino == EXIT_SUCCESS) {
				log_info(logger_operaciones,
						"Operacion reaizada: SET\tClave guardada: %s",
						ingresar_nueva_clave);
				sleep(RETARDO);
				return EXIT_SUCCESS;
			} else {
				log_error(logger_operaciones, "Operacion no reaizada: SET");
				sleep(RETARDO);
				return -1;
			}
		} else {
			log_error(logger_operaciones,
					"No se realizo la operación ya que no se encuentra levantado el planificador");
			exit_with_error("Se aborta el programa por falta de planificador");
			sleep(RETARDO);
			return -1;
		}
	}
	log_error(logger, "Error al leer el algoritmo del config");
	return -1;
}

void liberar_claves() {
	int cantidad_claves = list_size(lista_claves);
	for (int pos = 0; pos < cantidad_claves; pos++) {
		t_clave* clave = list_get(lista_claves, pos);
		liberar_clave(clave);
		free(clave);
	}
}

void liberar_clave(t_clave* clave){
	free(clave->clave);
	free(clave->valor);
	free(clave->id_instancia);
}

int guardar_clave_lsu(char* ingresar_nueva_clave, char* ingresar_nuevo_valor, t_tipo_guardado tipo) {
	t_list* lista_aux_instancias = list_duplicate(lista_instancias);
	list_sort(lista_aux_instancias, (void*) ordenar_instancia_por_espacio);
	t_instancia* instancia_actual = list_get(lista_aux_instancias, 0);
	char* id_instancia_agregado = agregar_clave_a_instancia(instancia_actual,
			ingresar_nueva_clave, ingresar_nuevo_valor); //INSTANCIA MÁS VACIA
	if (id_instancia_agregado != NULL && instancia_actual->conectado == 1) {
		if (tipo == GUARDAR_EN_MEMORIA) {
			if (keep_alive(instancia_actual->socket) == EXIT_SUCCESS) {
				list_destroy(lista_aux_instancias);
				return enviar_set_a_instancia(ingresar_nueva_clave, ingresar_nuevo_valor, instancia_actual->socket, id_instancia_agregado);
			} else {
				log_error(logger_operaciones,
						"Fallo la operacion por desconexión de la instanca");
				instancia_actual->conectado = 0;
				//INFORMA AL PLANIFICADOR QUE NO SE HIZO LA OPERACIÓN PORQUE SE CAYO LA INSTANCIA, ENCOLA EL ESI
				log_info(logger_operaciones, "Buscando nueva instancia");
				list_destroy(lista_aux_instancias);
				return guardar_clave_lsu(ingresar_nueva_clave, ingresar_nuevo_valor, tipo);
			}
		}else{
			if (keep_alive(instancia_actual->socket) == EXIT_SUCCESS){
				list_destroy(lista_aux_instancias);
				return EXIT_SUCCESS;
			}else{
				log_error(logger, "Se desconecto la instancia en la que se almacenaria la clave, se recalculara.");
				instancia_actual->conectado = 0;
				list_destroy(lista_aux_instancias);
				return guardar_clave_lsu(ingresar_nueva_clave, ingresar_nuevo_valor, tipo);
			}
		}
	} else {
		log_error(logger, "Error al ingresar clave en la instancia");
		return -1;
	}
}

int buscar_posicion_instancia(char* id_instancia) {
	for (int pos = 0; pos < list_size(lista_instancias); pos++) {
		t_instancia* instancia_a_buscada = list_get(lista_instancias, pos);
		if (string_equals_ignore_case(id_instancia,
				instancia_a_buscada->id_instancia)) {
			return pos;
		}
	}
	return -1;
}

int ordenar_instancia_por_espacio(t_instancia* instancia1,
		t_instancia* instancia2) {
	int porcentage_instancia1 = instancia1->espacio_disponible * 100
			/ CANTIDAD_ENTRADAS;
	int porcentage_instancia2 = instancia2->espacio_disponible * 100
			/ CANTIDAD_ENTRADAS;
	return (porcentage_instancia1 >= porcentage_instancia2);
}

int guardar_clave_key_explicit(char* ingresar_nueva_clave,
		char* ingresar_nuevo_valor, t_tipo_guardado tipo) {
	int cantidad_instancia = list_size(lista_instancias);
	for (int pos = 0; pos < cantidad_instancia; pos++) {
		t_instancia* instancia_actual = list_get(lista_instancias, pos);
		if ((instancia_actual->letra_inicio < ingresar_nueva_clave[0])
				&& (ingresar_nueva_clave[0] < instancia_actual->letra_fin)) {
			char* id_instancia_agregado = agregar_clave_a_instancia(
					instancia_actual, ingresar_nueva_clave, ingresar_nuevo_valor);
			if (id_instancia_agregado != NULL && instancia_actual->conectado == 1) {
				if (tipo == GUARDAR_EN_MEMORIA) {
					if (keep_alive(instancia_actual->socket == EXIT_SUCCESS)) {
						return enviar_set_a_instancia(ingresar_nueva_clave, ingresar_nuevo_valor, instancia_actual->socket, id_instancia_agregado);
					} else {
						log_error(logger_operaciones,
								"Fallo la operacion por desconexión de la instanca");
						list_remove(lista_instancias, pos);
						liberar_instancia(instancia_actual);
						log_info(logger_operaciones,
								"Buscando nueva instancia");
						reasignar_letras_a_almacenar();
						return guardar_clave_key_explicit(ingresar_nueva_clave,
								ingresar_nuevo_valor, tipo);
					}
				} else {
					if (keep_alive(instancia_actual->socket == EXIT_SUCCESS)){
						return EXIT_SUCCESS;
					}else{
						instancia_actual->conectado = 0;
						return guardar_clave_key_explicit(ingresar_nueva_clave,	ingresar_nuevo_valor, tipo);
					}
				}
			}
		}
	}
	log_error(logger,
			"No existen instancias disponibles o no se pudo ingresar la clave");
	return EXIT_FAILURE;
}

void reasignar_letras_a_almacenar() {
			asignar_letras_a_almacenar();
}

int contar_instancias_conectadas(){
	int cantidad_conectada = 0;
	int pos;
	for(pos = 0; pos < list_size(lista_instancias); pos++){
		t_instancia* instancia = list_get(lista_instancias, pos);
		if (instancia->conectado == 1)
			cantidad_conectada++;
	}
	return cantidad_conectada;
}

int guardar_clave_equitative_load(char * ingresar_nueva_clave,char* ingresar_nuevo_valor, t_tipo_guardado tipo) {
	t_instancia* instancia_actual;
	int pos = 0;

	int cantidad_instancia = list_size(lista_instancias);
	for (pos = 0; pos < cantidad_instancia; pos++) {
		instancia_actual = list_get(lista_instancias, pos);
		if (instancia_actual->estado == 0 && instancia_actual->conectado == 1) {
			char* id_instancia_agregado = agregar_clave_a_instancia(
					instancia_actual, ingresar_nueva_clave, ingresar_nuevo_valor);
			if (tipo == GUARDAR_EN_MEMORIA) {
				if (keep_alive(instancia_actual->socket) == EXIT_SUCCESS) {
					return enviar_set_a_instancia(ingresar_nueva_clave, ingresar_nuevo_valor, instancia_actual->socket, id_instancia_agregado);
					} else {
						log_error(logger_operaciones,
								"Fallo la operacion por desconexión de la instanca");
						list_remove(lista_instancias, pos);
						liberar_instancia(instancia_actual);
						log_info(logger_operaciones,
								"Buscando nueva instancia");
						return guardar_clave_equitative_load(
								ingresar_nueva_clave, ingresar_nuevo_valor,
								tipo);
					}
				}else{
					if (keep_alive(instancia_actual->socket) == EXIT_SUCCESS)
						return EXIT_SUCCESS;
					log_error(logger, "Se desconecto la instancia en la que se almacenaria la clave, se recalculara.");
					instancia_actual->conectado = 0;
					return guardar_clave_equitative_load(ingresar_nueva_clave, ingresar_nuevo_valor,tipo);
				}
		}
	}
		for (pos = 0; pos < cantidad_instancia; pos++) {
			t_instancia* instancia = list_get(lista_instancias, pos);
			instancia->estado = 0;
		}
		if (cantidad_instancia > 0) {
			pos = 0;
			instancia_actual = list_get(lista_instancias, pos);
			if(instancia_actual->conectado == 1){
			char* id_instancia_agregado = agregar_clave_a_instancia(
					instancia_actual, ingresar_nueva_clave, ingresar_nuevo_valor);
			if (id_instancia_agregado != NULL) {
				if (tipo == GUARDAR_EN_MEMORIA) {
					if (keep_alive(instancia_actual->socket) == EXIT_SUCCESS) {
						return enviar_set_a_instancia(ingresar_nueva_clave, ingresar_nuevo_valor, instancia_actual->socket, id_instancia_agregado);
					}else{
						log_error(logger_operaciones,"Fallo la operacion por desconexión de la instanca");
						list_remove(lista_instancias, pos);
						liberar_instancia(instancia_actual);
						log_info(logger_operaciones,"Buscando nueva instancia");
						return guardar_clave_equitative_load(ingresar_nueva_clave, ingresar_nuevo_valor,tipo);
					}
				}else{
					if (keep_alive(instancia_actual->socket) == EXIT_SUCCESS)
						return EXIT_SUCCESS;
					log_error(logger, "Se desconecto la instancia en la que se almacenaria la clave, se recalculara.");
					instancia_actual->conectado = 0;
					return guardar_clave_equitative_load(ingresar_nueva_clave, ingresar_nuevo_valor,tipo);
				}
			}else{
				char* mensaje = string_duplicate("Clave guardada en la instancia, ");
				string_append(&mensaje, id_instancia_agregado);
				log_info(logger, mensaje);
				free(mensaje);
				return EXIT_SUCCESS;
			}
		}
	}else{
		log_error(logger, "No existen instancias disponibles");
		return -1;
	}
	log_error(logger, "No existen instancias conectadas");
	return EXIT_FAILURE;
}


void compractar_instancias(int pos) {
	t_instancia* instancia = list_get(lista_instancias, pos);
	if (keep_alive(instancia->socket) == EXIT_SUCCESS && instancia->conectado == 1) {
		enviarMensajeConProtocolo(instancia->socket, "Compactar",
				COMPACTAR_INST);
		int respuesta = recibirProtocolo(instancia->socket);
		if (respuesta == OK_COMPACTACION) {
			log_info(logger_operaciones, "Se compacto la instancia %s",
					instancia->id_instancia);
		} else {
			log_error(logger_operaciones, "No se compacto la instancias %s",
					instancia->id_instancia);
		}
	} else {
		log_error(logger, "La instancias %s", instancia->id_instancia,
				"se desconecto");
		list_remove(lista_instancias, pos);
		liberar_instancia(instancia);
	}
}

char* agregar_clave_a_instancia(t_instancia* instancia,
		char* ingresar_nueva_clave, char* ingresar_valor_clave) {
	t_clave* nueva_clave = crear_clave(instancia, ingresar_nueva_clave, ingresar_valor_clave);
	list_add(lista_claves, (void*) nueva_clave);
	return instancia->id_instancia;
}

t_clave* crear_clave(t_instancia* instancia, char* ingresar_nueva_clave, char* ingresar_valor_clave) {
	t_clave* nueva_clave = malloc(sizeof(t_clave));
	nueva_clave->clave = string_duplicate(ingresar_nueva_clave);
	nueva_clave->valor = string_duplicate(ingresar_valor_clave);
	nueva_clave->id_instancia = string_duplicate(instancia->id_instancia);
	instancia->estado = 1;
	return nueva_clave;
}

int calcular_entradas_clave(char* ingresar_nueva_clave) {
	int largo = string_length(ingresar_nueva_clave);
	int resto = largo %TAMANO_ENTRADAS;
	if (resto ==0){
		return largo/TAMANO_ENTRADAS;
	}else{
		return (largo / TAMANO_ENTRADAS)+1;
	}
}

void parsear_config() {
	log_info(logger, "Levantando configuracion...");
	config = config_create(ruta_config);

	//LECTURA DE CONFIG
	TAMANO_ENTRADAS = config_get_int_value(config, "TAMANO_ENTRADA");
	log_info(logger, "Configuracion: TAMANO_ENTRADA: %d", TAMANO_ENTRADAS);
	PUERTO_COORD = config_get_int_value(config, "PUERTO");
	log_info(logger, "Configuracion: PUERTO_COORD: %d", PUERTO_COORD);
	CANTIDAD_ENTRADAS = config_get_int_value(config, "ENTRADAS");
	log_info(logger, "Configuracion: ENTRADAS: %d", CANTIDAD_ENTRADAS);
	ALG_DISTRIBUCION = string_duplicate(
			config_get_string_value(config, "ALG_DISTRIBUCION"));
	log_info(logger, "Configuracion: ALG_DISTRIBUCION: %s", ALG_DISTRIBUCION);
	RETARDO = config_get_int_value(config, "RETARDO");
	log_info(logger, "Configuracion: RETARDO: %d", RETARDO);
	config_destroy(config);
}

void iniciar_coordinador() {
	//Lista
	lista_instancias = list_create();
	lista_esis = list_create();
	lista_claves = list_create();
	sockets_clientes = list_create();

	//Inicialización de logguers
	levantar_logger();

	//Lectura de config
	parsear_config();
}

void levantar_logger() {
	logger = abrir_logger(ruta_log, programa, level);
	log_info(logger, "Logger iniciado");
	logger_operaciones = abrir_logger(ruta_log_operaciones, programa, level);
	log_info(logger, "Log de operaciones iniciado");
}

void exit_with_error(char* error_msg) {
	log_error(logger, error_msg);
	exit_gracefully(1);
}

void exit_gracefully(int return_nr) {
	liberar_estructuras();
	free(ruta_config);
	log_info(logger, "Se eliminaron las estructuras");
	log_destroy(logger);
	log_destroy(logger_operaciones);
	exit(return_nr);
}

void liberar_estructuras() {
	liberar_claves();
	liberar_instancias();
	liberar_esis();
	list_destroy(lista_claves);
	list_destroy(lista_instancias);
	list_destroy(lista_esis);
	free(ALG_DISTRIBUCION);
}

void liberar_esis(){
	int cantidad_instancias = list_size(lista_esis);
		for (int pos = 0; pos < cantidad_instancias; pos++) {
			t_esi* esi = list_get(lista_esis, pos);
			free(esi);
		}
}

void liberar_instancia(t_instancia* instancia){
	free(instancia->id_instancia);
}

void liberar_instancias(){
	int cantidad_instancias = list_size(lista_instancias);
	for (int pos = 0; pos < cantidad_instancias; pos++) {
		t_instancia* instancia = list_get(lista_instancias, pos);
		liberar_instancia(instancia);
		free(instancia);
	}
}

void iniciar_servidor() {

	//Creamos un servidor
	s_servidor = crearServidor(PUERTO_COORD);
}

void asignar_letras_a_almacenar() {
	int cantidad_de_instancias = contar_instancias_conectadas();
	int cantidad_de_letras_por_instancas = cantidad_de_letras
			/ cantidad_de_instancias;
	int resto_de_instancias = cantidad_de_letras % cantidad_de_instancias;
	cantidad_de_letras_por_instancas--;
	int pos, posLista, posTotales;
	for (pos = 0, posLista = 0, posTotales = 0;
			pos < cantidad_de_letras && posLista < cantidad_de_instancias && posTotales < list_size(lista_instancias);) {
		t_instancia* instancia = list_get(lista_instancias, posLista);
		if(instancia->conectado == 1){
			instancia->letra_inicio = letras[pos];
			pos = pos + cantidad_de_letras_por_instancas;
			if (resto_de_instancias != 0) {
				pos++;
				resto_de_instancias--;
			}
			instancia->letra_fin = letras[pos];
			posLista++;
			pos++;
		}
		posTotales++;
	}
}

void atender_planificador(int protocolo) {
	log_info(logger, "Se atiende al planificador");
	t_clave* estructura_clave;
	char* clave_recibida;
	switch (protocolo){
	case OPER_STATUS:
		clave_recibida = esperarMensaje(socket_planificador);
		estructura_clave = buscar_clave(clave_recibida);
		if (estructura_clave != NULL){
			enviarMensajeConProtocolo(socket_planificador, estructura_clave->valor, RESULTADO_STATUS);
			enviarMensaje(socket_planificador, estructura_clave->id_instancia);
		}else{
			enviarMensajeConProtocolo(socket_planificador, "Clave no guardada en ninguna instancia", RESULTADO_STATUS);
			ejecutar_set(clave_recibida,"",GUARDAR_EN_LISTA);
			estructura_clave = buscar_clave(clave_recibida);
			enviarMensaje(socket_planificador, estructura_clave->id_instancia);
			estructura_clave = list_remove(lista_claves, buscar_posicion_clave(clave_recibida));
			liberar_clave(estructura_clave);
		}
		break;
	}
}

int buscar_posicion_clave(char* clave_recibida){
	int pos;
	for(pos=0; pos < list_size(lista_claves); pos++){
		t_clave* clave = list_get(lista_claves, pos);
		if (string_equals_ignore_case(clave->clave, clave_recibida))
			return pos;
	}
	return -1;
}

int keep_alive(int socket) {
	enviarProtocolo(socket, DN_KEEPALIVE);
	int answ = recibirProtocolo(socket);
	if (answ == DN_ALIVE) {
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int get_esi_from_socket(int socket, t_esi** esi_encontrado) {
	int i;
	t_esi* elemento_actual;
	for (i = 0; i < list_size(lista_esis); i++) {
		elemento_actual = list_get(lista_esis, i);
		if (elemento_actual->socket == socket) {
			*esi_encontrado = elemento_actual;
			return (1);
		}
	}
	return 0;
}

int get_instancia_from_socket(int socket, t_instancia** instancia_encontrada) {
	int i;
	t_instancia* elemento_actual;
	for (i = 0; i < list_size(lista_instancias); i++) {
		elemento_actual = list_get(lista_instancias, i);
		if (elemento_actual->socket == socket) {
			*instancia_encontrada = elemento_actual;
			return (1);
		}
	}
	return 0;
}

void loggear_op_get(char* id_esi, char* clave) {
	log_info(logger_operaciones, "%s ----- GET %s", id_esi, clave);
}
void loggear_op_set(char* id_esi, char* clave, char* valor) {
	log_info(logger_operaciones, "%s ----- SET %s %s", id_esi, clave, valor);
}
void loggear_op_store(char* id_esi, char* clave) {
	log_info(logger_operaciones, "%s ----- STORE %s", id_esi, clave);
}

void iniciar_mock() {
	//MOCK DE INSTANCIAS Y CLAVE PARA PODER GUARDAR CLAVES
	t_clave* cl = malloc(sizeof(t_clave));
	cl->clave = string_duplicate("basquet");
	cl->id_instancia = string_duplicate("1");
	t_clave* cl1 = malloc(sizeof(t_clave));
	cl1->clave = string_duplicate("futbol");
	cl1->id_instancia = string_duplicate("2");
	list_add(lista_claves, (void*) cl);
	list_add(lista_claves, (void*) cl1);
	t_instancia* in = malloc(sizeof(t_instancia));
	in->espacio_disponible = 49;
	in->estado = 1;
	in->id_instancia = string_duplicate("1");
	t_instancia* in1 = malloc(sizeof(t_instancia));
	in1->espacio_disponible = 50;
	in1->estado = 1;
	in1->id_instancia = string_duplicate("2");
	t_instancia* in2 = malloc(sizeof(t_instancia));
	in->espacio_disponible = 49;
	in->estado = 1;
	in->id_instancia = string_duplicate("1");
	t_instancia* in3 = malloc(sizeof(t_instancia));
	in1->espacio_disponible = 50;
	in1->estado = 1;
	in1->id_instancia = string_duplicate("2");
	list_add(lista_instancias, (void*) in);
	list_add(lista_instancias, (void*) in1);
	list_add(lista_instancias, (void*) in2);
	list_add(lista_instancias, (void*) in3);
}

int enviar_set_a_instancia (char* ingresar_nueva_clave, char* ingresar_nuevo_valor, int socket, char* id_instancia_agregado){
	enviarMensajeConProtocolo(socket, ingresar_nueva_clave, SET_KEY_INST);
	enviarMensaje(socket, ingresar_nuevo_valor);
	int respuesta_instancia = recibirProtocolo(socket);
	if (OK_OP == respuesta_instancia) {
		char* respuesta = esperarMensaje(socket);
		t_instancia* instancia = buscar_instancia_socket(socket);
		instancia->espacio_disponible = atoi(respuesta);
		free(respuesta);
		char* mensaje = string_duplicate("Clave guardada en la instancia, ");
		string_append(&mensaje, id_instancia_agregado);
		log_info(logger, mensaje);
		free(mensaje);
		return EXIT_SUCCESS;
	}else if (NECESITA_COMP == respuesta_instancia){
			int cantidad_instancias = list_size(lista_instancias);
			for (int pos = 0; pos < cantidad_instancias; pos++) {
				t_instancia* instancia = list_get(lista_instancias, pos);
				if (keep_alive(instancia->socket) && instancia->conectado == 1) {
					enviarProtocolo(instancia->socket, COMPACTAR_INST);
					int respuesta_compactacion = recibirProtocolo(instancia->socket);
					if ( respuesta_compactacion == OK_COMPACTACION) {
						log_info(logger_operaciones,"Se compacto la instancia %s",instancia->id_instancia);
					}else{
						log_error(logger_operaciones,"La compactacion en la instancia %s fallo",instancia->id_instancia);
					}
				}else{
						log_error(logger_operaciones,"La compactacion en la instancia %s fallo, porque la instancia se encuentra desconectada",instancia->id_instancia);
				}
			}
			respuesta_instancia = recibirProtocolo(socket);
			if (OK_OP == respuesta_instancia) {
				char* respuesta = esperarMensaje(socket);
				t_instancia* instancia = buscar_instancia_socket(socket);
				instancia->espacio_disponible = atoi(respuesta);
				free(respuesta);
				char* mensaje = string_duplicate("Clave guardada en la instancia, ");
				string_append(&mensaje, id_instancia_agregado);
				log_info(logger, mensaje);
				free(mensaje);
				return EXIT_SUCCESS;
			}else{
				char* mensaje = string_duplicate("Clave no guardada, fallo en la instancia, ");
				string_append(&mensaje, id_instancia_agregado);
				log_error(logger, mensaje);
				free(mensaje);
				return EXIT_FAILURE;
			}
		}else{
			log_error(logger,"Se esepraba una respuesta valida de la instancia");
			exit_with_error("Se esepraba una respuesta valida de la instancia");
			return EXIT_FAILURE;
		}
}

t_instancia* buscar_instancia_socket(int socket){
	t_instancia* instancia;
	int pos;
	for(pos = 0; pos < list_size(lista_instancias); pos++){
		instancia = list_get(lista_instancias, pos);
		if (instancia->socket == socket)
			return instancia;
	}
	return NULL;
}

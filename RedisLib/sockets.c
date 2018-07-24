#include "sockets.h"


///////////
//CLIENTE//
///////////
int conectar(int puerto,char* ip){
	struct sockaddr_in direccServ;
	direccServ.sin_family = AF_INET;
	direccServ.sin_addr.s_addr = inet_addr(ip);
	direccServ.sin_port = htons(puerto);
	int conexion = socket(AF_INET, SOCK_STREAM, 0);
	while (connect(conexion, (void*) &direccServ, sizeof(direccServ)));
	return conexion;
}
int conectarAuth(int puerto,char* ip, int AUTH,int* p_respuesta){
	struct sockaddr_in direccServ;
		direccServ.sin_family = AF_INET;
		direccServ.sin_addr.s_addr = inet_addr(ip);
		direccServ.sin_port = htons(puerto);
		int conexion = socket(AF_INET, SOCK_STREAM, 0);
		while (connect(conexion, (void*) &direccServ, sizeof(direccServ)));
		char* autenticacion = string_itoa(AUTH);
		//envio auth
		enviarMensaje(conexion, autenticacion);
		//recibo respuesta
		*p_respuesta = recibirProtocolo(conexion);
		char *t_respuesta = esperarMensaje(conexion);
		printf("se concluyo la coneccion y el servidor respondio '%s'\n",t_respuesta);
		free(t_respuesta);
		free(autenticacion);
		return conexion;
}

////////////
//SERVIDOR//
////////////
int crearServidor(int puerto){
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(puerto);

	int servidor = socket(AF_INET, SOCK_STREAM, 0);

	int activador = 1;
	setsockopt(servidor,SOL_SOCKET,SO_REUSEADDR, &activador, sizeof(activador));

	if (bind(servidor,(void*) &direccionServidor, sizeof(direccionServidor))){
		return -1;
	}

	listen(servidor,SOMAXCONN);
	return servidor;
}

int esperarConexion(int servidor){
	printf("Estoy a la espera de una conexion\n");
	int cliente = aceptar(servidor);
	printf("Acepte!\n");
	if (cliente <= 0){
		return -1;
		printf("Error de conexion\n");
	}
	return cliente;
}

int esperarConexionAuth(int servidor, int *AUTH){
	printf("Estoy a la espera de una conexion\n");
	int cliente = aceptar(servidor);
	if (cliente <= 0){
		return -1;
	}
	char* autenticacion;
	autenticacion = esperarMensaje(cliente);
	*AUTH = atoi(autenticacion);
	free(autenticacion);
	return cliente;
}


/////////////////////
//ENVIO DE MENSAJES//
/////////////////////
void enviarMensajeConProtocolo(int conexion, char* mensaje, int protocolo){
	char* mensajeReal = header(protocolo);
	char* header_long = header(strlen(mensaje));
	string_append(&mensajeReal,header_long);
	free(header_long);
	string_append(&mensajeReal,mensaje);
	send(conexion,mensajeReal,strlen(mensajeReal),0);
	free(mensajeReal);
}
void enviarMensaje(int conexion, char* mensaje){
	char* mensajeReal = header(strlen(mensaje));
	string_append(&mensajeReal,mensaje);
	send(conexion,mensajeReal,strlen(mensajeReal),0);
	free(mensajeReal);
}
void enviarProtocolo(int socket , int protocolo) {
	char* mensaje = header(protocolo);
	send(socket,mensaje,strlen(mensaje),0);
	free(mensaje);
}
int sendIntTo(int socket, int number) {
	int32_t n = number;
	int status = send(socket, &n, sizeof(n), 0);
	return status <= 0 ? -1 : 0;
}

/////////////////////////
//RECEPCION DE MENSAJES//
/////////////////////////
char* esperarMensaje(int conexion){
	char header[5];
	char* buffer;
	int bytes= recv(conexion, header,4,MSG_WAITALL);
	header[4]= '\0';
	uint32_t tamanioPaquete = atoi(header);
	if (bytes<=0){
		buffer = malloc(2*sizeof(char));
		buffer[0] = '\0';
	}else{
		buffer = malloc((tamanioPaquete+1)*sizeof(char));
		recv(conexion,buffer,tamanioPaquete,MSG_WAITALL);
		buffer[tamanioPaquete] = '\0';
	}
	return buffer;
}

int recibirProtocolo(int conexion){
	char protocolo[5];
	int bytes= recv(conexion, protocolo,4,MSG_WAITALL); //TODO ojo con el /0 aca
	if(bytes!=4){
		perror("Error protocolo");
		return -1;
	}
	protocolo[4] = '\0';
	return atoi(protocolo);
}


//////////////
//AUXILIARES//
//////////////
int autentificar(int conexion, char* autor){
	send(conexion,autor,strlen(autor),0);
	return (esperarConfirmacion(conexion));
}
int esperarConfirmacion(int conexion){
	int bufferHandshake;
	int bytesRecibidos = recv(conexion, &bufferHandshake, 4 , MSG_WAITALL);
	if (bytesRecibidos <= 0) {
		return 0;
	}
	return bufferHandshake;
}
int aceptar(int servidor){
	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion = sizeof(struct sockaddr_in);
	int cliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion);
	return cliente;
}

char* header(int numero){
	char* longitud=string_new();
	char* nnn = string_itoa(numero);
	char* nn = string_reverse(nnn);
	string_append(&longitud,nn);
	free(nnn);
	free(nn);
	string_append(&longitud,"0000");
	char* n_longitud=string_substring(longitud,0,4);
	free(longitud);
	char* nn_longitud=string_reverse(n_longitud);
	free(n_longitud);
	return nn_longitud;
}



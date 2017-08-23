#include "interface.h"

void interface(interface_thread_param* param){

	//Extraigo los parametros (me ahorro hacer el param-> a cada rato)
	int cantComandos = param->cantComandos;
	comando* comandos = param->comandos;

	//Vars
	char buffer[buffer_size + 1];//+1 mas para el \0
	char comando[command_size + 1];
	char parametro[command_size + 1];
	int cantParametros;
	bool comandoCorrecto;

	for(;;){
		//Reseteo el flag
		comandoCorrecto = false;

		//limpio buffers
		fflush(stdin);
		memset(buffer,'\0',buffer_size+1);
		memset(comando,'\0',command_size);
		memset(parametro,'\0',command_size);

		//obtengo los datos de stdin
		fgets(buffer, buffer_size, stdin);
		//Formateo los datos
		cantParametros = sscanf(buffer, "%s %s", comando, parametro);

		//Paso por todos los comandos. Si el comando coincide con alguno, ejecuta la funcion
		int i;
		for(i=0;i<cantComandos;i++){
			if( !strcmp(comando, comandos[i].comando) ){
				comandos[i].funcion(comando,parametro);
				comandoCorrecto = true;
			}
		}

		if(!comandoCorrecto)
			printf("Comando no reconocido.\n");

	}

}

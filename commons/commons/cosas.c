/*
 * cosas.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#include "cosas.h"

int max(int a, int b){
	if(a>b) return a;
		else return b;
}

int min(int a, int b){
	if(a<b) return a;
		else return b;
}

bool esNumero(char* string){
	int size = strlen(string);
	int i;

	for (i=0 ; i < size ; i++){
		if(!isdigit(string[i])) return false;
	}
	return true;
}

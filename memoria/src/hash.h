/*
 * hash.h
 *
 *  Created on: 9/5/2017
 *      Author: utnso
 */

#ifndef HASH_H_
#define HASH_H_

#include <stdlib.h>
#include <time.h>

void hashInit(int size);
void createSeed();
int getPos(int pid, int pag);
int getSeed1();
int getSeed2();

int seed1;
int seed2;
int cant_frame;

#endif /* HASH_H_ */

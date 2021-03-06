/*
 * temp.h
 *
 */

#ifndef TEMP_H
#define TEMP_H
#include "symbol.h"
#include <stdio.h>

typedef struct Temp_temp_* Temp_temp;
Temp_temp Temp_newtemp(void);

typedef struct Temp_tempList_* Temp_tempList;
struct Temp_tempList_ {
    Temp_temp head;
    Temp_tempList tail;
};
Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t);

typedef S_symbol Temp_label;
Temp_label Temp_newlabel(void);
Temp_label Temp_namedlabel(string name);
string Temp_labelstring(Temp_label s);

typedef struct Temp_labelList_* Temp_labelList;
struct Temp_labelList_ {
    Temp_label head;
    Temp_labelList tail;
};
Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t);

typedef struct Temp_map_* Temp_map;
Temp_map Temp_empty(void);
Temp_map Temp_layerMap(Temp_map over, Temp_map under);
void Temp_enter(Temp_map m, Temp_temp t, string s);
string Temp_look(Temp_map m, Temp_temp t);
void Temp_dumpMap(FILE* out, Temp_map m);

Temp_map Temp_name(void);

/* used in liveness.c */
Temp_tempList Temp_copyList(Temp_tempList tl);
void Temp_print(void*);
void Temp_printList(Temp_tempList);
bool inList(Temp_tempList tl, Temp_temp t);
Temp_tempList unionn(Temp_tempList t1, Temp_tempList t2);
Temp_tempList except(Temp_tempList t1, Temp_tempList t2);
void deletee(Temp_tempList* t1, Temp_temp t);
void add(Temp_tempList* t1, Temp_temp t);
bool equals(Temp_tempList t1, Temp_tempList t2);

// It's general bad practice to provide
// this interface maybe. But this makes life easier..
// and is only used in regalloc::heuristic.
int Temp_num(Temp_temp);
#endif

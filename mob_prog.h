#ifndef MOB_PROG_H
#define MOB_PROG_H

bool mprog_seval(char* lhs, char* opr, char* rhs);
bool mprog_veval(int lhs, char *opr, int rhs);

enum DeduceResult {Deduce_Proceed, Deduce_Null, Deduce_Failure};
DeduceResult deduce_char_arg(CHAR_DATA *& result, PROG_RUNDATA * prog, char * arg, CHAR_DATA * vict, bool checkVals);

#endif

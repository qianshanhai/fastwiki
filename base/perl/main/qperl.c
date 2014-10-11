#include "EXTERN.h"
#define PERL_IN_MINIPERLMAIN_C
#include "perl.h"
#include "XSUB.h"

static PerlInterpreter *my_perl;

static void
xs_init(pTHX)
{
    PERL_UNUSED_CONTEXT;
    dXSUB_SYS;
}

int main(int argc, char *argv[])
{

	my_perl = perl_alloc();
	perl_construct(my_perl);

	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
	perl_parse(my_perl, xs_init, argc, argv, NULL);
	perl_run(my_perl);
	/*
	perl_destruct(my_perl);
	perl_free(my_perl);
	*/

	return 0;
}


fash: fash.o alias.o history.o cinterp.o tabcomplete.o redir.o
	cc fash.o alias.o history.o cinterp.o tabcomplete.o redir.o -o fash
fash.o: fash.c alias.h tabcomplete.h cinterp.h redir.h
	cc -c fash.c
alias.o: alias.c uthash.h alias.h
	cc -c alias.c
history.o: history.c history.h
	cc -c history.c
cinterp.o: cinterp.c cinterp.h
	cc -c cinterp.c
tabcomplete.o: tabcomplete.c tabcomplete.h
	cc -c tabcomplete.c
redir.o: redir.c redir.h
	cc -c redir.c
#vim: sts=2 sw=2 et:

# $1 - $sciezka do FIFO
# $2 - komunikat
# $3 - odstep czasu w sek

usage() {
	echo "$0 <sciezka do FIFO> <komunikat> [ <opoznienie (float)> ]"
	echo "domyslne opoznienie to 1.0 sek"
}

prerr() {
	{ echo "$1" ; echo ; usage ; } 1>&2
	exit $2
}

# sprawdzenie istnienia
[ -z $1 ] && { prerr "brak obowiazkowego parametru" 1; }
[ -p "$1" ] || { prerr "niepoprawny parametr \"sciezka do FIFO\"" 1 ; } 

# przyjecie domyslnej wart sleep
SLEEP=${3:- 1.0}

# otwarcie do zapisu
exec 1> "$1"

while : ; do
	echo -ne "$2"
	sleep $SLEEP
done

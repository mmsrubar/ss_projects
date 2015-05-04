#!/bin/sh
#
#	Project:	  strace highlighter
# Seminar:    Operating Systems (Brno University of Technology - FIT)
#	Author:		  Michal Srubar, xsruba03@stud.fit.vutbr.cz
#	Date:		    Mon May  4 21:39:52 CEST 2011
#	
# Description:
# This script can colorize and convert output of the strace utility to the HTML
# format.
#	
#	What is highlighted?:
#	- system calls are redesign to hyper-text link to syscall man page
#	- pid (process id)
#	- relative timestamp
#	- systemCall return values
#	- constants
#	- strings literals
#	note:
#	- all html special chars as >, <, &, are redesign to &lt;, &gt;, &amp;

usage() {
	printf "\
hltrace - strace highlighter.\n
Usage: hltrace [options] <trace.strace >trace.html\n
options:
  -s SYSCALL  Special highlight of this system call.\n"
  exit 0
}

begin() {
	printf "\
<html>
<style>
.pid    { color:darkred; }
.ts     { color:navy; }
.number { color:red; }
.const  { color:green; }
.string { color:blue; }
.hlcall { text-decoration:none; font-weight:bold; color:black; }
.call   { text-decoration:none; color:olive; }
a:hover { text-decoration: underline; }
</style>
<body><pre>\n" 
}

end() {
	printf "</pre></body></html>"
}

while getopts ":hs:" opt; do
	case $opt in
		h)	usage; exit 1 ;;
		s)	sflag=1
			syscall="$OPTARG" ;;
		\?)	usage; exit 1 ;;	
		:)	printf "ERROR: you need to specify the system call.\n" >&2; exit 1 ;;
	esac
done
shift $(($OPTIND-1))

if [ ! -z $1 ]; then
	usage
	exit 1
fi

# used regular expresions
# -----------------------
#
# html key words
#-e "s/<(.*)>/\&lt;\1\&gt;/g" \
#
# pids | tprintf("%-5d ", tcp->pid);
#-e "s/^([0-9][0-9]?[0-9]?[0-9]?[0-9]?) /$tag class=\"pid\">\1$Etag /g" \
#
# timestamp | tprintf("%6ld.%06ld ", (long) dtv.tv_sec, (long) dtv.tv_usec);
#-e "s/([0-9]?[0-9]?[0-9]?[0-9]?[0-9]?[0-9])\.([0-9][0-9][0-9][0-9][0-9][0-9]) /$tag class=\"ts\">\1\.\2$Etag /g" \
#
# [syscall(par) =]|return value
#-e "s/([_[:lower:][_[:lower:]0-9]+)\((.*)\)([[:space:]]+)=/$man\1.2.html\" class=\"call\">\1<\/a>(\2)\3=/g" \
#
# syscalls | tprintf(" <unfinished ...>\n");
#-e "s/([_[:lower:][_[:lower:]0-9]+)\((.*)([[:space:]]+)$bg/$man\1.2.html\" class=\"call\">\1<\/a>(\2\3$bg/g" \
#
# user specific syscalls
#-e "s/(man2\/$syscall.2.html\" )(class=\"call\")/\1class=\"hlcall\"/g" \
#
# constants
#-e :loop -e h -e "s/([[|[&({= ])([A-Z][A-Z0-9_]+)([]|&(),} ])/\1$tag class=\"const\">\2<\/span>\3/g" -e tloop \
#	
# numbers
#-e "s/ = ([-x0-9abcdefABCDEF]+)/ = $tag class=\"number\">\1$Etag/g" \
#-e "s/([( ])([-x0-9abcdefABCDEF]+)([,)])/\1$tag class=\"number\">\2$Etag\3/g" \
#-e "s/([( ])([-x0-9abcdefABCDEF]+) $bg/\1$tag class=\"number\">\2$Etag $bg/g"

export LC_ALL=C
tag="<span"
Etag="<\/span>"
man="<a href=\"http:\/\/www.kernel.org\/doc\/man-pages\/online\/pages\/man2\/"
bg="\&lt;unfinished ...\&gt;"

begin
sed -r \
-e "s@\&@\&amp;@g" -e "s@<@\&lt;@g" -e "s@>@\&gt;@g" \
-e "s/\"([^\"]*)\"/$tag class=\"string\">\"\1\"$Etag/g" \
-e "s/([_[:lower:][_[:lower:]0-9]+)\((.*)\)([[:space:]]+)=/$man\1.2.html\" class=\"call\">\1<\/a>(\2)\3=/g" \
-e "s/([_[:lower:][_[:lower:]0-9]+)\((.*)([[:space:]]+)$bg/$man\1.2.html\" class=\"call\">\1<\/a>(\2\3$bg/g" \
-e "s/(man2\/$syscall.2.html\" )(class=\"call\")/\1class=\"hlcall\"/g" \
-e "s/(^[0-9][0-9]?[0-9]?[0-9]?[0-9]?) /$tag class=\"pid\">\1$Etag /g" \
-e "s/([0-9]?[0-9]?[0-9]?[0-9]?[0-9]?[0-9])\.([0-9][0-9][0-9][0-9][0-9][0-9]) /$tag class=\"ts\">\1\.\2$Etag /g" \
-e :jmp -e h -e "s/([[|[&({= ])([[:upper:]][[:upper:]0-9_]+)([]|&(),} ])/\1$tag class=\"const\">\2<\/span>\3/g" -e tjmp \
-e "s/ = ([-x0-9abcdefABCDEF]+)/ = $tag class=\"number\">\1$Etag/g" \
-e "s/([( ])([-x0-9abcdefABCDEF]+)([,)])/\1$tag class=\"number\">\2$Etag\3/g" \
-e "s/([( ])([-x0-9abcdefABCDEF]+) $bg/\1$tag class=\"number\">\2$Etag $bg/g"
end

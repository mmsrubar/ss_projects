#!/usr/bin/python
# -- coding: utf-8 --

# Author:		Michal Srubar
# Email:		mmsrubar@gmail.com
# Datum:		Thu Jul 10 15:30:45 EDT 2014
# Popis:    Velice jednoduchy skript, pomoci ktereho muzes ziskat seznam vsech
#           clanku dane urovne ze serveru www.helpforenglish.cz 

import re
import urllib

prefix="http://www.h4e.cz"
pageLink="http://www.h4e.cz/?vp-page="
counter=0

import argparse

parser = argparse.ArgumentParser(description=u"Zobraz odkazy na všechny\
					články na serveru www.h4e.cz podle zadané úrovně od nejstaršího\
					po nejnověnší v HTML formatu.\n")
parser.add_argument("uroven", help=u"1=Starter, 2=Elementary,\
    3=Pre-intermediate, 4=Intermediate, 5=Upper-intermediate, 6=Advanced\n",\
										choices=["1", "2", "3", "4", "5", "6"])
parser.add_argument("-n", "--newest", help=u"nejnovější články jako první", action="store_true")
parser.add_argument("-v", "--verbose", help=u"zobraz více informací", action="store_true")
parser.add_argument("-d", "--debug", help=u"debug mode", action="store_true")
args = parser.parse_args()

if args.uroven == "1":
	level = "St.gif"
	str_level = "Starter"
elif args.uroven == "2":
	level = "Et.gif"
	str_level = "Elementary"
elif args.uroven == "3":
	level = "Pt.gif"
	str_level = "Pre-intermediate"
elif args.uroven == "4":
	level = "It.gif"
	str_level = "Intermediate"
elif args.uroven == "5":
	level = "Ut.gif"
	str_level = "Upper-intermediate"
elif args.uroven == "6":
	level = "At.gif"
	str_level = "Advanced"

if args.verbose:
	print u"Vybraná úroveň: "+args.uroven+" ("+level+")"

print "<html>"
print "<head><title>h4e.cz - starter articles</title></head>"
print "<body>"
print "<h1>Help For English: "+str_level+"</h1>"
print "<table border=1px>"
print "<th>číslo</th><th>článek</th><th>strana</th>"

if args.newest:
	fromm=1
	to=274
	step=1
else:
	fromm=274
	to=1
	step=-1

for i in range(fromm,to,step):
	
	sock = urllib.urlopen(pageLink+str(i))
	html = sock.read()

	# find all links in downloaded page
	links=re.findall(level+'".*>.*\n*\s.*\n\s*<h3 class="cla-nadpis">\n\s*<a href="([a-zA-Z0-9/\-()?:;&=+@.\\|~%_#]*)"', html)

	# print links from downloaded pages
	for link in links:

		# get approximate name of the article
		name = re.sub("/article/[0-9]*-", "", link);

		print "<tr>"
		print "<td>"+str(counter+1)+".</td>"
		print "<td><a href=\""+prefix+link+"\">"+name+"</a></td>"
		print "<td><a href=\""+pageLink+str(i)+"\">"+str(i)+"</a></td>",
		print "</tr>"

		counter += 1

	sock.close()

print "</table>"
print "</body>"
print "</html>"

if args.verbose:
	print u"Na úrovni \'"+args.uroven+"\' se na serveru "+prefix+u" nachází "+str(counter)+ u" článků."

/* This is a scanner for PDS firewall rules.
 * 
 * One rule per line:
 * <number> <action> <protocol> from <src IP> to <dest IP> [src-port <srcPort>] [dst-port <dstPort>]
 * 
 * Line description:
 * <number>:    rule number 
 * <action>:    allow|deny
 * <protocol>:  tcp|udp|icmp|ip
 * <src IP>:    IPv4|any
 * <dst IP>:    IPv4|any
 * <srcPort>:   port number
 * <dstPort>:   port number
 */

%option noyywrap

%{
#include <stdio.h>
#include "grammar.tab.h"
%}

DIGIT    [0-9]
ID    ^{DIGIT}+
IPV4  ((({DIGIT}{1,2})|(1{DIGIT}{2})|(2[0-4]{DIGIT})|(25[0-5]))\.){3}(({DIGIT}{1,2})|(1{DIGIT}{2})|(2[0-4]{DIGIT})|(25[0-5]))
IP    {IPV4}|any|ANY
A     allow|deny|ALLOW|DENY
P     tcp|udp|icmp|ip|TCP|UDP|ICMP|IP
F     from|FROM
T     to|TO
SP    src-port|SRC-PORT
DP    dst-port|DST-PORT
PORT  ([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])



%%
{ID}    printf("ID(%s)", yytext);   return ID;
{IP}    printf("IP(%s)", yytext);   return IP;
{A}     printf("A(%s)", yytext);    return ACTION;
{P}     printf("P(%s)", yytext);    return PROTOCOL;
{F}     printf("F(%s)", yytext);    return FROM;
{T}     printf("T(%s)", yytext);    return TO;
{SP}    printf("SP(%s)", yytext);   return SRCPORTSTR;
{DP}    printf("DP(%s)", yytext);   return DESTPORTSTR;
{PORT}  printf("PORT(%s)", yytext); return PORT;
%%
/*
{ID}    return ID;
{IP}    return IP;
{A}     return ACTION;
{P}     return PROTOCOL;
{F}     return FROM;
{T}     return TO;
{SP}    return SRCPORTSTR;
{DP}    return DESTPORTSTR;
{PORT}  return PORT;
*/


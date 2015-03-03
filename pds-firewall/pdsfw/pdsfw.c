/*
 * Project:     Simple Firewall (PDS)
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Sun Mar  1 00:31:18 CET 2015
 * Description: 
 */

/* FIXME change num to id .. */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
 
#define PROCF_NAME "pdsfw_xsruba03"
//#define PROCF_MAX_SIZE  1024
#define PROCF_MAX_SIZE  25

#define proto_ver(x) (x == UDP) ? "udp" : ((x==TCP) ? "tcp" : \
    ((x==ICMP) ? "icmp" : "ip"))
#define action_str(x) (x == ALLOW) ? "allow" : "deny"
#define in_out_str(x) (x == true) ? "INCOMMING" : "OUTGOING"

#define ICMP  1
#define UDP   17
#define TCP   6
#define IP    150   /* this one is not defined so I use it for IP packets */

#define ALLOW 1
#define DENY  2


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDS firewall");
MODULE_AUTHOR("Michal Srubar, xsruba03@stud.fit.vutbr.cz");

static struct nf_hook_ops nfho_in;
static struct nf_hook_ops nfho_out;


static struct fw_rule policy_list;

struct fw_rule {
  unsigned int num;
  bool src_ip_any;          /* true: source ip is set to any */
  bool dest_ip_any;         /* true: source ip is set to any */
  unsigned int src_ip;
  unsigned int dest_ip;
  unsigned int src_port;
  unsigned int dest_port;
  unsigned int protocol;  /* 1: tcp, 2: udp, 3: icmp, 4: ip */
  unsigned int action;    /* 1: allow, 2: deny */
  struct list_head list;
};

struct packet_info {
  struct iphdr *ip_header;
  struct udphdr *udp_header;
  struct tcphdr *tcp_header;
  unsigned int src_ip;
  unsigned int dest_ip;
  unsigned int src_port;
  unsigned int dest_port;
  int proto;
};

//the structure used for procfs
static struct proc_dir_entry *mf_proc_file;
unsigned long procf_buffer_pos;
char *procf_buffer;

unsigned int str_to_uint(char *str)
{
    unsigned int uint = 0;    
    int i = 0;

    if (str == NULL) {
        return 0;
    } 

    while (str[i] != '\0') {
        uint = uint*10 + (str[i]-'0');
        ++i;
    }

    return uint;
}

/* Convert the string IP to byte array, e.g.: * from "131.132.162.25" to 
 * [131][132][162][25]
 *
 * @ip_str:   string IPv4 adress
 */
unsigned int ip_str_to_hl(const char *ip_str)
{
    unsigned char ip_array[4];
    int i = 0;
    unsigned int ip = 0;

    if (ip_str==NULL) { return 0; } 

    memset(ip_array, 0, 4);
    while (ip_str[i]!='.') {
        ip_array[0] = ip_array[0]*10 + (ip_str[i++]-'0');
    }

    ++i;
    while (ip_str[i]!='.') {
        ip_array[1] = ip_array[1]*10 + (ip_str[i++]-'0');
    }

    ++i;
    while (ip_str[i]!='.') {
        ip_array[2] = ip_array[2]*10 + (ip_str[i++]-'0');
    }

    ++i;
    while (ip_str[i]!='\0') {
        ip_array[3] = ip_array[3]*10 + (ip_str[i++]-'0');
    }

    /* convert from byte array to host long integer format */
    ip = (ip_array[0] << 24);
    ip = (ip | (ip_array[1] << 16));
    ip = (ip | (ip_array[2] << 8));
    ip = (ip | ip_array[3]);

    return ip;
}

unsigned int port_str_to_int(char *port_str)
{
    unsigned int port = 0;    
    int i = 0;

    if (port_str==NULL) {
        return 0;
    } 

    while (port_str[i]!='\0') {
        port = port*10 + (port_str[i]-'0');
        ++i;
    }

    return port;
}

static int classificate(struct packet_info *packet, bool in)
{
  struct list_head *p;
  struct fw_rule *rule;
  int i;
 
  /* go through the list of firewall's rules and if there is a match drop the
   * packet. */
  list_for_each(p, &policy_list.list) {
    i++;
    rule = list_entry(p, struct fw_rule, list);

    printk(KERN_INFO "trying to apply rule %d\n", rule->num);
    if (rule->src_ip_any) {
      /* rule doesn't specify IPv4 address */
    } else {
      if (rule->src_ip != packet->src_ip) {
        /* rule's src IPv4 doesn't match packet's IPv4, try next rule */
        printk(KERN_INFO "src IP didn't matched\n");
        continue;
      } else {
        printk(KERN_INFO "MATCH src IP: %d == %d?\n", rule->src_ip, packet->src_ip);
      }

    }

    if (rule->dest_ip_any) {
      /* rule doesn't specify IPv4 address */
      printk(KERN_INFO "dest IP: any\n");
    } else {
      if (rule->dest_ip != packet->dest_ip) {
        /* rule's dest IPv4 doesn't match packet's IPv4, try next rule */
        printk(KERN_INFO "dest IP didn't matched\n");
        continue;
      } else {
        printk(KERN_INFO "MATCH: dest IP: %d == %d?\n", rule->dest_ip, packet->dest_ip);
      }
    }

    if (rule->src_port == 0) {
      /* rule doesn't specify source port */
    } else {
      if (rule->src_port != packet->src_port) {
        /* rule's source port doesn't match packet's packet's source port, 
         * try next rule */
        printk(KERN_INFO "src port didn't matched\n");
        continue;
      } else {
        printk(KERN_INFO "MATCH: src port: %d == %d?\n", rule->src_port, packet->src_port);
      }
    }

    if (rule->dest_port == 0) {
      /* rule doesn't specify source port */
    } else {
      printk(KERN_INFO "dest port: %d == %d?\n", rule->dest_port, packet->dest_port);
      if (rule->dest_port != packet->dest_port) {
        /* rule's source port doesn't match packet's packet's source port, 
         * try next rule */
        printk(KERN_INFO "dest port didn't matched\n");
        continue;
      } else {
        printk(KERN_INFO "MATCH: dest port: %d == %d?\n", rule->dest_port, packet->dest_port);
      }
    }

    if (rule->protocol != packet->proto) {
      /* rule's protocol doesn't match, try next rule */
      printk(KERN_INFO "protocol didn't matched\n");
      continue;
    } else {
      printk(KERN_INFO "MATCH protocol: %d == %d?\n", rule->protocol, packet->proto);
    }

    /* the packet successfully passed the through the rule, do the action */
    if (rule->action == DENY) {
      printk(KERN_INFO "Packet DROP (rule :%d)\n", rule->num);
      return DENY;
    }
  }

  /* default behaviour: allow the packet */
  printk(KERN_INFO "Packet ALLOWED\n");
  return ALLOW;
}

void print_rule(const char *msg, struct fw_rule *rule)
{
  printk(KERN_INFO "%s: %u %s %s from %pI4 (%s) to %pI4 (%s) src-port %u dest-port %u\n", msg, rule->num, action_str(rule->action), proto_ver(rule->protocol), &rule->src_ip, (rule->src_ip_any)?"any":"", &rule->dest_ip, (rule->dest_ip_any)?"any":"", rule->src_port, rule->dest_port);
}

void ip_hl_to_str(unsigned int ip, char *ip_str)
{
    /*convert hl to byte array first*/
    unsigned char ip_array[4];
    memset(ip_array, 0, 4);
    ip_array[0] = (ip_array[0] | (ip >> 24));
    ip_array[1] = (ip_array[1] | (ip >> 16));
    ip_array[2] = (ip_array[2] | (ip >> 8));
    ip_array[3] = (ip_array[3] | ip);

    sprintf(ip_str, "%u.%u.%u.%u", ip_array[0], ip_array[1], ip_array[2], ip_array[3]);
}

void add_token_to_buf(unsigned long *procf_buffer_pos, char token[], char *e)
{
  memcpy(procf_buffer + *procf_buffer_pos, token, strlen(token));
  *procf_buffer_pos += strlen(token);
  memcpy(procf_buffer + *procf_buffer_pos, e, strlen(e));  /* add extra space */
  (*procf_buffer_pos)++;
}

struct fw_rule *get_item_by_id(int id)
{
  struct fw_rule *rule;

  list_for_each_entry(rule, &policy_list.list, list) {
    printk(KERN_INFO "check rule-id(%d) with id(%d)\n", rule->num, id);
    if (rule->num == id) {
      printk(KERN_INFO "Rule with id=%d already exist\n", rule->num);
      return rule;
    }
  }

  return NULL;
}

void delete_rule(int id)
{
  struct list_head *p, *q;
  struct fw_rule *rule;

  list_for_each_safe(p, q, &policy_list.list) {

    rule = list_entry(p, struct fw_rule, list);
    if (rule->num == id) {
      list_del(p);
      kfree(rule);
      printk(KERN_INFO "Rule with id=%d removed\n", id);
      return;
    }
  }

  printk(KERN_INFO "There is no such rule with id=%d\n", id);
}

int add_rule(
             unsigned int num,
             unsigned int action,
             unsigned int protocol,
             const char *src_ip,
             const char *dest_ip,
             bool src_ip_any,
             bool dest_ip_any,
             unsigned int src_port,
             unsigned int dest_port)
{
  unsigned int tmp;
  struct fw_rule *rule;
  struct fw_rule *new;
  struct list_head *pos;
  bool ftail = false;

  /* We want to keed the IDs in the list ordered. Find a rule with a first
   * higher id then the rule we want to add have. If the rule with id already
   * exist then fail.*/
  list_for_each(pos, &policy_list.list) {
    rule = list_entry(pos, struct fw_rule, list);
    if (rule->num > num) {
      /* put new rule in front of the pos */
      break;
    } else if (rule->num == num) {
      printk(KERN_INFO "Rule with id=%d already exist\n", rule->num);
      return 1;
    } else {
      /* adding a rule with highest ID -> has to be added to the end of the list
       * */
      ftail = true;
    }
  }

  /* correct the pos pointer if the list is empty */
  if (pos == policy_list.list.next)
    pos = &policy_list.list;

  new = kmalloc(sizeof(*new), GFP_KERNEL);
  if (new == NULL) {
    return 1;
  }

  new->num = num;
  if (!(new->src_ip_any = src_ip_any))     /* set the value first */
    new->src_ip = ((tmp=ip_str_to_hl(src_ip))==0) ? 0 : ntohl(tmp);
  if (!(new->dest_ip_any = dest_ip_any)) {  /* set the value first */
    new->dest_ip = ((tmp=ip_str_to_hl(dest_ip))==0) ? 0 : ntohl(tmp);
  }
  new->src_port = src_port;
  new->dest_port = dest_port;
  new->protocol = protocol;  /* 1: tcp, 2: udp, 3: icmp, 4: ip */
  new->action = action;

  if (ftail) {
    list_add_tail(&(new->list), pos);
  } else {
    list_add(&(new->list), pos);
  }

  return 0;
}


static ssize_t procRead(struct file *fp, char *buffer, size_t len, 
                        loff_t *offset)
{
  static int finished = 0;
  int ret;
  struct fw_rule *rule;
  char token[20];
  int i;

  printk(KERN_INFO "procf_read (/proc/%s) called \n", PROCF_NAME);

  if (finished) {
    printk(KERN_INFO "finished is 1, nothing to read\n");
    finished = 0;
    return 0;
  } else {
    procf_buffer_pos = 0;
    ret = 0;

    printk(KERN_INFO "starting reading rules from the list\n");
    list_for_each_entry(rule, &policy_list.list, list) {

      finished = 1;

      /* number */
      sprintf(token, "%u", rule->num);
      add_token_to_buf(&procf_buffer_pos, token, " ");
      for (i = 0; i < 6 - strlen(token); i++) {
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
      }

      /* action */
      strcpy(token, action_str(rule->action));
      add_token_to_buf(&procf_buffer_pos, token, " ");
      if (rule->action == ALLOW) {
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
      } else {
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
      }

      /* src ip */
      if (rule->src_ip_any) {
        strcpy(token, "*");
      } else {
        ip_hl_to_str(rule->src_ip, token);
      } 
      add_token_to_buf(&procf_buffer_pos, token, " ");
      for (i = 0; i < 15 - strlen(token); i++) {
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
      }

      /* src port */
      if (rule->src_port == 0) {
        sprintf(token, "*");
      } else {
        sprintf(token, "%u", rule->src_port);
      }
      add_token_to_buf(&procf_buffer_pos, token, " ");
      for (i = 0; i < 7 - strlen(token); i++) {
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
      }

      /* dest ip */
      if (rule->dest_ip_any) {
        strcpy(token, "*");
      } else {
        ip_hl_to_str(rule->dest_ip, token);
      } 
      add_token_to_buf(&procf_buffer_pos, token, " ");
      for (i = 0; i < 15 - strlen(token); i++) {
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
      }

      /* dest port */
      if (rule->dest_port == 0) {
        sprintf(token, "*");
      } else {
        sprintf(token, "%u", rule->dest_port);
      }
      add_token_to_buf(&procf_buffer_pos, token, " ");
      for (i = 0; i < 7 - strlen(token); i++) {
        memcpy(procf_buffer + procf_buffer_pos, " ", 1); procf_buffer_pos++;
      }

      /* protocol */
      strcpy(token, proto_ver(rule->protocol));
      add_token_to_buf(&procf_buffer_pos, token, "\n");
    }

    memcpy(buffer, procf_buffer, procf_buffer_pos);
    ret = procf_buffer_pos;
  }

  /* you need to return how many bytes did you write into the buffer */
  return ret;
}
 
static unsigned long procfs_buffer_size = 0;
static char procfs_buffer[PROCF_MAX_SIZE];

enum fw_rule_fields {NUM, ACTION, PROTOCOL, FROM, SRC_IP, TO, DEST_IP, SRC_PORT_STR, SRC_PORT, DEST_PORT_STR, DEST_PORT};
 
/* If the file is not big enough then this func is called more than once. For
 * example if the file is 32B long and a user write 64B to the file then this
 * function would be called twice.
 */
static ssize_t procWrite(struct file *file, const char *buffer, size_t count, loff_t *off)
{
  static char token[16];
  static int j = 0;
  static enum fw_rule_fields type = NUM;
  static bool rule_complete = false;

  int read_again = false;
  int i;

  static unsigned int num = 0;
  static bool src_ip_any = false;          /* true: source ip is set to any */
  static bool dest_ip_any = false;         /* true: source ip is set to any */
  static char src_ip[15];
  static char dest_ip[15];
  static unsigned int src_port;
  static unsigned int dest_port;
  static int protocol;  /* 1: tcp, 2: udp, 3: icmp, 4: ip */
  static int action;    /* 1: allow, 2: deny */

  printk(KERN_INFO "procf_write is called.\n");

  procfs_buffer_size = count;
  if (procfs_buffer_size > PROCF_MAX_SIZE) {
    procfs_buffer_size = PROCF_MAX_SIZE;
    read_again = true;
    printk(KERN_INFO "Data from user are bigger then my buffer (buf_len=%dB)\n", PROCF_MAX_SIZE);
  }

  /* write data to the buffer */
  if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
    return -EFAULT;
  }

  printk(KERN_INFO "buf: %s\n", procfs_buffer);

  /* delete command have following format: "d number" */
  if (strncmp(procfs_buffer, "delete", strlen("delete")) == 0) {
    i = 7;  /* start with first number char */
    num = 0;

    while (i < procfs_buffer_size) {
      printk(KERN_INFO "procfs_buffer[%i]: %c\n", i, procfs_buffer[i]);
      num = num*10 + (procfs_buffer[i]-'0');
      ++i;
    }

    printk(KERN_INFO "Delete rule with id: %u\n", num);
    delete_rule(num);
    num = 0;

    if (read_again)
      printk(KERN_INFO "ERROR: Buffer was big enough for delete command!\n");

    return procfs_buffer_size;
  }

  i = 0; 
  while (i < procfs_buffer_size) {

    /* FIXME: use strstr() and then copy chars to found white space would be
     * probably more effective ... */

    if (procfs_buffer[i] == '\n' || (i+1 == procfs_buffer_size && read_again == false)) {
      /* now we are sure we have one entire rule */
      rule_complete = true;
      /* and end the current token */
      token[j] = '\0';
      j = 0;
    } else if (procfs_buffer[i] == ' ') {
      token[j] = '\0';  /* the token is complete */
      j = 0;            /* start writing to the begining the next time */
    } else {
      /* non whitechar */
      //printk(KERN_INFO "token[%u]: %c\n", j, procfs_buffer[i]);
      token[j] = procfs_buffer[i];
      j++;
      i++;
      continue;
    }

    printk(KERN_INFO "token: %s\n", token);
    if (strcmp(token, "src-port") == 0) {
      type = SRC_PORT_STR;
    } else if (strcmp(token, "dst-port") == 0) {
      type = DEST_PORT_STR;
    }

    switch (type) {
      case NUM:
        printk(KERN_INFO "NUM <- %s\n", token);
        num = str_to_uint(token);
        break;
      case ACTION:
        printk(KERN_INFO "ACTION <- %s\n", token);
        if (strcmp(token, "deny") == 0) {
          action = DENY;
        } else {
          action = ALLOW;
        }
        break;
      case PROTOCOL:
        printk(KERN_INFO "PROTOCOL <- %s\n", token);
        if (strcmp(token, "tcp") == 0) {
          protocol = TCP;
        } else if (strcmp(token, "udp") == 0) {
          protocol = UDP;
        } else if (strcmp(token, "icmp") == 0) {
          protocol = ICMP;
        } else {
          protocol = IP;
        }
        break;
      case FROM:
        printk(KERN_INFO "FROM <- %s\n", token);
        break;
      case SRC_IP:
        printk(KERN_INFO "SRC_IP <- %s\n", token);
        if (strcmp(token, "any") == 0) {
          src_ip_any = true;
        } else {
          src_ip_any = false;
          strncpy(src_ip, token, strlen(token));
        }
        break;
      case TO:
        printk(KERN_INFO "TO <- %s\n", token);
        break;
      case DEST_IP:
        printk(KERN_INFO "DEST_IP <- %s\n", token);
        if (strcmp(token, "any") == 0) {
          dest_ip_any = true;
        } else {
          dest_ip_any = false;
          strncpy(dest_ip, token, strlen(token));
        }
        break;
      case SRC_PORT_STR:
        printk(KERN_INFO "SRC_PORT_STR <- %s\n", token);
        break;
      case SRC_PORT:
        printk(KERN_INFO "SRC_PORT <- %s\n", token);
        src_port = str_to_uint(token);
        printk(KERN_INFO "src_port converted to: %d\n", src_port);
        break;
      case DEST_PORT_STR:
        printk(KERN_INFO "DEST_PORT_STR <- %s\n", token);
        break;
      case DEST_PORT:
        printk(KERN_INFO "DEST_PORT <- %s\n", token);
        dest_port = str_to_uint(token);
        printk(KERN_INFO "dest_port converted to: %d\n", dest_port);
        break;
    }

    if (rule_complete) {
      printk(KERN_INFO "RULE READING DONE!\n");
      printk(KERN_INFO "RULE: %d %s %s from %s to %s src-port %u dest-port %u\n", 
                        num, action_str(action), proto_ver(protocol), src_ip, 
                        dest_ip, src_port, dest_port);

      add_rule(num, action, protocol, src_ip, dest_ip, src_ip_any, 
               dest_ip_any, src_port, dest_port);

      rule_complete = false;
      type = 0;   /* reset token type -> start with NUM again */

      num = 0;
      src_ip_any = false;          /* true: source ip is set to any */
      dest_ip_any = false;         /* true: source ip is set to any */
      src_port = 0;
      dest_port = 0;
      protocol = 0;  /* 1: tcp, 2: udp, 3: icmp, 4: ip */
      action = 0;    /* 1: allow, 2: deny */
    } else {
      type++;
    }

    i++;
  }

  /* whipe out the buffer */
  //memcpy(buffer, '\0', procfs_buffer_size);

  return procfs_buffer_size;
}

int procOpen(struct inode *inode, struct file *fp) {
  printk(KERN_INFO "procOpen called\n");
  return 0;
}

int procClose(struct inode *inode, struct file *fp) {
  printk(KERN_INFO "procClose called\n");
        return 0;
}


/* Get packet information and return it in packet struct 
 * @packet:   struct for packet information
 * @skb:      packet
 * @in:       incoming packet? outgoing otherwise
 */
void get_packet_info(struct packet_info *packet, struct sk_buff *skb, bool in)
{
  /* icmp doesn't use ports */
  packet->src_port = 0;
  packet->dest_port = 0;
   
  packet->ip_header = (struct iphdr *)skb_network_header(skb);
  packet->src_ip = (unsigned int)packet->ip_header->saddr;
  packet->dest_ip = (unsigned int)packet->ip_header->daddr;

  printk(KERN_INFO "protocol: %d\n", packet->ip_header->protocol);
  if (packet->ip_header->protocol == UDP) {
    packet->proto = UDP;
    packet->udp_header = (struct udphdr *)(skb_transport_header(skb)+20);
    packet->src_port = (unsigned int)ntohs(packet->udp_header->source);
    packet->dest_port = (unsigned int)ntohs(packet->udp_header->dest);
  } else if (packet->ip_header->protocol == TCP) {
    packet->proto = TCP;
    packet->tcp_header = (struct tcphdr *)(skb_transport_header(skb)+20);
    packet->src_port = (unsigned int)ntohs(packet->tcp_header->source);
    packet->dest_port = (unsigned int)ntohs(packet->tcp_header->dest);
  } else if (packet->ip_header->protocol == ICMP) {
    packet->proto = ICMP;
  } else {
    packet->proto = IP;
  }

  printk(KERN_INFO "%s packet: src ip=%pI4:%u, dst ip=%pI4:%u, protol=%s\n", in_out_str(in), &packet->src_ip, packet->src_port, &packet->dest_ip, packet->dest_port, proto_ver(packet->proto));
}


unsigned int hook_in_packet(const struct nf_hook_ops *ops, 
                            struct sk_buff *skb, 
                            const struct net_device *in, 
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
  struct packet_info packet;

  /* get information about incoming packet */
  get_packet_info(&packet, skb, true);

  /* classificat the incoming packet */
  if (classificate(&packet, false) == DENY) {
    return NF_DROP;
  }

  return NF_ACCEPT;                
}

unsigned int hook_out_packet(const struct nf_hook_ops *ops, 
                             struct sk_buff *skb, 
                             const struct net_device *in, 
                             const struct net_device *out,
                             int (*okfn)(struct sk_buff *))
{
  struct packet_info packet;

  /* get information about outgoing packet */
  get_packet_info(&packet, skb, false);

  /* classificat the outgoing packet  */
  if (classificate(&packet, false) == DENY) {
    return NF_DROP;
  }

  return NF_ACCEPT;
} 


/* Initialization routine */
int init_module()
{
  static struct file_operations procFops;
  printk(KERN_INFO "inicializing pds kernel firewall module\n");

  /* inicialize the list of firewall rules */
  INIT_LIST_HEAD(&(policy_list.list));
  add_rule(20, DENY, ICMP, "192.168.0.1", NULL, false, true, 0, 0);
  add_rule(1, DENY, ICMP, "127.0.0.1", NULL, false, true, 0, 0);
  add_rule(20, DENY, ICMP, "192.168.0.1", NULL, false, true, 0, 0);
  add_rule(30, ALLOW, ICMP, "192.168.123.123", "192.168.123.123", false, false, 65535, 65535);
  add_rule(15, ALLOW, TCP, NULL, "192.168.0.1", true, false, 0, 0);

  procf_buffer = (char *) vmalloc(PROCF_MAX_SIZE);


  /* inicialize operations for my proc file */
  procFops.open = procOpen;
  procFops.release = procClose;
  procFops.read = procRead;
  procFops.write = procWrite;

  mf_proc_file = proc_create(PROCF_NAME, 0644, NULL, &procFops);
  if (mf_proc_file==NULL) {
      printk(KERN_INFO "Error: could not initialize /proc/%s\n", PROCF_NAME);
      return -ENOMEM; 
  } 

  printk(KERN_INFO "/proc/%s is created\n", PROCF_NAME);

  /* initialize hook struct for incoming packets */
  nfho_in.hooknum = NF_INET_LOCAL_IN;
  nfho_in.pf = PF_INET;
  nfho_in.priority = NF_IP_PRI_FIRST;
  nfho_in.hook = hook_in_packet;
  nf_register_hook(&nfho_in);

  /* initialize hook struct for outgoing packets */
  nfho_out.hooknum = NF_INET_LOCAL_OUT;
  nfho_out.pf = PF_INET;
  nfho_out.priority = NF_IP_PRI_FIRST;
  nfho_out.hook = hook_out_packet;
  nf_register_hook(&nfho_out);
 
  return 0;
}

/* Cleanup routine */
void cleanup_module()
{
  struct list_head *p, *q;
  struct fw_rule *rule;

  printk(KERN_INFO "cleanup_module() routine called.\n");
  printk(KERN_INFO "removing /proc/%s file\n", PROCF_NAME);
  remove_proc_entry(PROCF_NAME, NULL);

  printk(KERN_INFO "freeing list of firewall rules\n");
  list_for_each_safe(p, q, &policy_list.list) {
    rule = list_entry(p, struct fw_rule, list);
    list_del(p);
    kfree(rule);
  }

  printk(KERN_INFO "removing netfilet in and out hooks\n");
  nf_unregister_hook(&nfho_in);
  nf_unregister_hook(&nfho_out);

  printk(KERN_INFO "kernel module unloaded.\n");
}

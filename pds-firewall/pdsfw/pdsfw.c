#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
 
#define PROCF_NAME "pdsfw_xsruba03"
//#define PROCF_MAX_SIZE  1024
#define PROCF_MAX_SIZE  25

#define proto_ver(x) (x == UDP) ? "udp" : ((x==ICMP) ? "icmp" : "tcp")
#define action_str(x) (x == ALLOW) ? "allow" : "deny"
#define in_out_str(x) (x == true) ? "INCOMMING" : "OUTGOING"

#define IP        0   //FIXME
#define ICMP        1
#define UDP         17
#define TCP         6
#define ALLOW       1
#define DENY        2

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDS firewall");
MODULE_AUTHOR("Michal Srubar, xsruba03@stud.fit.vutbr.cz");

static struct nf_hook_ops nfho_in;
static struct nf_hook_ops nfho_out;

enum fw_rule_fields {
  NUM = 0,
  ACTION, 
  PROTOCOL, 
  FROM, 
  SRC_IP, 
  TO, 
  DEST_IP, 
  SRC_PORT_STR, 
  SRC_PORT, 
  DEST_PORT_STR,
  DEST_PORT,
};

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
};

//the structure used for procfs
static struct proc_dir_entry *mf_proc_file;
unsigned long procf_buffer_pos;
char *procf_buffer;

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

    printk(KERN_INFO "dest ip: packet(%d) == rule(%d)\n", packet->dest_ip, rule->dest_ip);
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

    if (rule->protocol != packet->ip_header->protocol) {
      /* rule's protocol doesn't match, try next rule */
      printk(KERN_INFO "protocol didn't matched\n");
      continue;
    } else {
      printk(KERN_INFO "MATCH protocol: %d == %d?\n", rule->protocol, packet->ip_header->protocol);
    }

    /* the packet successfully passed the through the rule, do the action */
    if (rule->action == DENY) {
      printk(KERN_INFO "Packet DROPED (rule :%d)\n", rule->num);
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


void add_token_to_buf(unsigned long *procf_buffer_pos, char token[], char *e)
{
  memcpy(procf_buffer + *procf_buffer_pos, token, strlen(token));
  *procf_buffer_pos += strlen(token);
  memcpy(procf_buffer + *procf_buffer_pos, e, 1);  /* add extra space */
  (*procf_buffer_pos)++;
}

/* Check if the rule with num id is already in list or not.
 * @num - id of rule we are looking for
 *
 * returns: 0 - the id DOES NOT exist yet
 *          1 - the rule with id already exists
 */
int check_id_rule(int num)
{
  struct fw_rule *rule;
  struct list_head *p, *q;

  list_for_each_safe(p, q, &(policy_list.list)) {
    rule = list_entry(p, struct fw_rule, list);

    if (rule->num == num) {
      printk(KERN_INFO "Rule with id=%d already exists\n", num);
      return 1;
    }
  }

  return 0;
}

int add_rule(struct fw_rule *policy_list,
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


  struct fw_rule *new = kmalloc(sizeof(*new), GFP_KERNEL);
  if (new == NULL) {
    return 1;
  }

  if (check_id_rule(num) == 1)
    return 1;

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

  INIT_LIST_HEAD(&(new->list));
  list_add_tail(&(new->list), &(policy_list->list));

  print_rule("ADD RULE", new);

  return 0;
}

void delete_rule(int num)
{
  struct fw_rule *rule;
  struct list_head *p, *q;

  //printk(KERN_INFO "Deleting rule number: %d\n", num);

  list_for_each_safe(p, q, &policy_list.list) {
    rule = list_entry(p, struct fw_rule, list);

    if (rule->num == num) {
      list_del(p);
      kfree(rule);
      printk(KERN_INFO "Rule with id=%d deleted\n", num);
      return;
    }
  }

  printk(KERN_INFO "No such rule with id=%d found\n", num);
}

static ssize_t procRead(struct file *fp, char *buffer, size_t len, 
                        loff_t *offset)
{
  static int finished = 0;
  int ret;
  struct fw_rule *rule;
  char token[20];

  printk(KERN_INFO "procf_read (/proc/%s) called \n", PROCF_NAME);

  if (finished) {
    printk(KERN_INFO "eof is 1, nothing to read\n");
    finished = 0;
    return 0;
  } else {
    procf_buffer_pos = 0;
    ret = 0;

    list_for_each_entry(rule, &policy_list.list, list) {

      finished = 1;

      /* number */
      sprintf(token, "%u", rule->num);
      add_token_to_buf(&procf_buffer_pos, token, "\t");

      /* action */
      strcpy(token, action_str(rule->action));
      add_token_to_buf(&procf_buffer_pos, token, "\t");

      /* protocol */
      strcpy(token, proto_ver(rule->protocol));
      add_token_to_buf(&procf_buffer_pos, token, "\t");

      /* src ip */
      if (rule->src_ip_any) {
        strcpy(token, "*\t");
      } else {
        ip_hl_to_str(rule->src_ip, token);
      } 
      add_token_to_buf(&procf_buffer_pos, token, "\t");

      /* dest ip */
      if (rule->dest_ip_any) {
        strcpy(token, "*\t");
      } else {
        ip_hl_to_str(rule->dest_ip, token);
      } 
      memcpy(procf_buffer + procf_buffer_pos, token, strlen(token));
      add_token_to_buf(&procf_buffer_pos, token, "\t");

      if (rule->src_port) {
        /* src port */
        sprintf(token, "%u", rule->src_port);
        add_token_to_buf(&procf_buffer_pos, token, "\t");
      }

      if (rule->dest_port) {
        /* dest port */
        sprintf(token, "%u", rule->dest_port);
        add_token_to_buf(&procf_buffer_pos, token, "");
      }

      add_token_to_buf(&procf_buffer_pos, "", "\n");
    }

    memcpy(buffer, procf_buffer, procf_buffer_pos);
    ret = procf_buffer_pos;
  }

  /* you need to return how many bytes did you write into the buffer */
  return ret;
}
 
static unsigned long procfs_buffer_size = 0;
static char procfs_buffer[PROCF_MAX_SIZE];

 

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
  static char src_ip[15] = {[14] = '\0'};
  static char dest_ip[15] = {[14] = '\0'};
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

  printk(KERN_INFO "buf(%i): %s\n", procfs_buffer_size, procfs_buffer);

  /* delete command have following format: "d number" */
  if (procfs_buffer[0] == 'd') {
    printk(KERN_INFO "Delete cmd received\n");

    i = 2;
    while (i < procfs_buffer_size) {

      /* skip new line char */
      if (procfs_buffer[i] == '\n') {
        i++;
        continue;
      }

      num = num*10 + (procfs_buffer[i]-'0');
      i++;
    }

    if (read_again)
      printk(KERN_INFO "ERROR: Buffer was big enough for delete command!\n");

    //printk(KERN_INFO "calling delete_rule(%d)\n", num);
    delete_rule(num);
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
        printk(KERN_INFO "SRC_PORT <- %s\n", token);
        break;
      case SRC_PORT:
        printk(KERN_INFO "SRC_PORT <- %s\n", token);
        src_port = str_to_uint(token);
        break;
      case DEST_PORT_STR:
        printk(KERN_INFO "DEST_PORT <- %s\n", token);
        break;
      case DEST_PORT:
        printk(KERN_INFO "DEST_PORT <- %s\n", token);
        dest_port = str_to_uint(token);
        break;
    }

    if (rule_complete) {
      printk(KERN_INFO "RULE READING DONE!\n");

      add_rule(&policy_list, num, action, protocol, src_ip, dest_ip, src_ip_any, 
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
  if (packet->ip_header->protocol == UDP) {
    packet->udp_header = (struct udphdr *)(skb_transport_header(skb)+20);
    packet->src_port = (unsigned int)ntohs(packet->udp_header->source);
    packet->dest_port = (unsigned int)ntohs(packet->udp_header->dest);
  } else if (packet->ip_header->protocol == TCP) {
    packet->tcp_header = (struct tcphdr *)(skb_transport_header(skb)+20);
    packet->src_port = (unsigned int)ntohs(packet->tcp_header->source);
    packet->dest_port = (unsigned int)ntohs(packet->tcp_header->dest);
  }

  printk(KERN_INFO "%s packet: src ip=%pI4:%u, dst ip=%pI4:%u, protol=%s\n", in_out_str(in), &packet->src_ip, packet->src_port, &packet->dest_ip, packet->dest_port, proto_ver(packet->ip_header->protocol));
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
  add_rule(&policy_list, 10, DENY, ICMP, "127.0.0.1", NULL, false, true, 0, 0);
  add_rule(&policy_list, 20, DENY, ICMP, "192.168.0.1", NULL, false, true, 0, 0);

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

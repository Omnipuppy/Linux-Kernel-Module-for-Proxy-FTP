#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/inet.h>
#include <linux/udp.h>
#include <net/sock.h>

MODULE_LICENSE("GPL");

#define FTP_PORT 21
#define BUFFER_SIZE 1024

static char* proxy_src_ip;
static char* proxy_dst_ip;
static int proxy_upload;
static int proxy_download;

module_param(proxy_src_ip, charp, 0);
module_param(proxy_dst_ip, charp, 0);
module_param(proxy_upload, bool, 0);
module_param(proxy_download, bool, 0);

static struct nf_hook_ops nfho;
static struct socket *ftp_sock;

static int ftp_connect(struct socket **sock, struct sockaddr_in *addr) {
    int err;
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = in_aton(proxy_dst_ip);
    addr->sin_port = htons(FTP_PORT);
    err = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, sock);
    if (err < 0) {
        printk(KERN_ERR "Error creating socket: %d", err);
        return err;
    }
    err = kernel_connect(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in), 0);
    if (err < 0) {
        printk(KERN_ERR "Error connecting to FTP server: %d", err);
        sock_release(*sock);
        return err;
    }
    return 0;
}

static int proxy_handler(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);
    struct tcphdr *tcp_header;
    struct sockaddr_in ftp_addr;
    struct msghdr msg;
    struct iovec iov;
    int res, data_len;
    char *buf;

    if (ip_header->protocol == IPPROTO_TCP) {
        tcp_header = (struct tcphdr *)((__u32 *)ip_header + ip_header->ihl);
        if (tcp_header->dest == htons(44443)) {
            if (proxy_upload) {
                res = ftp_connect(&ftp_sock, &ftp_addr);
                if (res < 0) {
                    return NF_DROP;
                }
                buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
                if (!buf) {
                    return NF_DROP;
                }
                iov.iov_base = skb_transport_header(skb);
                iov.iov_len = skb->len - (skb_transport_header(skb) - skb->data);
                msg.msg_name = &ftp_addr;
                msg.msg_namelen = sizeof(struct sockaddr_in);
                msg.msg_control = NULL;
                msg.msg_controllen = 0;
                msg.msg_flags = 0;
                data_len = kernel_sendmsg(ftp_sock, &msg, &iov, 1, iov.iov_len);
                kfree(buf);
                sock_release(ftp_sock);
            } else if (proxy_download) {
                res = ftp_connect(&ftp_sock, &ftp_addr);
                if (res < 0) {
                    return NF_DROP;
                }
buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
if (!buf) {
return NF_DROP;
}
data_len = kernel_recvmsg(ftp_sock, &msg, &iov, 1, iov.iov_len, 0);
if (data_len <= 0) {
kfree(buf);
sock_release(ftp_sock);
return NF_DROP;
}
iov.iov_base = buf;
iov.iov_len = data_len;
res = kernel_sendmsg(ftp_sock, &msg, &iov, 1, iov.iov_len);
if (res < 0) {
kfree(buf);
sock_release(ftp_sock);
return NF_DROP;
}
kfree(buf);
sock_release(ftp_sock);
}
}
}
return NF_ACCEPT;
}

static int __init proxy_init(void) {
nfho.hook = proxy_handler;
nfho.hooknum = NF_INET_LOCAL_OUT;
nfho.pf = PF_INET;
nfho.priority = NF_IP_PRI_FIRST;
nf_register_net_hook(&init_net, &nfho);
printk(KERN_INFO "FTP proxy module loaded\n");
return 0;
}

static void __exit proxy_exit(void) {
nf_unregister_net_hook(&init_net, &nfho);
printk(KERN_INFO "FTP proxy module unloaded\n");
}

module_init(proxy_init);
module_exit(proxy_exit);


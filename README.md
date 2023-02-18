# Linux-Kernel-Module-for-Proxy-FTP
ChatGPT created Rootkit to create a Proxyd FTP Anonymously (if enabled)

sudo apt-get install linux-headers-$(uname -r)

gcc -o my_module.o -c my_module.c -I /lib/modules/$(uname -r)/build/include

make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

sudo insmod my_module.ko proxy_src_ip=192.168.1.1 proxy_dst_ip=10.0.0.2 proxy_upload=1 proxy_download=0


UPlOAD

sudo insmod ftp_proxy.ko proxy_src_ip=<proxy_src_ip> proxy_dst_ip=<proxy_dst_ip> proxy_upload=1 proxy_download=0
curl -T <local_file_path> ftp://<proxy_dst_ip>:44443/<remote_file_name>

DOWNLOAD

sudo insmod ftp_proxy.ko proxy_src_ip=<proxy_src_ip> proxy_dst_ip=<proxy_dst_ip> proxy_upload=0 proxy_download=1
curl -x <proxy_src_ip>:44443 -o <local_file_name> ftp://<remote_file_path>

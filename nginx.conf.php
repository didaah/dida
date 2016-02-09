<?php  header('Content-Type: text/plain; charset=utf-8'); ?>
#若使用 nginx，可参考以下配置，实现简洁链接和文件保护
#本文件修改自 nginx 1.0 默认配置文件

#user  nobody;
worker_processes  1;

#error_log  logs/error.log;
#error_log  logs/error.log  notice;
#error_log  logs/error.log  info;

#pid        logs/nginx.pid;


events {
    worker_connections  1024;
}


http {
    include       mime.types;
    default_type  application/octet-stream;

    #log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
    #                  '$status $body_bytes_sent "$http_referer" '
    #                  '"$http_user_agent" "$http_x_forwarded_for"';

    #access_log  logs/access.log  main;

    sendfile        on;
    tcp_nopush     on;
    
    # 上传文件
    client_max_body_size 16m;
    
    #keepalive_timeout  0;
    keepalive_timeout  65;
    tcp_nodelay on;

    # 打开 gzip
    gzip  on;

    # 超过 1 k 才进行压缩
    gzip_min_length 1k;

    # 压缩等级
    gzip_comp_level 2;

    # 压缩类型
    gzip_types  text/plain application/x-javascript text/css application/xml application/javascript;

    server {
        listen       80;
        server_name  localhost;

        #charset koi8-r;

        #access_log  logs/host.access.log  main;

        location / {
            root   html;
            index  index.php;
            # dida - 简洁链接
            if (!-e $request_filename) {
              rewrite ^/(.*)$ /index.php?q=$1 last;
            }
        }

        #error_page  404              /404.html;

        # redirect server error pages to the static page /50x.html
        #
        error_page   500 502 503 504  /50x.html;
        location = /50x.html {
            root   html;
        }

        # pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
        # php 支持，视具体情况处理
        location ~ \.php$ {
            root           html;
            fastcgi_pass   127.0.0.1:9000;
            fastcgi_index  index.php;
            fastcgi_param  SCRIPT_FILENAME $document_root$real_script_name; 
            include        fastcgi_params;
        }

        # dida - 保护文件
        location ~ \.(htaccess|engine|inc|info|install|lang|module|profile|test|po|sh|.*sql|theme|tpl(\.php)?|xtmpl|svn-base)$|^(code-style\.pl|Entries.*|Repository|Root|Tag|Template|all-wcprops|entries|format)$ {
            # 当 hook_menu 定义的路径，含有以上后缀时，将发生冲突。
            # deny all;
        
            # 全部交给 index.php 处理
            rewrite ^/(.*)$ /index.php?q=$1 last;
            break;
        }

        # 日志目录保护
        location ^~ /sites/logs {
          deny all;
        }        

        # 文件缓存
        location ~* ^.+.(jpg|jpeg|gif|png|js|css|ico)$ {
          root   html;
          access_log off;
          expires 30d;
          add_header Cache_Control private;
        }

    }
}

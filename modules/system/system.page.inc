<?php
// $Id$

/**
 * 输出 robots.txt 内容
 */
function system_page_robots() {  
$data = <<<EOT
User-agent: *
Crawl-delay: 10

Disallow: /includes/
Disallow: /misc/
Disallow: /modules/
Disallow: /sites/all/
Disallow: /sites/cache/
Disallow: /sites/themes/
Disallow: /sites/modules/
Disallow: /themes/
Disallow: /install/
Disallow: /tools/

Disallow: /rpc.php
Disallow: /cron.php
Disallow: /custom.php
Disallow: /install.php

Disallow: /admin
Disallow: /ajax
Disallow: /captcha
Disallow: /g
Disallow: /search
Disallow: /images/error
Disallow: /user/logout
Disallow: /user/register
Disallow: /user/pass
Disallow: /user/login
EOT;

  if ($cache = custom_get('system_robots')) {
    $data = $cache;
  }

  header('Content-Type:text/plain; charset=utf-8');

  echo $data;

  exit;
}


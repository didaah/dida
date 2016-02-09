<?php
// $Id$

if (!isset($_GET['module'])) die('byebye');

define('DIDA_ROOT', getcwd());

require_once DIDA_ROOT . '/includes/bootstrap.inc';

header('Content-Type: text/html; charset=utf-8');

if (!isset($_GET['bootstrap']) || $_GET['bootstrap'] != 'full') {
  $includes = NULL;
  
  if (!empty($_GET['includes'])) {
    if (strpos($_GET['includes'], '|') !== false) {
      $includes = explode('|', $_GET['includes']);
    } else {
      $includes[] = $_GET['includes'];
    }
  }
  
  bootstrap('custom', $includes);
} else {
  
  bootstrap('full');
}

if ($GLOBALS['conf']['modules'][$_GET['module']]) {
  // 加载模块主文件
  require_once dd_get_path('module', $_GET['module']).'/'.$_GET['module'].'.module';
  
  // 检查函数是否存在
  $function = $_GET['module'].'_call_custom';
  
  if (function_exists($function)) {
    echo $function($_GET);
  } else {
    echo 'byebye';
  }
}
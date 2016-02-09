<?php
// $Id$

/**
 * 脚本开始运行时间
 */
define('DIDA_START_TIME', microtime(true));

/**
 * 工作目录
 * 建议包含文件，使用绝对路径。
 */
define('DIDA_ROOT', getcwd());

require_once DIDA_ROOT . '/includes/bootstrap.inc';

// 程序初始化
bootstrap('full');

// 获取当前路径($_GET['q'])输出
$return = menu_get_item();

// 针对当前路径，所有模块输出完成，触发 hook_exit()
module_invoke_all('exit', 'full');

if (is_int($return)) {
  switch ($return) {
    case MENU_ACCESS_DENIED:
      // 无权限访问
      dd_get_access();
    break;
    case MENU_NOT_FOUND:
      // 路径未定义
      dd_get_not();
  }
  exit;
}

// 调用 page 模板输出内容
print theme('page', $return);

